#pragma once

#include <string>

namespace password_prompt {

/**
 * Prompts the user for a password with hidden input (echo disabled).
 * Windows-specific implementation using _getch() for secure input.
 *
 * @param prompt_message The message to display when prompting for password
 * @return The entered password as std::string
 *
 * @note Input is hidden with asterisks. Supports backspace for correction.
 *       Ctrl+C aborts the prompt and returns an empty string.
 */
std::string prompt_password(const char* prompt_message);

} // namespace password_prompt
