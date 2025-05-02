#include <array>
#include <fileapi.h>
#include <iostream>
#include <optional>
#include <string>
#include <windows.h>
#include <openssl/evp.h>

std::string to_utf8(const std::wstring &wstr)
{
  if (wstr.empty())
    return {};
  int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr,
                                        0, nullptr, nullptr);
  std::string str(size_needed - 1, 0);
  WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, str.data(), size_needed,
                      nullptr, nullptr);
  return str;
}

std::string_view target_file = "test.txt";

void PrintAction(DWORD action, const std::wstring &filename_w)
{
  std::string filename = to_utf8(filename_w);
  if (filename != target_file)
    return;

  switch (action)
  {
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

std::optional<HANDLE> OpenDirectoryHandle(const std::wstring &directory)
{
  HANDLE dirHandle = CreateFileW(
      directory.c_str(), FILE_LIST_DIRECTORY,
      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
      nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);

  if (dirHandle == INVALID_HANDLE_VALUE)
  {
    return std::nullopt;
  }

  return dirHandle;
}

void WatchDirectory(const std::wstring &directory)
{
  auto hDirOpt = OpenDirectoryHandle(directory);
  if (!hDirOpt.has_value())
  {
    std::cout << "Failed to open directory. Error: " << GetLastError() << "\n";
    return;
  }

  std::cout << "Handle received from CreateFileW\n";
  HANDLE watchDirHandle = hDirOpt.value();

  std::array<char, 1024> buffer;
  DWORD bytesReturned;

  while (ReadDirectoryChangesW(
      watchDirHandle, buffer.data(), static_cast<DWORD>(buffer.size()),
      FALSE, // Don't watch subdirectories
      FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_SIZE |
          FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_ATTRIBUTES |
          FILE_NOTIFY_CHANGE_CREATION,
      &bytesReturned, nullptr, nullptr))
  {

    FILE_NOTIFY_INFORMATION *pNotify =
        reinterpret_cast<FILE_NOTIFY_INFORMATION *>(buffer.data());

    std::wcout  << "ReadDirectoryChangesW received change notification\n"
                << "FileName: ";

    for(WCHAR c : pNotify->FileName)
      std::wcout << c;

      std::wcout << "FileNameLength: " << pNotify->FileNameLength << '\n'
                << "FileAction:  " << pNotify->Action << '\n';

    do
    {
      std::wstring filename_w(pNotify->FileName,
                              pNotify->FileNameLength / sizeof(WCHAR));
      PrintAction(pNotify->Action, filename_w);

      if (pNotify->NextEntryOffset == 0)
        break;
      pNotify = reinterpret_cast<FILE_NOTIFY_INFORMATION *>(
          reinterpret_cast<std::byte *>(pNotify) + pNotify->NextEntryOffset);
    } while (true);
  }

  std::cout << "Directory reading complete, closing handle\n";
  CloseHandle(watchDirHandle);
}

int main()
{
  OpenSSL_add_all_algorithms();
  std::cout << "OpenSSL version: " << OpenSSL_version(OPENSSL_VERSION) << "\n";

  constexpr std::wstring_view directory =
      L"C:\\Users\\Vishal\\Documents\\workspace\\encrypter\\test\\files";

  std::cout << "Watching file: " << target_file << " in "
            << to_utf8(std::wstring(directory)) << "\n";

  WatchDirectory(std::wstring(directory));
}
