#pragma once

#include <string>

namespace file_watcher {

/**
 * Monitors a file for changes and re-encrypts on modification
 * Watches the directory containing source_path for FILE_ACTION_MODIFIED and
 * FILE_ACTION_RENAMED_NEW_NAME events. When the target file is modified or renamed,
 * it automatically triggers encryption via file_encryption::encrypt_file_stream().
 *
 * @param source_path Path to file being watched (UTF-8 std::string)
 * @param dest_path Path to encrypted output file (UTF-8 std::string)
 * @param password Encryption password for key derivation

 * @note This is a blocking function that runs an infinite loop until
 *       ReadDirectoryChangesW() fails or the handle is closed
 * @note Windows-specific implementation requiring <windows.h> at runtime
 * @note All system resources (HANDLE) are properly cleaned up via RAII on exit
 * @note Logs file system events and encryption status to stdout/stderr
 */
void watch_directory(const std::string& source_path, const std::string& dest_path,
                     const std::string& password);

} // namespace file_watcher
