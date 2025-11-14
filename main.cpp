#include <tgbot/tgbot.h>
#include <unordered_map>
#include <regex>
#include <iostream>

std::string loadToken() {
    const char* token = std::getenv("TELEGRAM_BOT_TOKEN");
    if (!token) {
        throw std::runtime_error("TELEGRAM_BOT_TOKEN env variable is not set");
    }
    return std::string(token);
}

int main() {
    // 1. Load bot token
    const std::string token = loadToken();
    TgBot::Bot bot(token);

    // 2. In-memory karma storage: username -> score
    std::unordered_map<std::string, int> karma;

    // Regex to catch things like "@user123 ++" or "@user123--"
    std::regex karmaRegex(R"((@\w+)\s*(\+\+|--))");

    // 3. Handle ANY incoming message
    bot.getEvents().onAnyMessage([&](TgBot::Message::Ptr message) {
        std::cout << "got a message: " << message << "\t" << message->text << std::endl;

        if (!message || message->text.empty())
            return;

        const std::string& text = message->text;

        if (message->chat->type != TgBot::Chat::Type::Group &&
            message->chat->type != TgBot::Chat::Type::Supergroup)
        {
            bot.getApi().sendMessage(
                message->chat->id,
                "Hi! I work only inside group chats.\n"
                "Add me to a group and use @username ++ or -- to change karma."
                );
            return;
        }

        std::smatch match;
        if (std::regex_search(text, match, karmaRegex)) {
            // match[1] = "@username", match[2] = "++" or "--"
            std::string username = match[1].str();
            std::string op       = match[2].str();

            // Prevent self-karma
            if (message->from && ("@" + message->from->username) == username) {
                bot.getApi().sendMessage(
                    message->chat->id,
                    "You cannot change your own karma 😉"
                    );
                return;
            }

            int delta = (op == "++") ? 1 : -1;
            int& score = karma[username];
            score += delta;

            std::string response = username + " now has karma: " + std::to_string(score);
            bot.getApi().sendMessage(message->chat->id, response);
            return;
        }

        // Command to show karma: "/karma @user"
        if (text.rfind("/karma", 0) == 0) { // starts with "/karma"
            std::istringstream iss(text);
            std::string cmd, userArg;
            iss >> cmd >> userArg;

            if (!userArg.empty() && userArg[0] == '@') {
                int score = karma[userArg];
                std::string response = userArg + " has karma: " + std::to_string(score);
                bot.getApi().sendMessage(message->chat->id, response);
            } else {
                bot.getApi().sendMessage(
                    message->chat->id,
                    "Usage: /karma @username"
                    );
            }
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
