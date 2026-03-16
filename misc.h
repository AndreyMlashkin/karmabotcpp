#ifndef MISC_H
#define MISC_H

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <cstdint>

std::string telegrammToken();
std::unordered_set<std::int64_t> getAllowedChatIds();
bool exchangeRatesEnabled();
bool isWhiteListed(std::int64_t chatId);

#endif // MISC_H
