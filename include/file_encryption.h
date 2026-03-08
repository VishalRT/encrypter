#pragma once

#include <string>

namespace file_encryption {

/**
 * Encrypts a file using AES-256-CBC with password-derived key (PBKDF2).
 *
 * File format: [MAGIC_HEADER: 11 bytes][SALT: 16 bytes][ENCRYPTED_DATA]
 * Key derivation: PBKDF2-HMAC-SHA256 with 100,000 iterations
 * Cipher: AES-256-CBC
 *
 * @param input_path    Path to source file to encrypt
 * @param output_path   Path to destination encrypted file
 * @param password      Password to derive encryption key from
 *
 * @return 0 on success, 1 on failure (errors logged to stderr)
 *
 * @note No authentication/integrity protection. For sensitive data, consider
 *       switching to authenticated encryption (AES-GCM) in future versions.
 */
int encrypt_file_stream(const std::string& input_path, const std::string& output_path,
                        const std::string& password);

/**
 * Decrypts a file encrypted by encrypt_file_stream() using the same password.
 *
 * @param input_path    Path to encrypted file
 * @param output_path   Path to destination decrypted file
 * @param password      Password to derive decryption key from
 * @return 0 on success, 1 on failure (errors logged to stderr)
 * @note Expects the same file format as encrypt_file_stream() with ENCRYPTERV1 MAGIC_HEADER and
 * SALT
 * @note No authentication/integrity protection. For sensitive data, consider    switching to
 * authenticated encryption (AES-GCM) in future versions.
 */
int decrypt_file_stream(const std::string& input_path, const std::string& output_path,
                        const std::string& password);

} // namespace file_encryption
