#include <filesystem>
#include <string>
////////////////////////////////// third party
#include <CLI/CLI.hpp>
#include <openssl/err.h>
#include <openssl/evp.h>
////////////////////////////////// Project includes
#include "CLI/CLI.hpp"
#include "file_encryption.h"
#include "file_watcher.h"
#include "logger.h"
#include "password_prompt.h"

int main(int argc, char** argv) {
	std::string src_file;
	std::string dst_file;

	CLI::App app{"Encrypter"};

	CLI::App* file_mode = app.add_subcommand("file", "One time file encryption");
	file_mode->add_option("-s, --source", src_file, "Source file path to be encrypted")
		->required(true);
	file_mode->add_option("-d, --destination", dst_file,
						  "Destination file path to store encrypted data");

	CLI::App* watch_mode = app.add_subcommand("watch", "Directory file watcher");
	watch_mode->add_option("-s, --source", src_file, "Source directory path to watch")
		->required(true);
	CLI::Option* destination_option = watch_mode->add_option(
		"-d, --destination", dst_file, "Destination directory path to store encrypted data");

	app.require_subcommand(1);

	CLI11_PARSE(app, argc, argv);

	// Update destination if user hasn't passed it, prepending "enc_" to the filename
	if (destination_option->count() == 0) {
		dst_file = (std::filesystem::path(src_file).parent_path() /
					("enc_" + std::filesystem::path(src_file).filename().string()))
					   .string();
	}

	std::string pwd = password_prompt::prompt_password("Enter password for watch mode");
	if (pwd.empty()) {
		enc_logger::log.error(
			"Empty password not allowed. Hello this is supposed to be secure ???");
		return 1;
	}

	OpenSSL_add_all_algorithms();
	ERR_load_crypto_strings();

	enc_logger::log.debug("C++ Standard version: {}", __cplusplus);
	enc_logger::log.info("OpenSSL version: {}", OpenSSL_version(OPENSSL_VERSION));

	if (app.got_subcommand(file_mode)) {
		enc_logger::log.debug("File encrypter mode \n\tsource path: {}, \n\tdestination path: {}\n",
							  src_file, dst_file);

		int rc = file_encryption::encrypt_file_stream(src_file, dst_file, pwd);

		// TODO: This is just for testing to be used on service
		if (rc == 0) {
			enc_logger::log.debug("Encryption completed: {}", dst_file);

			// Decrypt to decrypted.txt for verification
			std::filesystem::path src_path(src_file);
			std::filesystem::path decrypted_path =
				src_path.parent_path() / ("dec_" + src_path.filename().string());

			enc_logger::log.debug("Decrypting Path resolved is : {}", decrypted_path.string());
			int decrypt_rc =
				file_encryption::decrypt_file_stream(dst_file, decrypted_path.string(), pwd);
			if (decrypt_rc == 0) {
				enc_logger::log.debug("Decryption completed: {}", decrypted_path.string());
			} else {
				enc_logger::log.error("Decryption failed with code: {}", decrypt_rc);
			}
		}

	} else if (app.got_subcommand(watch_mode)) {
		enc_logger::log.debug("Watch mode \n\tsource path: {}, \n\tdestination path: {}", src_file,
							  dst_file);

		enc_logger::log.debug("Performing initial encryption");
		int rc = file_encryption::encrypt_file_stream(src_file, dst_file, pwd);
		if (rc == 0) {
			enc_logger::log.debug("Initial encryption completed: {}", dst_file);
		} else {
			enc_logger::log.warning("Initial encryption failed. Starting watcher anyway");
			// We can still start watching, maybe the file will be created later
			// or we might need to chage this if required later to create it the file
			// ?
		}

		file_watcher::watch_directory(src_file, dst_file, pwd);

		enc_logger::log.debug("Exiting watch mode");
	}

	EVP_cleanup();
	ERR_free_strings();
	// enc_logger::log.info("Usage for one-time encryption: encrypter --file <source_file> "
	// 					 "<destination_file>");
	// enc_logger::log.info("Usage for watch mode: encrypter --watch <source_file> "
	// 					 "<destination_file>");

	return 0;
}
