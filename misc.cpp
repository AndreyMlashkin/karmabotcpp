#include "misc.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <sstream>
#include <cstdlib>

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

bool exchangeRatesEnabled()
{
    const char* env = std::getenv("ENABLE_EXCHANGE_RATES");
    if (!env) {
        return true;
    }

    std::string value(env);
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });

    if (value == "1" || value == "true" || value == "yes" || value == "on") {
        return true;
    }

    if (value == "0" || value == "false" || value == "no" || value == "off") {
        return false;
    }

    throw std::runtime_error(
        "ENABLE_EXCHANGE_RATES must be one of: 1, 0, true, false, yes, no, on, off"
    );
}

bool isWhiteListed(std::int64_t chatId)
{
    return getAllowedChatIds().contains(chatId);
}
