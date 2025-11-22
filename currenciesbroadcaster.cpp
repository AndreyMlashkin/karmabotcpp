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

    double usdKesBuy() const
    {
        return usdKes * 0.97;
    }
    double usdKesSell() const
    {
        return usdKes * 1.03;
    }
    double eurKesBuy() const
    {
        return usdKes * 0.99;
    }
    double eurKesSell() const
    {
        return usdKes * 1.05;
    }
    double rubKesBuy() const
    {
        return usdKes * 0.965;
    }
    double rubKesSell() const
    {
        return usdKes * 1.03;
    }
};

Rates fetchRates()
{
    using nlohmann::json;

    auto r = cpr::Get(
        cpr::Url{"https://open.er-api.com/v6/latest/USD"}
    );

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
        throw std::runtime_error(std::string("FX API returned non-success: ") + err + "\nText was: " + r.text);
    }

    if (!j.contains("rates")) {
        throw std::runtime_error(std::string("FX response has no 'rates' field") + "\nText was: " + r.text);
    }

    auto rates = j.at("rates");

    if (!rates.contains("KES") || !rates.contains("EUR") || !rates.contains("RUB")) {
        throw std::runtime_error(std::string("FX response missing one of KES/EUR/RUB") + "\nText was: " + r.text);
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
        << "USD/KES: " << r.usdKesBuy() << " - " << r.usdKesSell() << "\n"
        << "EUR/KES: " << r.eurKesBuy() << " - " << r.eurKesSell() << "\n"
        << "RUB/KES: " << r.rubKesBuy() << " - " << r.rubKesSell() << "\n";
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
    using clock = std::chrono::system_clock;

    const int TRIGGER_HOUR_UTC = 16; // 19:00 EAT = 16:00 UTC

    int lastSentYday = -1; // day-of-year tracking for catch-up

    while (!m_stop.load(std::memory_order_relaxed)) {

        // ---- 1. Get current UTC time ----
        auto now = clock::now();
        std::time_t tt = clock::to_time_t(now);

        std::tm utc{};
#if defined(_WIN32)
        gmtime_s(&utc, &tt);
#else
        utc = *std::gmtime(&tt);
#endif

        const int todayYday = utc.tm_yday;

        // ---- 2. Catch-up logic: If UTC > 16:00 and we did not send today ----
        bool pastTrigger =
            (utc.tm_hour > TRIGGER_HOUR_UTC) ||
            (utc.tm_hour == TRIGGER_HOUR_UTC &&
             (utc.tm_min > 0 || utc.tm_sec >= 0));

        if (!m_stop.load() && pastTrigger && lastSentYday != todayYday) {
            sendRates();
            lastSentYday = todayYday;
        }

        // ---- 3. Compute next trigger time (UTC today or tomorrow) ----

        std::tm target = utc;
        target.tm_hour = TRIGGER_HOUR_UTC;
        target.tm_min  = 0;
        target.tm_sec  = 0;

        std::time_t target_tt = timegm(&target); // always convert as UTC

        if (target_tt <= tt) {
            // already past today's 16:00 UTC → schedule tomorrow
            target.tm_mday += 1;
            target_tt = timegm(&target);
        }

        auto target_time = clock::from_time_t(target_tt);

        // ---- 4. Sleep in chunks until next trigger ----
        while (!m_stop.load()) {
            auto now2 = clock::now();
            if (now2 >= target_time)
                break;

            auto remain = std::chrono::duration_cast<std::chrono::seconds>(
                target_time - now2
            );

            auto chunk = std::min(remain, std::chrono::seconds(60));
            if (chunk.count() <= 0) break;

            std::this_thread::sleep_for(chunk);
        }

        if (m_stop.load()) break;

        // ---- 5. Scheduled send ----
        sendRates();

        // Update last sent day (UTC)
        auto s_now = std::chrono::system_clock::now();
        std::time_t s_tt = std::chrono::system_clock::to_time_t(s_now);

        std::tm s_utc{};
#if defined(_WIN32)
        gmtime_s(&s_utc, &s_tt);
#else
        s_utc = *std::gmtime(&s_tt);
#endif
        lastSentYday = s_utc.tm_yday;
    }
}

void CurrenciesBroadcaster::sendRates()
{
    try {
        Rates r = fetchRates();
        std::string msg = formatRates(r);
        std::cout << "Sending rates: " << msg;

        for (auto chatId : m_chatsToBroadcast)
        {
            try
            {
                m_bot->getApi().sendMessage(chatId, msg, nullptr, nullptr, nullptr, "Markdown");
            }
            catch (const std::exception& e)
            {
                std::cerr << "[FX ERROR] Failed to send to chat "
                          << chatId << ": " << e.what() << std::endl;
            }
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "[FX ERROR] fetch/send failed: " << e.what() << std::endl;
    }
}

bool CurrenciesBroadcaster::isRunning() const noexcept
{
    return m_thread.joinable() && !m_stop.load(std::memory_order_relaxed);
}
