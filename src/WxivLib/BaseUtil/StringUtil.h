// Copyright(c) 2022 Ryan Seghers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
#pragma once

#include <string>
#include <vector>

namespace Wxiv
{
    bool checkStringContains(std::string big, const char* little);
    bool checkStringContains(std::string big, std::string little);
    void toLower(std::string& s);
    std::string getNormalizedExt(const std::string& inputExt);

    std::vector<std::string> splitString(const std::string s, const std::string delim);
    std::string stringJoin(const std::vector<std::string>& strings, const char* delimiter);
    bool tryParseInt(std::string s, int& v);
    bool tryParseFloat(std::string s, float& v);

    std::string trimSpaces(const std::string s);
    bool checkIsOnlyAscii(const std::wstring& s);
}
