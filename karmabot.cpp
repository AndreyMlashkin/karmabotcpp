#include "karmabot.h"

#include <tgbot/tgbot.h>

#include <iostream>
#include <regex>

namespace {

std::string getUserName(TgBot::Message::Ptr message)
{
    std::string sender;

    if (message->from)
    {
        if (!message->from->username.empty())
        {
            sender = "@" + message->from->username;
        }
        else
        {
            // fallback if no username
            sender = message->from->firstName;
            if (!message->from->lastName.empty()) {
                sender += " " + message->from->lastName;
            }
        }
    }
    else
    {
        sender = "<unknown>";
    }
    return sender;
}

}

KarmaBot::KarmaBot()
    : m_loader("saved_karma.txt")
{
    m_loader.loadKarma(m_karma);
}

void KarmaBot:: logMessageToStdout(const std::shared_ptr<TgBot::Message> &message)
{
    std::cout << "got a message: " << message->text
              << " from: " << getUserName(message)
              << " in chat: " << message->chat->id
              << std::endl;
}

std::string KarmaBot::updateKarma(const std::shared_ptr<TgBot::Message> &message)
{
    const std::string& text = message->text;

    // Regex to catch things like "@user123 ++" or "@user123--"
    std::regex karmaRegex(R"((@\w+)\s*(\+\+|--))");

    // Regex to catch plain "++" or "--" in a reply
    std::regex replyKarmaRegex(R"(^\s*(\+\+|--)\s*$)");

    std::string targetKey;   // what we use as key in the karma map
    std::string displayName; // what we show in the message
    std::string op;

    std::smatch match;

    // CASE 1: Classic "@username ++" / "@username --"
    if (std::regex_search(text, match, karmaRegex)) {
        // match[1] = "@username", match[2] = "++" or "--"
        displayName = match[1].str();
        targetKey = displayName; // store by @username
        op = match[2].str();
    }
    // CASE 2: Reply with "++" or "--" → affect replied user
    else if (message->replyToMessage && std::regex_match(text, match, replyKarmaRegex)) {
        op = match[1].str(); // "++" or "--"

        if (message->replyToMessage->from) {
            if (!message->replyToMessage->from->username.empty()) {
                displayName = "@" + message->replyToMessage->from->username;
                targetKey = displayName; // use @username as key
            } else {
                // Fallback if user has no username: use id-based key
                targetKey = "id:" + std::to_string(message->replyToMessage->from->id);
                displayName = targetKey;
            }
        }
    }

    // If we didn't detect any karma operation, continue as usual
    if (targetKey.empty())
        return {};

    // Prevent self-karma (for both mention and reply cases)
    if (message->from && message->replyToMessage
            && message->from->id == message->replyToMessage->from->id)
    {
        return "You cannot change your own karma 😉";
    }
    // Also prevent "@self ++" case
    if (message->from && !message->from->username.empty()
            && displayName == "@" + message->from->username)
    {
        return "You cannot change your own karma 😉";
    }

    int delta = (op == "++") ? 1 : -1;
    assert(targetKey.size() != 0);
    int &score = m_karma[targetKey];
    score += delta;

    std::string response = displayName + " now has karma: " + std::to_string(score);
    m_loader.saveKarma(m_karma);
    return response;
}

std::string KarmaBot::displayKarma(const std::shared_ptr<TgBot::Message> &message)
{
    const std::string& text = message->text;
    // Command to show karma: "/karma @user"
    if (text.rfind("/karma", 0) != 0)
        return {};

    std::istringstream iss(text);
    std::string cmd, userArg;
    iss >> cmd >> userArg;

    // CASE 1: /karma @username → show karma for that user
    if (!userArg.empty() && userArg[0] == '@') {
        int score = m_karma[userArg];
        std::string response = userArg + " has karma: " + std::to_string(score);
        return response;
    }

    // CASE 2: /karma with NO argument → dump full report using KarmaLoader::karmaToString
    if (userArg.empty()) {
        if (m_karma.empty())
        {
            return "No karma data available yet.";
        }

        // Use KarmaLoader to generate a string representation
        std::string csv = m_loader.karmaToString(m_karma);

        // You can send it as plain text (username,karma per line)
        std::string report = "📊 Karma report (username, karma):\n\n" + csv;
        return report;
    }
    // CASE 3: Invalid usage
    return "Usage:\n"
           "/karma @username — show karma for a user\n"
           "/karma — show full karma report";
}
