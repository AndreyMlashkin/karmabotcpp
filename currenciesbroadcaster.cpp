#include "currenciesbroadcaster.h"

#include <thread>
#include <sstream>
#include <chrono>
#include <iostream>
#include <cassert>

#include <tgbot/tgbot.h>

struct Rates {
    double usdKes{};
    double eurKes{};
    double rubKes{};
};

// TODO: replace with real HTTP call to some FX API
inline Rates fetchRates()
{
    Rates r;
    // Placeholder values – replace with real data
    r.usdKes = 130.50;
    r.eurKes = 142.30;
    r.rubKes = 1.45;
    return r;
}

inline std::string formatRates(const Rates& r)
{
    std::ostringstream oss;
    oss << "📈 *Exchange rates (KES)*\n\n"
        << "USD/KES: " << r.usdKes << "\n"
        << "EUR/KES: " << r.eurKes << "\n"
        << "RUB/KES: " << r.rubKes << "\n";
    return oss.str();
}

CurrenciesBroadcaster::CurrenciesBroadcaster(TgBot::Bot *bot, std::unordered_set<std::int64_t> chatsToBroadcast)
    : m_bot(bot),
      m_chatsToBroadcast(chatsToBroadcast)
{
    assert(m_bot);
}

CurrenciesBroadcaster::~CurrenciesBroadcaster()
{
    stop();
}

void CurrenciesBroadcaster::start()
{
    // Already running?
    if (m_thread.joinable()) {
        return;
    }

    m_stop.store(false, std::memory_order_relaxed);
    m_thread = std::thread(&CurrenciesBroadcaster::run, this);
}

void CurrenciesBroadcaster::stop()
{
    m_stop.store(true, std::memory_order_relaxed);
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

void CurrenciesBroadcaster::run()
{
    while (!m_stop.load(std::memory_order_relaxed)) {
        try {
            Rates r = fetchRates();
            std::string msg = formatRates(r);
            std::cout << "Sending rates: " << msg;

            for (auto chatId : m_chatsToBroadcast) {
                try {
                    m_bot->getApi().sendMessage(chatId, msg, nullptr, nullptr, nullptr, "Markdown");
                } catch (const std::exception& e) {
                    std::cerr << "[FX ERROR] Failed to send to chat "
                              << chatId << ": " << e.what() << std::endl;
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "[FX ERROR] fetch/send failed: " << e.what() << std::endl;
        }

        // Sleep in small chunks so stop() is responsive
        for (int i = 0; i < 60 && !m_stop.load(std::memory_order_relaxed); ++i) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
}

bool CurrenciesBroadcaster::isRunning() const noexcept
{
    return m_thread.joinable() && !m_stop.load(std::memory_order_relaxed);
}
