#include <tgbot/tgbot.h>
#include <unordered_map>
#include <unordered_set>
#include <iostream>

#include "karmabot.h"
#include "karmaloader.h"

std::string loadToken() {
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

int main() {
    // 1. Load bot token
    const std::string token = loadToken();
    TgBot::Bot bot(token);

    KarmaBot karmabot;

    // 3. Handle ANY incoming message
    bot.getEvents().onAnyMessage([&](TgBot::Message::Ptr message)
    {
        if (!message || message->text.empty())
            return;

        karmabot.logMessageToStdout(message);

        bool isGroup = message->chat->type == TgBot::Chat::Type::Group || message->chat->type == TgBot::Chat::Type::Supergroup;
        if (!isGroup || !isWhiteListed(message->chat->id))
        {
            bot.getApi().sendMessage(
                message->chat->id,
                "Hi! I work only inside a dedicated group\n"
                "If you want to adapt the bot in your group, contact the admin @the_good_exchange"
                "Use @username ++ or -- to change karma."
                );
            return;
        }

        std::string response = karmabot.updateKarma(message);
        if(!response.empty())
        {
            bot.getApi().sendMessage(message->chat->id, response);
            return;
        }

        response = karmabot.displayKarma(message);
        if(!response.empty())
        {
            bot.getApi().sendMessage(message->chat->id, response);
            return;
        }
    });

    // 4. Long polling loop
    TgBot::TgLongPoll longPoll(bot);

    std::cout << "Karma bot started..." << std::endl;

    while (true) {
        try {
            longPoll.start();
        } catch (std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }

    return 0;
}
