// Copyright(c) 2022 Ryan Seghers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
#pragma once
#include <string>
#include <vector>
#include <memory>
#include <cmath>
#include <limits>

#pragma warning(push, 0)
#include <arrow/api.h>
#pragma warning(pop)

namespace Wxiv
{
    /**
     * @brief Enumerates filter comparator operations.
     */
    enum class ArrowFilterOpEnum
    {
        Unset,
        Equal,
        NotEqual,
        Lte,
        Gte,
    };

    /**
     * @brief Represents an expression that can be used to filter rows in an arrow table, for example field1 > 4
     * where field1 is the name of a column in the table.
     */
    struct ArrowFilterExpression
    {
        std::string colName;
        ArrowFilterOpEnum op = ArrowFilterOpEnum::Unset;
        std::string valueStr;

        ArrowFilterExpression();
        ArrowFilterExpression(std::string colName, ArrowFilterOpEnum op, std::string valueStr);
        bool empty();
        void clear();
        bool checkIsValid(std::shared_ptr<arrow::Table> ptable);
    };

    /**
     * @brief This is an arrow ScalarVisitor for ArrowFilterExpression.
     */
    struct FilterExpressionVisitor : public arrow::ScalarVisitor
    {
        ArrowFilterExpression filterExpression;

        /**
         * @brief Whether or not the valueStr could be parsed appropriately for the column type.
         * This is not known until the column type is known which is not necessarily on construction.
         */
        bool isValid = false;

        // cache these values for perf
        int64_t valueInt = -1;
        bool valueBool = false;
        double valueDouble = std::numeric_limits<double>::quiet_NaN();
        std::shared_ptr<arrow::ChunkedArray> col = nullptr;
        arrow::Type::type colType = arrow::Type::NA;

        FilterExpressionVisitor();
        FilterExpressionVisitor(std::shared_ptr<arrow::Table> ptable, ArrowFilterExpression expr);

        bool build(std::shared_ptr<arrow::Table> ptable);
        bool checkMatchRow(int rowIndex);

        // functions to check via Visitor
        bool checkIntValue(int64_t v);
        bool checkDoubleValue(double v);
        bool checkBoolValue(bool v);
        bool checkStringValue(std::string v);

        // Visitor methods
        arrow::Status Visit(const arrow::NullScalar& s) override;
        arrow::Status Visit(const arrow::BooleanScalar& s) override;
        arrow::Status Visit(const arrow::StringScalar& s) override;

        arrow::Status Visit(const arrow::Int8Scalar& s) override;
        arrow::Status Visit(const arrow::Int16Scalar& s) override;
        arrow::Status Visit(const arrow::Int32Scalar& s) override;
        arrow::Status Visit(const arrow::Int64Scalar& s) override;
        arrow::Status Visit(const arrow::UInt8Scalar& s) override;
        arrow::Status Visit(const arrow::UInt16Scalar& s) override;
        arrow::Status Visit(const arrow::UInt32Scalar& s) override;
        arrow::Status Visit(const arrow::UInt64Scalar& s) override; // TODO: this could go out of range when cast to int

        arrow::Status Visit(const arrow::HalfFloatScalar& s) override;
        arrow::Status Visit(const arrow::FloatScalar& s) override;
        arrow::Status Visit(const arrow::DoubleScalar& s) override;
    };

    /**
     * @brief Contains a set of FilterExpressions and can use FilterExpressionVisitor to filter a table.
     */
    struct FilterSpec
    {
        std::vector<ArrowFilterExpression> expressions;

        std::vector<int> getMatchingRows(std::shared_ptr<arrow::Table> ptable);
    };
}
