#include <gtest/gtest.h>
#include <string>
#include "StringUtil.h"

using namespace std;
using namespace Wxiv;

namespace WxivTests
{
    TEST(StringUtilTests, testIsOnlyAsciiOnOnlyAscii)
    {
        std::wstring s(L"only ascii");
        EXPECT_TRUE(checkIsOnlyAscii(s));
    }

    TEST(StringUtilTests, testIsOnlyAsciiOnUnicode)
    {
        std::wstring s(L"has unicΩode");
        EXPECT_FALSE(checkIsOnlyAscii(s));
    }
}
