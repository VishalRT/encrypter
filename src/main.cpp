#include <array>
#include <fileapi.h>
#include <iostream>
#include <optional>
#include <string>
#include <windows.h>
#include <conio.h>
#include <fstream>
#include <vector>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/rand.h>

namespace {
constexpr size_t SALT_SIZE = 16;
constexpr size_t KEY_SIZE = 32; // AES-256
constexpr size_t IV_SIZE = 16;
constexpr int PBKDF2_ITERS = 100000;
const char MAGIC[] = "ENCRYPv1"; // 8 bytes
}

static void print_openssl_error()
{
  unsigned long err = ERR_get_error();
  if (err) {
    char buf[256];
    ERR_error_string_n(err, buf, sizeof(buf));
    std::cerr << "OpenSSL error: " << buf << "\n";
  }
}

static std::string prompt_password(const char *prompt)
{
  std::cout << prompt << " (input hidden): ";
    std::string pwd;
    int ch;
  while ((ch = _getch()) != '\r') {
    if (ch == '\b') {
      if (!pwd.empty()) { pwd.pop_back(); std::cout << "\b \b"; }
    // Ctrl+C (ASCII 3) pressed: handle user interruption
        } else if (ch == 3) {
          std::cout << "\n"; exit(1);
        } else {
          pwd.push_back(static_cast<char>(ch)); std::cout << '*';
        }
  }
  std::cout << "\n";
  return pwd;
}

static bool derive_key_iv(const std::string &password, const std::vector<unsigned char> &salt,
                          std::vector<unsigned char> &key, std::vector<unsigned char> &iv)
{
  std::vector<unsigned char> buf(KEY_SIZE + IV_SIZE);
  const EVP_MD *md = EVP_sha256();
  if (!PKCS5_PBKDF2_HMAC(password.c_str(), static_cast<int>(password.size()),
                         salt.data(), static_cast<int>(salt.size()), PBKDF2_ITERS,
                         md, static_cast<int>(buf.size()), buf.data())) {
    print_openssl_error();
    return false;
  }
  key.assign(buf.begin(), buf.begin() + KEY_SIZE);
  iv.assign(buf.begin() + KEY_SIZE, buf.begin() + KEY_SIZE + IV_SIZE);
  return true;
}

static int encrypt_file_stream(const std::string &inpath, const std::string &outpath, const std::string &password)
{
  std::ifstream infile(inpath, std::ios::binary);
  if (!infile) { std::cerr << "Failed to open source file: " << inpath << "\n"; return 1; }
  
  std::ofstream outfile(outpath, std::ios::binary);
  if (!outfile) { std::cerr << "Failed to open destination file: " << outpath << "\n"; return 1; }

  std::vector<unsigned char> salt(SALT_SIZE);
  if (RAND_bytes(salt.data(), static_cast<int>(salt.size())) != 1) {
    std::cerr << "Failed to generate salt\n"; print_openssl_error(); return 1;
  }

  std::vector<unsigned char> key, iv;
  if (!derive_key_iv(password, salt, key, iv)) { std::cerr << "Key derivation failed\n"; return 1; }

  // write header: MAGIC + salt
  outfile.write(MAGIC, sizeof(MAGIC)-1);
  outfile.write(reinterpret_cast<const char *>(salt.data()), static_cast<std::streamsize>(salt.size()));

  EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
  if (!ctx) { std::cerr << "EVP_CIPHER_CTX_new failed\n"; return 1; }
  if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key.data(), iv.data()) != 1) {
    std::cerr << "EVP_EncryptInit_ex failed\n"; print_openssl_error(); EVP_CIPHER_CTX_free(ctx); return 1; }

  const size_t BUFSIZE = 4096;
  std::vector<unsigned char> inbuf(BUFSIZE);
  std::vector<unsigned char> outbuf(BUFSIZE + EVP_CIPHER_block_size(EVP_aes_256_cbc()));

  while (infile) {
    std::streamsize read_bytes = infile.gcount();
    infile.read(reinterpret_cast<char *>(inbuf.data()), static_cast<std::streamsize>(inbuf.size()));
    if (read_bytes > 0) {
      int outlen = 0;
      if (EVP_EncryptUpdate(ctx, outbuf.data(), &outlen, inbuf.data(), static_cast<int>(read_bytes)) != 1) {
        std::cerr << "EVP_EncryptUpdate failed\n"; print_openssl_error(); EVP_CIPHER_CTX_free(ctx); return 1; }
      outfile.write(reinterpret_cast<const char *>(outbuf.data()), outlen);
    }
  }

  int tmplen = 0;
  if (EVP_EncryptFinal_ex(ctx, outbuf.data(), &tmplen) != 1) { std::cerr << "EVP_EncryptFinal_ex failed\n"; print_openssl_error(); EVP_CIPHER_CTX_free(ctx); return 1; }
  if (tmplen > 0) outfile.write(reinterpret_cast<const char *>(outbuf.data()), tmplen);
  EVP_CIPHER_CTX_free(ctx);
  return 0;
}

int main(int argc, char **argv)
{
  OpenSSL_add_all_algorithms();
  ERR_load_crypto_strings();

  std::cout << "OpenSSL version: " << OpenSSL_version(OPENSSL_VERSION) << "\n";

  // If the user passed source and destination files, perform encryption using the new function
  if (argc == 3) {
    std::string src = argv[1];
    std::string dst = argv[2];
    std::string pwd = prompt_password("Enter password");
    if (pwd.empty()) { std::cerr << "Empty password not allowed\n"; return 1; }
    int rc = encrypt_file_stream(src, dst, pwd);
    EVP_cleanup();
    ERR_free_strings();
    if (rc == 0) std::cout << "Encryption completed: " << dst << "\n";
    return rc;
  } else {
    std::cout << "Usage: encrypter <source_file > <destination_file>\n";
  }

  /* Commenting Service for now. Need to keep it simple for start
  OpenSSL_add_all_algorithms();
  constexpr std::wstring_view directory =
      L"C:\\Users\\Vishal\\Documents\\workspace\\encrypter\\test\\files";

  std::cout << "Watching file: " << target_file << " in "
            << to_utf8(std::wstring(directory)) << "\n";

  // WatchDirectory(std::wstring(directory));
  */
  return 0;
}

////SERVICE LOGIC BELOW To be used later

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
  // if (filename != target_file)
  //   return;

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
    target_file = filename;
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

    for (DWORD i = 0; i < pNotify->FileNameLength / sizeof(WCHAR); ++i)
      std::wcout << pNotify->FileName[i];

      std::wcout << "\nFileNameLength: " << (pNotify->FileNameLength / sizeof(WCHAR)) << '\n'
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

