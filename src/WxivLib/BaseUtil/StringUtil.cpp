// Copyright(c) 2022 Ryan Seghers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
#include <string>
#include <algorithm>
#include <cassert>
#include <cstring>
#include <filesystem>
#include <locale>
#include <codecvt>

#include "StringUtil.h"

using namespace std;
namespace fs = std::filesystem;

namespace Wxiv
{
    /**
     * @brief Check if little is in big.
     */
    bool checkStringContains(std::string big, const char* little)
    {
        return strstr(big.c_str(), little) != nullptr;
    }

    /**
     * @brief Check if little is in big.
     */
    bool checkStringContains(std::string big, std::string little)
    {
        return big.find(little) != std::string::npos;
    }

    std::string stringJoin(const std::vector<std::string>& strings, const char* delimiter)
    {
        std::string result;
        std::string delim(delimiter);

        for (const auto& s : strings)
        {
            if (!result.empty())
            {
                result.append(delim);
            }

            result.append(s);
        }

        return result;
    }

    /**
     * @brief In-place to lower.
     * @param s
     */
    void toLower(std::string& s)
    {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
    }

    /**
     * @brief In-place to lower.
     * @param s
     */
    void toLower(std::wstring& s)
    {
        std::transform(s.begin(), s.end(), s.begin(), [](wchar_t c) { return std::tolower(c); });
    }

    /**
     * @brief Simple, not fast nor efficient.
     * @param s
     * @param delim
     * @return
     */
    std::vector<std::string> splitString(const std::string s, const std::string delim)
    {
        std::vector<std::string> words;

        size_t lastIndex = 0; // one past last delim
        size_t thisIndex;

        while ((thisIndex = s.find(delim, lastIndex)) != s.npos)
        {
            words.push_back(s.substr(lastIndex, thisIndex - lastIndex));
            lastIndex = thisIndex + 1;
        }

        // last word
        words.push_back(s.substr(lastIndex, s.size() - lastIndex));

        return words;
    }

    /**
     * @brief Trim spaces from beginning and end of input string.
     * @param s
     * @return
     */
    std::string trimSpaces(const std::string s)
    {
        if (!s.empty())
        {
            size_t sidx = s.find_first_not_of(' ');
            size_t eidx = s.find_last_not_of(' ');

            if (sidx == s.npos)
            {
                // all spaces
                return string();
            }

            return s.substr(sidx, eidx - sidx + 1);
        }
        else
        {
            return s;
        }
    }

    bool tryParseInt(std::string s, int& v)
    {
        try
        {
            v = std::stoi(s);
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    bool tryParseFloat(std::string s, float& v)
    {
        try
        {
            v = std::stof(s);
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    /**
     * @brief Without leading period, and lowercased.
     * I am choosing to treat extensions as well-known non-unicode strings, hence not wide strings.
     * @param path
     * @return Can return empty string.
     */
    std::string getExtension(const std::wstring& path)
    {
        fs::path p(path);
        string ext = p.extension().string();

        if (ext.size() > 1)
        {
            toLower(ext);
            return ext.substr(1);
        }
        else
        {
            return string();
        }
    }

    bool checkIsOnlyAscii(const std::wstring& s)
    {
        for (size_t i = 0; i < s.size(); i++)
        {
            if ((int)s[i] > 127)
                return false;
        }

        return true;
    }

    /**
     * @brief Given a string that is presumably a file extension (e.g. ".JPEG") return it lowercase and
     * without leading period (e.g. "jpeg").
     */
    std::string getNormalizedExt(const std::string& inputExt)
    {
        if (inputExt.size() > 1)
        {
            string ext = inputExt.starts_with('.') ? inputExt.substr(1) : inputExt;
            toLower(ext);
            return ext;
        }
        else
        {
            return inputExt;
        }
    }
}
