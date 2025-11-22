#include <tgbot/tgbot.h>
#include <unordered_map>
#include <unordered_set>
#include <iostream>

#include "karmabot.h"
#include "misc.h"

int main() {
    // 1. Load bot token
    const std::string token = telegrammToken();
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
