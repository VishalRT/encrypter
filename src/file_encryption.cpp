#include "file_encryption.h"

#include <cstddef>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

namespace file_encryption {

namespace {
constexpr size_t SALT_SIZE = 16;
constexpr size_t KEY_SIZE = 32; // AES-256
constexpr size_t IV_SIZE = 16;
constexpr int PBKDF2_ITERS = 100000;
const char MAGIC_HEADER[] = "ENCRYPv1"; // 8 bytes

void print_openssl_error() {
    unsigned long err = ERR_get_error();
    if (err) {
        char buf[256];
        ERR_error_string_n(err, buf, sizeof(buf));
        std::cerr << "OpenSSL error: " << buf << "\n";
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
        std::cerr << "Failed to open source file: " << input_path << "\n";
        return 1;
    }

    std::ofstream outfile(output_path, std::ios::binary);
    if (!outfile) {
        std::cerr << "Failed to open destination file: " << output_path << "\n";
        return 1;
    }

    std::vector<unsigned char> salt(SALT_SIZE);
    if (RAND_bytes(salt.data(), static_cast<int>(salt.size())) != 1) {
        std::cerr << "Failed to generate salt\n";
        print_openssl_error();
        return 1;
    }

    std::vector<unsigned char> key, iv;
    if (!derive_key_iv(password, salt, key, iv)) {
        std::cerr << "Key derivation failed\n";
        return 1;
    }

    // write header: MAGIC + salt
    outfile.write(MAGIC_HEADER, sizeof(MAGIC_HEADER) - 1);
    outfile.write(reinterpret_cast<const char*>(salt.data()),
                  static_cast<std::streamsize>(salt.size()));

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        std::cerr << "EVP_CIPHER_CTX_new failed\n";
        return 1;
    }
    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key.data(), iv.data()) != 1) {
        std::cerr << "EVP_EncryptInit_ex failed\n";
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
                std::cerr << "EVP_EncryptUpdate failed\n";
                print_openssl_error();
                EVP_CIPHER_CTX_free(ctx);
                return 1;
            }
            outfile.write(reinterpret_cast<const char*>(outbuf.data()), outlen);
        }
    }

    int tmplen = 0;
    if (EVP_EncryptFinal_ex(ctx, outbuf.data(), &tmplen) != 1) {
        std::cerr << "EVP_EncryptFinal_ex failed\n";
        print_openssl_error();
        EVP_CIPHER_CTX_free(ctx);
        return 1;
    }
    if (tmplen > 0)
        outfile.write(reinterpret_cast<const char*>(outbuf.data()), tmplen);
    EVP_CIPHER_CTX_free(ctx);
    return 0;
}

} // namespace file_encryption
