#include "password_prompt.h"

#include <cmath>
#include <conio.h>
#include <iostream>
#include <string>

#include "logger.h"

namespace password_prompt {

using namespace logger;

namespace {
constexpr int BACKSPACE_CHAR = '\b';
constexpr int CTRL_C_CHAR = 3;
constexpr int CARRIAGE_RETURN_CHAR = '\r';
constexpr char HIDDEN_INPUT_MASK = '*';
} // namespace

std::string prompt_password(const char* prompt_message) {
    logger::log.info("{} (input hidden)", prompt_message);
    std::cout.flush();

    std::string password_buffer;
    int character_input;

    // Windows-specific: _getch() reads unbuffered input without echo
    while ((character_input = _getch()) != CARRIAGE_RETURN_CHAR) {
        if (character_input == BACKSPACE_CHAR) {
            if (!password_buffer.empty()) {
                password_buffer.pop_back();
                std::cout << "\b \b";
                std::cout.flush();
            }
        } else if (character_input == CTRL_C_CHAR) {
            std::cout << "\n";
            password_buffer.clear();
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
