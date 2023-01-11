#include <string>
#include <vector>
#include <gtest/gtest.h>

#include "WxivUtil.h"
#include <wx/stdpaths.h>

#include "ArrowUtil.h"
#include "TempFile.h"

using namespace std;
using namespace Wxiv;

namespace WxivTests
{
    /**
     * @brief Test save and load parquet and csv file formats, both ascii-only and non-ascii.
     */
    TEST(ArrowUtilTests, testSaveAndLoad)
    {
        for (string ext : {"parquet", "csv"})
        {
            for (bool doUnicodeName : {false, true})
            {
                TempFile tempFile(doUnicodeName ? L"ArrowUtilTests-unicΩode" : L"ArrowUtilTests", ext);

                int nRows = 10;
                std::shared_ptr<arrow::Table> ptable = ArrowUtil::createTestTable(nRows);
                EXPECT_EQ(ptable->num_rows(), nRows);

                // save (this throws for any problem)
                std::string path = toNativeString(tempFile.GetFullPath());
                ArrowUtil::saveFile(path, ptable);

                EXPECT_TRUE(wxFileName(tempFile.GetFullPath()).FileExists());

                // load
                std::shared_ptr<arrow::Table> rt = ArrowUtil::loadFile(path);

                // basic check: not trying to test arrow itself
                EXPECT_EQ(ptable->num_columns(), rt->num_columns());
                EXPECT_EQ(ptable->num_rows(), rt->num_rows());
            }
        }
    }
}