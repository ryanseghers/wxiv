#include <string>
#include <vector>
#include <gtest/gtest.h>

#include "WxivUtil.h"
#include <wx/stdpaths.h>

#include "ArrowUtil.h"
#include "ArrowFilterExpression.h"

using namespace std;
using namespace Wxiv;

namespace WxivTests
{
    /**
     * @brief Create a test table, filter some values, check results.
     * This is going to break every time the test table def changes, but may still be a net win.
     */
    TEST(FilterSpecTests, testBasicFilter)
    {
        int nRows = 10;
        std::shared_ptr<arrow::Table> ptable = ArrowUtil::createTestTable(nRows);
        EXPECT_EQ(ptable->num_rows(), nRows);

        ArrowFilterExpression expr;
        expr.colName = "int32s";
        expr.op = ArrowFilterOpEnum::Equal;
        expr.valueStr = "3";

        FilterSpec spec;
        spec.expressions.push_back(expr);
        vector<int> indices = spec.getMatchingRows(ptable);
        EXPECT_EQ(1, indices.size());
        EXPECT_EQ(3, indices[0]);
    }

    /**
     * @brief Same type of test but with two expressions.
     */
    TEST(FilterSpecTests, testFilterTwoExpressions)
    {
        int nRows = 10;
        std::shared_ptr<arrow::Table> ptable = ArrowUtil::createTestTable(nRows);
        EXPECT_EQ(ptable->num_rows(), nRows);

        FilterSpec spec;
        spec.expressions.push_back(ArrowFilterExpression("int32s", ArrowFilterOpEnum::Gte, "2"));
        spec.expressions.push_back(ArrowFilterExpression("int32s", ArrowFilterOpEnum::Lte, "3"));
        vector<int> indices = spec.getMatchingRows(ptable);
        EXPECT_EQ(2, indices.size());
        EXPECT_EQ(2, indices[0]);
        EXPECT_EQ(3, indices[1]);
    }
}