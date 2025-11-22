#ifndef CURRENCIESBROADCASTER_H
#define CURRENCIESBROADCASTER_H

#include <unordered_set>
#include <cstdint>
#include <atomic>
#include <thread>

namespace TgBot {
class Bot;
}


class CurrenciesBroadcaster
{
public:
    CurrenciesBroadcaster(TgBot::Bot* bot, std::unordered_set<std::int64_t> chatsToBroadcast);
    ~CurrenciesBroadcaster();

    void start();
    void stop();
    bool isRunning() const noexcept;

private:
    void run();

    TgBot::Bot* m_bot;
    std::unordered_set<std::int64_t> m_chatsToBroadcast;

    std::thread m_thread;
    std::atomic<bool> m_stop{false};
};

#endif // CURRENCIESBROADCASTER_H
