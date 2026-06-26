#include "file_watcher.h"

#include <array>
#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

#include <fileapi.h>
#include <windows.h>

#include "file_encryption.h"
#include "logger.h"

using namespace logger;

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
            log.info("[PRINT_ACTION]File Created: {}", filename);
            break;
        case FILE_ACTION_REMOVED:
            log.info("[PRINT_ACTION]File Deleted: {}", filename);
            break;
        case FILE_ACTION_MODIFIED:
            log.info("[PRINT_ACTION]File Modified: {}", filename);
            break;
        case FILE_ACTION_RENAMED_OLD_NAME:
            log.info("[PRINT_ACTION]File Renamed (Old Name): {}", filename);
            break;
        case FILE_ACTION_RENAMED_NEW_NAME:
            log.info("[PRINT_ACTION]File Renamed (New Name): {}", filename);
            break;
        default:
            log.info("[PRINT_ACTION]Unknown Action: {}", filename);
            break;
    }
}

} // anonymous namespace

namespace file_watcher {

void watch_directory(std::string& source_path, const std::string& dest_path,
                     const std::string& password) {
    std::filesystem::path source_path_fs = source_path;
    std::wstring directory_to_watch = source_path_fs.parent_path().wstring();
    std::wstring file_to_watch = source_path_fs.filename().wstring();

    auto directory_handle_opt = open_directory_handle(directory_to_watch);
    if (!directory_handle_opt.has_value()) {
        log.error("Failed to open directory for watching. Error: {}", GetLastError());
        return;
    }
    HANDLE watch_directory_handle = directory_handle_opt.value();
    log.info("Watching for changes in {}", to_utf8(directory_to_watch));

    std::array<char, BUFFER_SIZE> buffer;
    DWORD bytes_returned;

    while (ReadDirectoryChangesW(watch_directory_handle, buffer.data(),
                                 static_cast<DWORD>(buffer.size()),
                                 FALSE, // Don't watch subdirectories
                                 FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_FILE_NAME,
                                 &bytes_returned, nullptr, nullptr)) {
        FILE_NOTIFY_INFORMATION* notify_info =
            reinterpret_cast<FILE_NOTIFY_INFORMATION*>(buffer.data());

        std::optional<std::wstring> rename_old_name;

        do {
            std::wstring filename_wide(notify_info->FileName,
                                       notify_info->FileNameLength / sizeof(WCHAR));

            print_action(notify_info->Action, filename_wide);

            if (notify_info->Action == FILE_ACTION_MODIFIED && filename_wide == file_to_watch) {
                log.info("File Modified: {}", to_utf8(filename_wide));
                int rc = file_encryption::encrypt_file_stream(source_path, dest_path, password);
                if (rc == 0) {
                    log.info("Encryption successful of Modified file {}", to_utf8(filename_wide));
                } else {
                    log.error("Encryption failed of Modified file {}", to_utf8(filename_wide));
                }

            } else if (notify_info->Action == FILE_ACTION_RENAMED_OLD_NAME &&
                       filename_wide == file_to_watch) {
                log.info("File Renamed from: {}", to_utf8(filename_wide));
                rename_old_name = filename_wide;

            } else if (notify_info->Action == FILE_ACTION_RENAMED_NEW_NAME) {
                log.info("File Renamed to: {}", to_utf8(filename_wide));

                if (rename_old_name && *rename_old_name == file_to_watch) {
                    log.info("Renamed file matches previous watch target. Updating source path and "
                             "re-encrypting...");
                    log.info("Old Filename: {}, New Filename: {}, Source path: {}",
                             to_utf8(*rename_old_name), to_utf8(filename_wide),
                             source_path_fs.string());

                    source_path_fs.replace_filename(filename_wide);
                    log.info("Updated source path: {}", source_path_fs.string());

                    source_path = source_path_fs.string();
                    file_to_watch = filename_wide;
                    rename_old_name.reset();

                    log.info("Renamed file matches watch target. Updated source path to {}",
                             to_utf8(file_to_watch));
                    int rc = file_encryption::encrypt_file_stream(source_path, dest_path, password);
                    if (rc == 0) {
                        log.info("Encryption successful.");
                    } else {
                        log.error("Encryption failed.");
                    }
                } else {
                    log.info("Renamed file does not match watch target. No action taken.");
                }
            }

            if (notify_info->NextEntryOffset == 0)
                break;
            notify_info = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(
                reinterpret_cast<std::byte*>(notify_info) + notify_info->NextEntryOffset);
        } while (true);
    }

    log.info("\nClosing Handle");
    CloseHandle(watch_directory_handle);
}

} // namespace file_watcher
