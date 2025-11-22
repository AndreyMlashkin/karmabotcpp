#ifndef KARMALOADER_H
#define KARMALOADER_H

#include <string>
#include <unordered_map>

class KarmaLoader
{
public:
    KarmaLoader(const std::string& fileName);
    void loadKarma(std::unordered_map<std::string, int> &karma);
    void saveKarma(const std::unordered_map<std::string, int>& karma) const;

    static std::string karmaToString(const std::unordered_map<std::string, int>& karma);

private:
    std::string m_fileName;
};

#endif // KARMALOADER_H
