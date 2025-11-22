#include "currenciesbroadcaster.h"

#include <thread>
#include <sstream>
#include <chrono>
#include <iostream>
#include <cassert>

#include <tgbot/tgbot.h>

#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

struct Rates {
    double usdKes{};
    double eurKes{};
    double rubKes{};
};

Rates fetchRates()
{
    using nlohmann::json;

    auto r = cpr::Get(
        cpr::Url{"https://open.er-api.com/v6/latest/USD"}
    );

    std::cout << r.text;

    if (r.error || r.status_code != 200) {
        throw std::runtime_error(
            "FX HTTP error: " + r.error.message +
            " (status " + std::to_string(r.status_code) + ")" +
            " Text was:\n" + r.text
        );
    }

    json j = json::parse(r.text);

    // Basic success check
    if (j.value("result", std::string{}) != "success") {
        std::string err = "unknown";
        if (j.contains("error-type")) {
            err = j["error-type"].get<std::string>();
        }
        throw std::runtime_error("FX API returned non-success: " + err);
    }

    if (!j.contains("rates")) {
        throw std::runtime_error("FX response has no 'rates' field");
    }

    auto rates = j.at("rates");

    if (!rates.contains("KES") || !rates.contains("EUR") || !rates.contains("RUB")) {
        throw std::runtime_error("FX response missing one of KES/EUR/RUB");
    }

    // 1 USD = ? KES / EUR / RUB
    double usdKes = rates.at("KES").get<double>(); // USD→KES
    double usdEur = rates.at("EUR").get<double>(); // USD→EUR
    double usdRub = rates.at("RUB").get<double>(); // USD→RUB

    Rates out;
    out.usdKes = usdKes;

    // We want 1 EUR = ? KES
    // 1 USD = usdEur EUR → 1 EUR = (1 / usdEur) USD → in KES: usdKes / usdEur
    out.eurKes = usdKes / usdEur;

    // 1 RUB = ? KES
    out.rubKes = usdKes / usdRub;

    return out;
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
