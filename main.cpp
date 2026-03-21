#include <tgbot/tgbot.h>
#include <unordered_map>
#include <unordered_set>
#include <iostream>
#include <csignal>

#include "karmabot.h"
#include "misc.h"
#include "currenciesbroadcaster.h"
#include "git_version.h"

namespace {

volatile std::sig_atomic_t g_shutdownRequested = 0;

void requestShutdown(int)
{
    g_shutdownRequested = 1;
}

bool shutdownRequested()
{
    return g_shutdownRequested != 0;
}

}

int main() {
    std::signal(SIGINT, requestShutdown);
    std::signal(SIGTERM, requestShutdown);

    // 1. Load bot token
    const std::string token = telegrammToken();
    TgBot::Bot bot(token);
    const auto allowedChats = getAllowedChatIds();

    KarmaBot karmabot;

    // 3. Handle ANY incoming message
    bot.getEvents().onAnyMessage([&](TgBot::Message::Ptr message)
    {
        if (!message || message->text.empty())
            return;

        karmabot.logMessageToStdout(message);

        bool isGroup = message->chat->type == TgBot::Chat::Type::Group || message->chat->type == TgBot::Chat::Type::Supergroup;
        if (!isGroup || !allowedChats.contains(message->chat->id))
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

    std::string startupMessage = std::string("Karma bot updated. Version: ") + gitVersion();
    std::cout << startupMessage << std::endl;

    for (auto chatId : allowedChats) {
        try {
            bot.getApi().sendMessage(chatId, startupMessage);
        } catch (const std::exception& e) {
            std::cerr << "Failed to send startup message to " << chatId
                      << ": " << e.what() << std::endl;
        }
    }

    CurrenciesBroadcaster broadcaster(&bot, allowedChats);
    if (exchangeRatesEnabled()) {
        broadcaster.start();
    } else {
        std::cout << "Exchange rates broadcaster disabled." << std::endl;
    }

    while (!shutdownRequested()) {
        try {
            longPoll.start();
        } catch (std::exception& e) {
            if (shutdownRequested()) {
                break;
            }
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }

    std::cout << "Shutdown requested. Saving karma and exiting." << std::endl;
    karmabot.save();

    return 0;
}
