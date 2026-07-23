#include "file_watcher.h"

#include <windows.h>

#include <array>
#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>

#include "file_encryption.h"
#include "logger.h"

namespace {

constexpr size_t BUFFER_SIZE = 2048;

/// Converts wide-character string to UTF-8 encoding
/// @param wstr Input wide-character string
/// @return UTF-8 encoded std::string
std::string to_utf8(const std::wstring& wstr) {
    if (wstr.empty())
        return {};
    int size_needed =
        WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string str(size_needed - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, str.data(), size_needed, nullptr, nullptr);
    return str;
}

/** Opens a directory handle for monitoring file system changes
 * Uses Windows CreateFileW API with Directory-specific flags for change notifications.
 * @param directory Path to directory to watch (wide-character string)
 * @return std::optional<HANDLE> - valid handle on success, nullopt if CreateFileW fails
 */
std::optional<HANDLE> open_directory_handle(const std::wstring& directory) {
    HANDLE dir_handle = CreateFileW(directory.c_str(), FILE_LIST_DIRECTORY,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                                    OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);

    if (dir_handle == INVALID_HANDLE_VALUE) {
        return std::nullopt;
    }

    return dir_handle;
}

void print_action(DWORD action, const std::wstring& filename_w) {
    std::string filename = to_utf8(filename_w);

    switch (action) {
        case FILE_ACTION_ADDED:
            enc_logger::log.debug("[PRINT_ACTION]File Created: {}", filename);
            break;
        case FILE_ACTION_REMOVED:
            enc_logger::log.debug("[PRINT_ACTION]File Deleted: {}", filename);
            break;
        case FILE_ACTION_MODIFIED:
            enc_logger::log.debug("[PRINT_ACTION]File Modified: {}", filename);
            break;
        case FILE_ACTION_RENAMED_OLD_NAME:
            enc_logger::log.debug("[PRINT_ACTION]File Renamed (Old Name): {}", filename);
            break;
        case FILE_ACTION_RENAMED_NEW_NAME:
            enc_logger::log.debug("[PRINT_ACTION]File Renamed (New Name): {}", filename);
            break;
        default:
            enc_logger::log.debug("[PRINT_ACTION]Unknown Action: {}", filename);
            break;
    }
}

} // anonymous namespace

namespace file_watcher {

void watch_directory(std::string& source_file, const std::string& destination_file,
                     const std::string& password) {
    std::filesystem::path source_path{source_file};
    std::wstring watch_dir = source_path.parent_path().wstring();
    std::wstring watch_filename = source_path.filename().wstring();

    auto directory_handle_opt = open_directory_handle(watch_dir);
    if (!directory_handle_opt.has_value()) {
        enc_logger::log.error("Failed to open directory for watching. Error: {}", GetLastError());
        return;
    }
    HANDLE watch_directory_handle = directory_handle_opt.value();
    enc_logger::log.info("Watching for changes in {}", to_utf8(watch_dir));

    std::array<char, BUFFER_SIZE> buffer;
    DWORD bytes_returned;

    while (true) {
        WINBOOL result = ReadDirectoryChangesW(
            watch_directory_handle, buffer.data(), static_cast<DWORD>(buffer.size()),
            FALSE, // Don't watch subdirectories
            FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_FILE_NAME, &bytes_returned, nullptr,
            nullptr);
        if (!static_cast<bool>(result)) {
            DWORD error = GetLastError();
            enc_logger::log.error("ReadDirectoryChangesW failed. Error: {}", error);
            continue;
        }

        auto* notify_info = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(buffer.data());

        std::optional<std::wstring> rename_old_name;

        do {
            std::wstring filename_notified(notify_info->FileName,
                                           notify_info->FileNameLength / sizeof(WCHAR));

            print_action(notify_info->Action, filename_notified);

            if (notify_info->Action == FILE_ACTION_MODIFIED &&
                filename_notified == watch_filename) {
                enc_logger::log.info("File Modified: {}", to_utf8(filename_notified));
                int rc =
                    file_encryption::encrypt_file_stream(source_file, destination_file, password);
                if (rc == 0) {
                    enc_logger::log.debug("Encryption successful of Modified file {}",
                                          to_utf8(filename_notified));
                } else {
                    enc_logger::log.error("Encryption failed of Modified file {}",
                                          to_utf8(filename_notified));
                }

            } else if (notify_info->Action == FILE_ACTION_RENAMED_OLD_NAME &&
                       filename_notified == watch_filename) {
                enc_logger::log.debug("File Renamed from: {}", to_utf8(filename_notified));
                rename_old_name = filename_notified;

            } else if (notify_info->Action == FILE_ACTION_RENAMED_NEW_NAME) {
                enc_logger::log.debug("File Renamed to: {}", to_utf8(filename_notified));

                if (rename_old_name && *rename_old_name == watch_filename) {
                    enc_logger::log.debug(
                        "Renamed file matches previous watch target. Updating source file path and "
                        "re-encrypting...");

                    enc_logger::log.debug(
                        "Old Filename: {}, \n\tNew Filename: {},"
                        "\n\tOld Source path: {},"
                        "\n\tNew Source path: {}",
                        to_utf8(*rename_old_name), to_utf8(filename_notified), source_path.string(),
                        (source_path.parent_path() / to_utf8(filename_notified)).string());

                    // Updating the file name to original source path here
                    source_path.replace_filename(filename_notified);
                    source_file = source_path.string();
                    watch_filename = filename_notified;
                    rename_old_name.reset();

                    enc_logger::log.info(
                        "Renamed file matches watch target. Updated source path to {}",
                        source_file);
                    int rc = file_encryption::encrypt_file_stream(source_file, destination_file,
                                                                  password);
                    if (rc == 0) {
                        enc_logger::log.debug("Encryption successful.");
                    } else {
                        enc_logger::log.error("Encryption failed.");
                    }
                } else if (filename_notified == watch_filename) {
                    // This is created after watcher is open. Refer to main.cpp:L39
                    enc_logger::log.info(
                        "Renamed file matches watch target. Performing initial encryption...");
                    int rc = file_encryption::encrypt_file_stream(source_file, destination_file,
                                                                  password);
                    if (rc == 0) {
                        enc_logger::log.debug("Encryption successful.");
                    } else {
                        enc_logger::log.error("Encryption failed.");
                    }
                } else {
                    enc_logger::log.debug(
                        "Renamed file does not match watch target. No action taken.");
                }
            }

            if (notify_info->NextEntryOffset == 0)
                break;
            notify_info = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(
                reinterpret_cast<std::byte*>(notify_info) + notify_info->NextEntryOffset);
        } while (true);
    }

    enc_logger::log.debug("Closing Watch Directory Handle");
    CloseHandle(watch_directory_handle);
}

} // namespace file_watcher
