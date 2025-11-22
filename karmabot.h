#ifndef KARMABOT_H
#define KARMABOT_H

#include "karmaloader.h"

#include <unordered_map>
#include <memory>

namespace TgBot
{
class Message;
}

class KarmaBot
{
public:
    KarmaBot();

    static void logMessageToStdout(const std::shared_ptr<TgBot::Message>& message);

    //! If no karma update detected - returns an empty string
    std::string updateKarma(const std::shared_ptr<TgBot::Message>& message);

    std::string displayKarma(const std::shared_ptr<TgBot::Message>& message);


private:
    KarmaLoader m_loader;
    std::unordered_map<std::string, int> m_karma;
};

#endif // KARMABOT_H
