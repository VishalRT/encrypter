#include <filesystem>
#include <iostream>
#include <string>
/////////////////////////////// WIN API's

////////////////////////////////// OPENSSL API's
#include <openssl/err.h>
#include <openssl/evp.h>
////////////////////////////////// Local includes
#include "file_encryption.h"
#include "file_watcher.h"
#include "logger.h"
#include "password_prompt.h"

using namespace logger;

int main(int argc, char** argv) {
    OpenSSL_add_all_algorithms();
    ERR_load_crypto_strings();

    log.info("OpenSSL version: {}", OpenSSL_version(OPENSSL_VERSION));

    if (argc == 4 && std::string(argv[1]) == "--watch") {
        // Keeping src as non-const since we may want to update it on rename events in watch mode
        std::string src = argv[2];
        const std::string DST = argv[3];
        std::string pwd = password_prompt::prompt_password("Enter password for watch mode");
        if (pwd.empty()) {
            std::cerr << "Empty password not allowed\n";
            return 1;
        }

        log.info("Performing initial encryption");
        int rc = file_encryption::encrypt_file_stream(src, DST, pwd);
        if (rc == 0) {
            std::cout << "Initial encryption completed: " << DST << "\n";
        } else {
            std::cerr << "Initial encryption failed.\n";
            // We can still start watching, maybe the file will be created later
        }

        file_watcher::watch_directory(src, DST, pwd);

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
        // TODO: This is just for testing to be used on service
        if (rc == 0) {
            std::cout << "Encryption completed: " << dst << "\n";

            // Decrypt to decrypted.txt for verification
            std::filesystem::path src_path(src);
            std::filesystem::path decrypted_path = src_path.parent_path() / "decrypted.txt";
            int decrypt_rc =
                file_encryption::decrypt_file_stream(dst, decrypted_path.string(), pwd);
            if (decrypt_rc == 0) {
                std::cout << "Decryption completed: " << decrypted_path.string() << "\n";
            } else {
                std::cerr << "Decryption failed.\n" << decrypt_rc;
            }
        }
        return rc;
    } else {
        std::cout << "Usage for one-time encryption: encrypter --file "
                     "<source_file> <destination_file>\n";
        std::cout << "Usage for watch mode: encrypter --watch <source_file> "
                     "<destination_file>\n";
    }

    return 0;
}
