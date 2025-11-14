#include "karmaloader.h"

#include <iostream>
#include <fstream>
#include <sstream>

KarmaLoader::KarmaLoader(const std::string &fileName)
    : m_fileName(fileName)
{}

void KarmaLoader::loadKarma(std::unordered_map<std::string, int> &karma)
{
    std::ifstream file(m_fileName);
    if (!file.is_open()) {
        std::cout << "[INFO] karma.csv not found. Starting fresh.\n";
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty())
            continue;

        std::stringstream ss(line);
        std::string username;
        std::string scoreStr;

        if (!std::getline(ss, username, ','))
            continue;
        if (!std::getline(ss, scoreStr, ','))
            continue;

        try {
            int score = std::stoi(scoreStr);
            karma[username] = score;
        } catch (...) {
            std::cerr << "[WARN] Invalid line in CSV: " << line << std::endl;
        }
    }
}

void KarmaLoader::saveKarma(const std::unordered_map<std::string, int> &karma)
{
    std::ofstream file(m_fileName, std::ios::trunc);
    if (!file.is_open()) {
        std::cerr << "[ERROR] Could not open karma.csv for writing!\n";
        return;
    }

    for (const auto &[username, score] : karma) {
        file << username << "," << score << "\n";
    }
}
