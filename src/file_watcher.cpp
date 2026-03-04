#include "file_watcher.h"

#include <array>
#include <cstddef>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>

#include <fileapi.h>
#include <windows.h>

#include "file_encryption.h"

namespace {

// Constants in UPPER_CASE (NL.17)
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

/// Opens a directory handle for monitoring file system changes
/// Uses Windows CreateFileW API with Directory-specific flags for change notifications.
/// @param directory Path to directory to watch (wide-character string)
/// @return std::optional<HANDLE> - valid handle on success, nullopt if CreateFileW fails
std::optional<HANDLE> open_directory_handle(const std::wstring& directory) {
    HANDLE dir_handle = CreateFileW(directory.c_str(), FILE_LIST_DIRECTORY,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                                    OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);

    if (dir_handle == INVALID_HANDLE_VALUE) {
        return std::nullopt;
    }

    return dir_handle;
}

/// To be removed Logs file system action to stdout (for debugging and user feedback)
/// @param action FILE_ACTION_* constant from Windows API (FILE_ACTION_ADDED, etc.)
/// @param filename_w Wide-character filename that triggered the action
void print_action(DWORD action, const std::wstring& filename_w) {
    std::string filename = to_utf8(filename_w);

    switch (action) {
        case FILE_ACTION_ADDED:
            std::cout << "File Created: " << filename << "\n";
            break;
        case FILE_ACTION_REMOVED:
            std::cout << "File Deleted: " << filename << "\n";
            break;
        case FILE_ACTION_MODIFIED:
            std::cout << "File Modified: " << filename << "\n";
            break;
        case FILE_ACTION_RENAMED_OLD_NAME:
            std::cout << "File Renamed (Old Name): " << filename << "\n";
            break;
        case FILE_ACTION_RENAMED_NEW_NAME:
            std::cout << "File Renamed (New Name): " << filename << "\n";
            break;
        default:
            std::cout << "Unknown Action: " << filename << "\n";
            break;
    }
}

} // anonymous namespace

namespace file_watcher {

void watch_directory(const std::string& source_path, const std::string& dest_path,
                     const std::string& password) {
    std::filesystem::path source_path_fs = source_path;
    std::wstring directory_to_watch = source_path_fs.parent_path().wstring();
    std::wstring file_to_watch = source_path_fs.filename().wstring();

    auto directory_handle_opt = open_directory_handle(directory_to_watch);
    if (!directory_handle_opt.has_value()) {
        std::cerr << "Failed to open directory for watching. Error: " << GetLastError() << "\n";
        return;
    }
    HANDLE watch_directory_handle = directory_handle_opt.value();
    std::cout << "Watching for changes in " << to_utf8(directory_to_watch) << "\n";

    std::array<char, BUFFER_SIZE> buffer;
    DWORD bytes_returned;

    while (ReadDirectoryChangesW(watch_directory_handle, buffer.data(),
                                 static_cast<DWORD>(buffer.size()),
                                 FALSE, // Don't watch subdirectories
                                 FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_FILE_NAME,
                                 &bytes_returned, nullptr, nullptr)) {
        FILE_NOTIFY_INFORMATION* notify_info =
            reinterpret_cast<FILE_NOTIFY_INFORMATION*>(buffer.data());

        do {
            std::wstring filename_wide(notify_info->FileName,
                                       notify_info->FileNameLength / sizeof(WCHAR));

            print_action(notify_info->Action, filename_wide);

            if (notify_info->Action == FILE_ACTION_MODIFIED && filename_wide == file_to_watch) {
                std::cout << "File Modified: " << to_utf8(filename_wide) << ". Re-encrypting...\n";
                int rc = file_encryption::encrypt_file_stream(source_path, dest_path, password);
                if (rc == 0) {
                    std::cout << "Encryption successful.\n";
                } else {
                    std::cerr << "Encryption failed.\n";
                }
            } else if (notify_info->Action == FILE_ACTION_RENAMED_NEW_NAME &&
                       filename_wide == file_to_watch) {
                std::cout << "File Renamed to: " << to_utf8(filename_wide)
                          << ". Re-encrypting...\n";
                int rc = file_encryption::encrypt_file_stream(source_path, dest_path, password);
                if (rc == 0) {
                    std::cout << "Encryption successful.\n";
                } else {
                    std::cerr << "Encryption failed.\n";
                }
            }

            if (notify_info->NextEntryOffset == 0)
                break;
            notify_info = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(
                reinterpret_cast<std::byte*>(notify_info) + notify_info->NextEntryOffset);
        } while (true);
    }

    std::cout << "\nClosing Handle\n";
    CloseHandle(watch_directory_handle);
}

} // namespace file_watcher
