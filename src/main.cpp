#include <filesystem>
#include <string>
/////////////////////////////// WIN API's

////////////////////////////////// OPENSSL API's
#include <openssl/err.h>
#include <openssl/evp.h>
////////////////////////////////// Project includes
#include "file_encryption.h"
#include "file_watcher.h"
#include "logger.h"
#include "password_prompt.h"

using namespace logger;

int main(int argc, char** argv) {
	OpenSSL_add_all_algorithms();
	ERR_load_crypto_strings();

	enc_logger::log.debug("C++ Standard version: {}", __cplusplus);
	enc_logger::log.info("OpenSSL version: {}", OpenSSL_version(OPENSSL_VERSION));

	if (argc == 4 && std::string(argv[1]) == "--watch") {
		// Keeping src as non-const since we may want to update it on rename events
		// in watch mode
		std::string src = argv[2];
		const std::string DST = argv[3];
		std::string pwd = password_prompt::prompt_password("Enter password for watch mode");
		if (pwd.empty()) {
			enc_logger::log.error(
				"Empty password not allowed. Hello this is supposed to be secure ?");
			return 1;
		}

		enc_logger::log.debug("Performing initial encryption");
		int rc = file_encryption::encrypt_file_stream(src, DST, pwd);
		if (rc == 0) {
			enc_logger::log.debug("Initial encryption completed: {}", DST);
		} else {
			enc_logger::log.warning("Initial encryption failed. Starting watcher anyway");
			// We can still start watching, maybe the file will be created later
			// or we might need to chage this if required later to create it the file
			// ?
		}

		file_watcher::watch_directory(src, DST, pwd);

		enc_logger::log.debug("Exiting watch mode");
		EVP_cleanup();
		ERR_free_strings();
		return 0;

	} else if (argc == 4 && std::string(argv[1]) == "--file") {
		std::string src = argv[2];
		std::string dst = argv[3];
		std::string pwd = password_prompt::prompt_password("Enter password");
		if (pwd.empty()) {
			enc_logger::log.error("Empty password not allowed");
			return 1;
		}
		int rc = file_encryption::encrypt_file_stream(src, dst, pwd);
		EVP_cleanup();
		ERR_free_strings();
		// TODO: This is just for testing to be used on service
		if (rc == 0) {
			enc_logger::log.debug("Encryption completed: {}", dst);

			// Decrypt to decrypted.txt for verification
			std::filesystem::path src_path(src);
			std::filesystem::path decrypted_path = src_path.parent_path() / "decrypted.txt";
			int decrypt_rc =
				file_encryption::decrypt_file_stream(dst, decrypted_path.string(), pwd);
			if (decrypt_rc == 0) {
				enc_logger::log.debug("Decryption completed: {}", decrypted_path.string());
			} else {
				enc_logger::log.error("Decryption failed with code: {}", decrypt_rc);
			}
		}
		return rc;
	} else {
		enc_logger::log.info("Usage for one-time encryption: encrypter --file <source_file> "
							 "<destination_file>");
		enc_logger::log.info("Usage for watch mode: encrypter --watch <source_file> "
							 "<destination_file>");
	}

	return 0;
}
