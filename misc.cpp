#include "misc.h"

#include <stdexcept>
#include <sstream>

std::string telegrammToken() {
    const char* token = std::getenv("TELEGRAM_BOT_TOKEN");
    if (!token) {
        throw std::runtime_error("TELEGRAM_BOT_TOKEN env variable is not set");
    }
    return std::string(token);
}

std::unordered_set<std::int64_t> getAllowedChatIds()
{
    const char* env = std::getenv("CHAT_IDS");
    if (!env) {
        throw std::runtime_error("CHAT_IDS env variable is not set");
    }

    std::string value(env);
    std::unordered_set<std::int64_t> result;

    std::stringstream ss(value);
    std::string token;
    while (std::getline(ss, token, ','))
    {
        // Trim spaces
        auto start = token.find_first_not_of(" \t");
        auto end   = token.find_last_not_of(" \t");
        if (start == std::string::npos) {
            continue; // skip empty parts like ",,"
        }
        std::string trimmed = token.substr(start, end - start + 1);

        try {
            std::int64_t id = std::stoll(trimmed);
            result.insert(id);
        } catch (const std::exception& e) {
            throw std::runtime_error(
                        std::string("Invalid chat id in CHAT_IDS: '") +
                        trimmed + "' (" + e.what() + ")"
                        );
        }
    }

    if (result.empty()) {
        throw std::runtime_error("CHAT_IDS is set but no valid IDs were parsed");
    }

    return result;
}

bool isWhiteListed(std::int64_t chatId)
{
    return getAllowedChatIds().contains(chatId);
}
