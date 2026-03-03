#include <array>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
/////////////////////////////// WIN API's
#include <fileapi.h>
#include <windows.h>
////////////////////////////////// OPENSSL API's
#include <openssl/err.h>
#include <openssl/evp.h>
////////////////////////////////// Local includes
#include "file_encryption.h"
#include "password_prompt.h"

std::string to_utf8(const std::wstring& wstr) {
    if (wstr.empty())
        return {};
    int size_needed =
        WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string str(size_needed - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, str.data(), size_needed, nullptr, nullptr);
    return str;
}

std::wstring to_wstring(const std::string& str) {
    if (str.empty())
        return {};
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstr(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstr[0], size_needed);
    return wstr;
}

static std::optional<HANDLE> open_directory_handle(const std::wstring& directory) {
    HANDLE dirHandle = CreateFileW(directory.c_str(), FILE_LIST_DIRECTORY,
                                   FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                                   OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);

    if (dirHandle == INVALID_HANDLE_VALUE) {
        return std::nullopt;
    }

    return dirHandle;
}
/**
 * For debugging to be removed
 */
void PrintAction(DWORD action, const std::wstring& filename_w) {
    std::string filename = to_utf8(filename_w);
    // if (filename != target_file)
    //   return;

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
            // target_file = filename;
            break;
        default:
            std::cout << "Unknown Action: " << filename << "\n";
            break;
    }
}

static void watch_directory(const std::string& source_path, const std::string& dest_path,
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

    std::array<char, 2048> buffer;
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

            PrintAction(notify_info->Action, filename_wide);

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

    std::cout << "\n Closing Handle" << std::endl;
    CloseHandle(watch_directory_handle);
}

int main(int argc, char** argv) {
    OpenSSL_add_all_algorithms();
    ERR_load_crypto_strings();

    std::cout << "OpenSSL version: " << OpenSSL_version(OPENSSL_VERSION) << "\n";

    if (argc == 4 && std::string(argv[1]) == "--watch") {
        const std::string SRC = argv[2];
        const std::string DST = argv[3];
        std::string pwd = password_prompt::prompt_password("Enter password for watch mode");
        if (pwd.empty()) {
            std::cerr << "Empty password not allowed\n";
            return 1;
        }

        // Initial encryption
        std::cout << "Performing initial encryption...\n";
        int rc = file_encryption::encrypt_file_stream(SRC, DST, pwd);
        if (rc == 0) {
            std::cout << "Initial encryption completed: " << DST << "\n";
        } else {
            std::cerr << "Initial encryption failed.\n";
            // We can still start watching, maybe the file will be created later
        }

        watch_directory(SRC, DST, pwd);

        std::cout << "Exiting watch mode.\n";
        EVP_cleanup();
        ERR_free_strings();
        return 0;

    } else if (argc == 4 && std::string(argv[1]) == "--file") {
        std::string src = argv[2];
        std::string dst = argv[3];
        std::string pwd = password_prompt::prompt_password("Enter password");
        if (pwd.empty()) {
            std::cerr << "Empty password not allowed\n";
            return 1;
        }
        int rc = file_encryption::encrypt_file_stream(src, dst, pwd);
        EVP_cleanup();
        ERR_free_strings();
        if (rc == 0)
            std::cout << "Encryption completed: " << dst << "\n";
        return rc;
    } else {
        std::cout << "Usage for one-time encryption: encrypter --file "
                     "<source_file> <destination_file>\n";
        std::cout << "Usage for watch mode: encrypter --watch <source_file> "
                     "<destination_file>\n";
    }

    return 0;
}
