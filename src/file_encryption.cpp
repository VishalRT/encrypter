#include "file_encryption.h"

#include <cstddef>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "logger.h"
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

using namespace logger;

namespace file_encryption {

namespace {
constexpr size_t SALT_SIZE = 16;
constexpr size_t KEY_SIZE = 32; // AES-256
constexpr size_t IV_SIZE = 16;
constexpr int PBKDF2_ITERS = 100000;
const char MAGIC_HEADER[] = "ENCRYPTERV1"; // 11 bytes

void print_openssl_error() {
	unsigned long err = ERR_get_error();
	if (err) {
		char buf[256];
		ERR_error_string_n(err, buf, sizeof(buf));
		log.error("OpenSSL error: {}", buf);
	}
}

bool derive_key_iv(const std::string& password, const std::vector<unsigned char>& salt,
				   std::vector<unsigned char>& key, std::vector<unsigned char>& iv) {
	std::vector<unsigned char> buf(KEY_SIZE + IV_SIZE);
	const EVP_MD* md = EVP_sha256();
	if (!PKCS5_PBKDF2_HMAC(password.c_str(), static_cast<int>(password.size()), salt.data(),
						   static_cast<int>(salt.size()), PBKDF2_ITERS, md,
						   static_cast<int>(buf.size()), buf.data())) {
		print_openssl_error();
		return false;
	}
	key.assign(buf.begin(), buf.begin() + KEY_SIZE);
	iv.assign(buf.begin() + KEY_SIZE, buf.begin() + KEY_SIZE + IV_SIZE);
	return true;
}
} // anonymous namespace

int encrypt_file_stream(const std::string& input_path, const std::string& output_path,
						const std::string& password) {
	std::ifstream infile(input_path, std::ios::binary);
	if (!infile) {
		log.error("Failed to open source file: {}", input_path);
		return 1;
	}

	std::ofstream outfile(output_path, std::ios::binary);
	if (!outfile) {
		log.error("Failed to open destination file: {}", output_path);
		return 1;
	}

	std::vector<unsigned char> salt(SALT_SIZE);
	if (RAND_bytes(salt.data(), static_cast<int>(salt.size())) != 1) {
		log.error("Failed to generate salt");
		print_openssl_error();
		return 1;
	}

	std::vector<unsigned char> key, iv;
	if (!derive_key_iv(password, salt, key, iv)) {
		log.error("Key derivation failed");
		return 1;
	}

	// write header: MAGIC + salt
	outfile.write(MAGIC_HEADER, sizeof(MAGIC_HEADER) - 1);
	outfile.write(reinterpret_cast<const char*>(salt.data()),
				  static_cast<std::streamsize>(salt.size()));

	EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
	if (!ctx) {
		log.error("EVP_CIPHER_CTX_new failed");
		return 1;
	}
	if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key.data(), iv.data()) != 1) {
		log.error("EVP_EncryptInit_ex failed");
		print_openssl_error();
		EVP_CIPHER_CTX_free(ctx);
		return 1;
	}

	const size_t BUFSIZE = 4096;
	std::vector<unsigned char> inbuf(BUFSIZE);
	std::vector<unsigned char> outbuf(BUFSIZE + EVP_CIPHER_block_size(EVP_aes_256_cbc()));

	while (infile) {
		infile.read(reinterpret_cast<char*>(inbuf.data()), BUFSIZE);
		std::streamsize read_bytes = infile.gcount();
		if (read_bytes > 0) {
			int outlen = 0;
			if (EVP_EncryptUpdate(ctx, outbuf.data(), &outlen, inbuf.data(),
								  static_cast<int>(read_bytes)) != 1) {
				log.error("EVP_EncryptUpdate failed");
				print_openssl_error();
				EVP_CIPHER_CTX_free(ctx);
				return 1;
			}
			outfile.write(reinterpret_cast<const char*>(outbuf.data()), outlen);
		}
	}

	int tmplen = 0;
	if (EVP_EncryptFinal_ex(ctx, outbuf.data(), &tmplen) != 1) {
		log.error("EVP_EncryptFinal_ex failed");
		print_openssl_error();
		EVP_CIPHER_CTX_free(ctx);
		return 1;
	}
	if (tmplen > 0)
		outfile.write(reinterpret_cast<const char*>(outbuf.data()), tmplen);
	EVP_CIPHER_CTX_free(ctx);
	return 0;
}

int decrypt_file_stream(const std::string& input_path, const std::string& output_path,
						const std::string& password) {
	std::ifstream infile(input_path, std::ios::in | std::ios::binary);
	if (!infile) {
		log.error("Failed to open encrypted file: {}", input_path);
		return 1;
	}

	std::filebuf file_buffer;
	if (!file_buffer.open(output_path, std::ios::out | std::ios::binary)) {
		log.error("Failed to open/create decrypted file: {}", output_path);
		return 1;
	}

	std::vector<unsigned char> magic_header(sizeof(MAGIC_HEADER) - 1);
	if (!infile.read(reinterpret_cast<char*>(magic_header.data()), sizeof(MAGIC_HEADER) - 1)) {
		log.error("Failed to read magic header from encrypted file");
		return 1;
	}

	// Verify magic header
	if (std::string(reinterpret_cast<char*>(magic_header.data()), sizeof(MAGIC_HEADER) - 1) !=
		MAGIC_HEADER) {
		log.error("Invalid file format - not an ENCRYPTERV1 encrypted file");
		return 1;
	}

	std::vector<unsigned char> salt(SALT_SIZE);
	if (!infile.read(reinterpret_cast<char*>(salt.data()), SALT_SIZE)) {
		log.error("Failed to read salt from encrypted file");
		return 1;
	}

	std::vector<unsigned char> key, iv;
	if (!derive_key_iv(password, salt, key, iv)) {
		log.error("Key derivation failed");
		return 1;
	}

	EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
	if (!ctx) {
		log.error("EVP_CIPHER_CTX_new failed");
		return 1;
	}
	if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key.data(), iv.data()) != 1) {
		log.error("EVP_DecryptInit_ex failed");
		print_openssl_error();
		EVP_CIPHER_CTX_free(ctx);
		return 1;
	}

	const size_t BUFSIZE = 4096;
	std::vector<unsigned char> inbuf(BUFSIZE);
	/** Adding extra 16 bytes for padding and cryptossl lib operations
	 * For more info:
	 * https://github.com/openssl/openssl/issues/26169
	 * https://www.openssl.org/docs/man3.0/man3/EVP_DecryptUpdate.html
	 */
	std::vector<unsigned char> decrypted_buffer(BUFSIZE + EVP_CIPHER_block_size(EVP_aes_256_cbc()));

	while (infile) {
		infile.read(reinterpret_cast<char*>(inbuf.data()), BUFSIZE);
		std::streamsize read_bytes = infile.gcount();
		if (read_bytes > 0) {
			int outlen = 0;
			if (EVP_DecryptUpdate(ctx, decrypted_buffer.data(), &outlen, inbuf.data(),
								  static_cast<int>(read_bytes)) != 1) {
				log.error("EVP_DecryptUpdate failed");
				print_openssl_error();
				EVP_CIPHER_CTX_free(ctx);
				return 1;
			}
			file_buffer.sputn(reinterpret_cast<const char*>(decrypted_buffer.data()), outlen);
		}
	}

	int tmplen = 0;
	if (EVP_DecryptFinal_ex(ctx, decrypted_buffer.data(), &tmplen) != 1) {
		log.error("EVP_DecryptFinal_ex failed");
		print_openssl_error();
		EVP_CIPHER_CTX_free(ctx);
		return 1;
	}
	if (tmplen > 0)
		file_buffer.sputn(reinterpret_cast<const char*>(decrypted_buffer.data()), tmplen);
	EVP_CIPHER_CTX_free(ctx);
	return 0;
}

} // namespace file_encryption
