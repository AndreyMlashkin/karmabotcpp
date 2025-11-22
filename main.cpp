#include <tgbot/tgbot.h>
#include <unordered_map>
#include <regex>
#include <iostream>

#include "karmaloader.h"

std::string loadToken() {
    const char* token = std::getenv("TELEGRAM_BOT_TOKEN");
    if (!token) {
        throw std::runtime_error("TELEGRAM_BOT_TOKEN env variable is not set");
    }
    return std::string(token);
}

std::int64_t getChatId()
{
    const char* id = std::getenv("CHAT_ID");
    if (!id) {
        throw std::runtime_error("CHAT_ID env variable is not set");
    }

    try {
        return std::stoll(std::string(id));
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Invalid CHAT_ID value: ") + e.what());
    }
}


int main() {
    // 1. Load bot token
    const std::string token = loadToken();
    TgBot::Bot bot(token);

    // 2. In-memory karma storage: username -> score
    KarmaLoader loader("saved_karma.txt");
    std::unordered_map<std::string, int> karma;
    loader.loadKarma(karma);

    // Regex to catch things like "@user123 ++" or "@user123--"
    std::regex karmaRegex(R"((@\w+)\s*(\+\+|--))");

    // Regex to catch plain "++" or "--" in a reply
    std::regex replyKarmaRegex(R"(^\s*(\+\+|--)\s*$)");

    // 3. Handle ANY incoming message
    bot.getEvents().onAnyMessage([&](TgBot::Message::Ptr message) {
        std::cout << "got a message: " << message->text << " from: " << message->from << std::endl;

        if (!message || message->text.empty())
            return;

        const std::string& text = message->text;

        if (message->chat->type != TgBot::Chat::Type::Group &&
            message->chat->type != TgBot::Chat::Type::Supergroup)
        {
            bot.getApi().sendMessage(
                message->chat->id,
                "Hi! I work only inside a dedicated group\n"
                "If you want to adapt the bot in your group, contact the admin @the_good_exchange"
                "Use @username ++ or -- to change karma."
                );
            return;
        }

        // ---- KARMA UPDATE (mention style or reply style) ----
        {
            std::string targetKey;   // what we use as key in the karma map
            std::string displayName; // what we show in the message
            std::string op;

            std::smatch match;

            // CASE 1: Classic "@username ++" / "@username --"
            if (std::regex_search(text, match, karmaRegex)) {
                // match[1] = "@username", match[2] = "++" or "--"
                displayName = match[1].str();
                targetKey   = displayName;          // store by @username
                op          = match[2].str();
            }
            // CASE 2: Reply with "++" or "--" → affect replied user
            else if (message->replyToMessage &&
                     std::regex_match(text, match, replyKarmaRegex)) {
                op = match[1].str(); // "++" or "--"

                if (message->replyToMessage->from) {
                    if (!message->replyToMessage->from->username.empty()) {
                        displayName = "@" + message->replyToMessage->from->username;
                        targetKey   = displayName; // use @username as key
                    } else {
                        // Fallback if user has no username: use id-based key
                        targetKey   = "id:" + std::to_string(message->replyToMessage->from->id);
                        displayName = targetKey;
                    }
                }
            }

            // If we didn't detect any karma operation, continue as usual
            if (targetKey.empty()) {
                // fall through to /karma handling
            } else {
                // Prevent self-karma (for both mention and reply cases)
                if (message->from && message->replyToMessage &&
                    message->from->id == message->replyToMessage->from->id) {
                    bot.getApi().sendMessage(
                        message->chat->id,
                        "You cannot change your own karma 😉"
                        );
                    return;
                }
                // Also prevent "@self ++" case
                if (message->from &&
                    !message->from->username.empty() &&
                    displayName == "@" + message->from->username) {
                    bot.getApi().sendMessage(
                        message->chat->id,
                        "You cannot change your own karma 😉"
                        );
                    return;
                }

                int delta = (op == "++") ? 1 : -1;
                int& score = karma[targetKey];
                score += delta;
                loader.saveKarma(karma);

                std::string response = displayName + " now has karma: " + std::to_string(score);
                bot.getApi().sendMessage(message->chat->id, response);
                return;
            }
        }

        // Command to show karma: "/karma @user"
        if (text.rfind("/karma", 0) == 0) { // starts with "/karma"
            std::istringstream iss(text);
            std::string cmd, userArg;
            iss >> cmd >> userArg;

            // CASE 1: /karma @username → show karma for that user
            if (!userArg.empty() && userArg[0] == '@') {
                int score = karma[userArg];
                std::string response = userArg + " has karma: " + std::to_string(score);
                bot.getApi().sendMessage(message->chat->id, response);
                return;
            }

            // CASE 2: /karma with NO argument → dump full report using KarmaLoader::karmaToString
            if (userArg.empty()) {
                if (karma.empty()) {
                    bot.getApi().sendMessage(
                        message->chat->id,
                        "No karma data available yet."
                        );
                    return;
                }

                // Use KarmaLoader to generate a string representation
                std::string csv = loader.karmaToString(karma);

                // You can send it as plain text (username,karma per line)
                std::string report = "📊 Karma report (username, karma):\n\n" + csv;

                bot.getApi().sendMessage(
                    message->chat->id,
                    report
                    );
                return;
            }
            // CASE 3: Invalid usage
            bot.getApi().sendMessage(message->chat->id,
                                     "Usage:\n"
                                     "/karma @username — show karma for a user\n"
                                     "/karma — show full karma report");
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
