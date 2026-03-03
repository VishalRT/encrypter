#include "password_prompt.h"

#include <conio.h>
#include <iostream>
#include <string>

namespace password_prompt {

namespace {
constexpr int BACKSPACE_CHAR = '\b';
constexpr int CTRL_C_CHAR = 3;
constexpr int CARRIAGE_RETURN_CHAR = '\r';
constexpr char HIDDEN_INPUT_MASK = '*';
} // anonymous namespace

std::string prompt_password(const char* prompt_message) {
    std::cout << prompt_message << " (input hidden): ";
    std::cout.flush();

    std::string password_buffer;
    int character_input;

    // Windows-specific: _getch() reads unbuffered input without echo
    while ((character_input = _getch()) != CARRIAGE_RETURN_CHAR) {
        if (character_input == BACKSPACE_CHAR) {
            if (!password_buffer.empty()) {
                password_buffer.pop_back();
                std::cout << "\b \b"; // why two backspaces and a space
                std::cout.flush();
            }
        } else if (character_input == CTRL_C_CHAR) {
            std::cout << "\n";
            return "";
        } else {
            password_buffer.push_back(static_cast<char>(character_input));
            std::cout << HIDDEN_INPUT_MASK;
            std::cout.flush();
        }
    }

    std::cout << "\n";
    return password_buffer;
}

} // namespace password_prompt
