#include <expected>
#include <iostream>
#include <string>
#include <windows.h>

// Helper function to convert std::wstring (UTF-16) to std::string (UTF-8)
std::string to_utf8(const std::wstring &wstr) {
  if (wstr.empty())
    return {};
  int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr,
                                        0, nullptr, nullptr);
  std::string str(size_needed - 1, 0);
  WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, str.data(), size_needed,
                      nullptr, nullptr);
  return str;
}

constexpr std::string_view TARGET_FILE = "test.txt";

void PrintAction(DWORD action, const std::wstring &filename_w) {
  std::string filename = to_utf8(filename_w);
  if (filename != TARGET_FILE)
    return; // Ignore other files

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
  }
}

std::expected<HANDLE, DWORD>
OpenDirectoryHandle(const std::wstring &directory) {
  HANDLE hDir =
      CreateFileW(directory.c_str(), FILE_LIST_DIRECTORY,
                  FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                  nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);

  if (hDir == INVALID_HANDLE_VALUE) {
    return std::unexpected(GetLastError());
  }
  return hDir;
}

void WatchDirectory(const std::wstring &directory) {
  auto hDirExpected = OpenDirectoryHandle(directory);
  if (!hDirExpected) {
    std::cout << "Failed to open directory. Error: " << hDirExpected.error()
              << "\n";
    return;
  }

  HANDLE hDir = hDirExpected.value();
  /*std::array<char, 1024> buffer;*/
  /*DWORD bytesReturned;*/
  /**/
  /*while (ReadDirectoryChangesW(*/
  /*    hDir, buffer.data(), static_cast<DWORD>(buffer.size()),*/
  /*    FALSE, // Don't watch subdirectories*/
  /*    FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_SIZE |*/
  /*        FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_ATTRIBUTES,*/
  /*    &bytesReturned, nullptr, nullptr)) {*/
  /**/
  /*  FILE_NOTIFY_INFORMATION *pNotify =*/
  /*      reinterpret_cast<FILE_NOTIFY_INFORMATION *>(buffer.data());*/
  /**/
  /*  do {*/
  /*    std::wstring filename_w(pNotify->FileName,*/
  /*                            pNotify->FileNameLength / sizeof(WCHAR));*/
  /*    PrintAction(pNotify->Action, filename_w);*/
  /**/
  /*    if (pNotify->NextEntryOffset == 0)*/
  /*      break;*/
  /*    pNotify = reinterpret_cast<FILE_NOTIFY_INFORMATION *>(*/
  /*        reinterpret_cast<std::byte *>(pNotify) +
   * pNotify->NextEntryOffset);*/
  /*  } while (true);*/
  /*}*/

  std::cout << "Directory Reading successfull closing handle\n";
  CloseHandle(hDir);
}

int main() {
  constexpr std::wstring_view directory =
      L"C:\\Users\\Vishal\\Documents\\workspace\\encrypter\\test";

  std::cout << "Watching file: " << TARGET_FILE << " in "
            << to_utf8(std::wstring(directory)) << "\n";

  WatchDirectory(std::wstring(directory));
}
