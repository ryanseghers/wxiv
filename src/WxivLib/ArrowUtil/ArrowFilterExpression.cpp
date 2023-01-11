// Copyright(c) 2022 Ryan Seghers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
#include <string>
#include <memory>

#include "ArrowFilterExpression.h"
#include "MiscUtil.h"
#include "StringUtil.h"
#include "MathUtil.h"

using namespace std;

namespace Wxiv
{
    ArrowFilterExpression::ArrowFilterExpression()
    {
    }

    ArrowFilterExpression::ArrowFilterExpression(std::string colName, ArrowFilterOpEnum op, std::string valueStr)
        : colName(colName), op(op), valueStr(valueStr)
    {
    }

    bool ArrowFilterExpression::empty()
    {
        return this->op == ArrowFilterOpEnum::Unset;
    }

    void ArrowFilterExpression::clear()
    {
        this->op = ArrowFilterOpEnum::Unset;
        this->colName = "";
        this->valueStr = "";
    }

    /**
     * @brief Check if this expression is valid for the specified table.
     * An expression can be valid for one table and not for another because column types can be different
     * for the same column name.
     * @param ptable
     * @return
     */
    bool ArrowFilterExpression::checkIsValid(std::shared_ptr<arrow::Table> ptable)
    {
        FilterExpressionVisitor v(ptable, *this);
        return v.isValid;
    }

    FilterExpressionVisitor::FilterExpressionVisitor()
    {
    }

    FilterExpressionVisitor::FilterExpressionVisitor(std::shared_ptr<arrow::Table> ptable, ArrowFilterExpression filterExpression)
    {
        this->filterExpression = filterExpression;
        this->build(ptable);
    }

    /**
     * @brief Check that all expressions match row, i.e. AND them.
     * @return Vector of indices of matching rows in the table.
     */
    vector<int> FilterSpec::getMatchingRows(std::shared_ptr<arrow::Table> ptable)
    {
        size_t n = ptable->num_rows();

        // build visitors
        vector<FilterExpressionVisitor> visitors;

        for (auto& e : this->expressions)
        {
            FilterExpressionVisitor v(ptable, e);
            visitors.push_back(v);
        }

        // match rows
        vector<int> matchingIndices;

        for (int i = 0; i < n; i++)
        {
            // filter expressions disqualify rows, filter them out
            bool isMatch = true;

            for (auto& e : visitors)
            {
                if (!e.checkMatchRow(i))
                {
                    isMatch = false;
                    break;
                }
            }

            if (isMatch)
            {
                matchingIndices.push_back(i);
            }
        }

        return matchingIndices;
    }

    /**
     * @brief Fill in cached values to prep for matching.
     * @param ptable
     * @return
     */
    bool FilterExpressionVisitor::build(std::shared_ptr<arrow::Table> ptable)
    {
        this->col = ptable->GetColumnByName(this->filterExpression.colName);

        if (!this->col)
        {
            return false;
        }

        string valueStr = this->filterExpression.valueStr;
        toLower(valueStr);
        valueStr = trimSpaces(valueStr);

        // parse the value per col type
        colType = this->col->type()->id();

        try
        {
            if (colType == arrow::BooleanType::type_id)
            {
                if ((valueStr == "t") || (valueStr == "true") || (valueStr == "1"))
                {
                    this->valueBool = true;
                }
                else if ((valueStr == "f") || (valueStr == "false") || (valueStr == "0"))
                {
                    this->valueBool = false;
                }
                else
                {
                    throw std::runtime_error("Unrecognized bool value");
                }
            }
            else if (colType == arrow::Int16Type::type_id)
            {
                this->valueInt = std::stoll(valueStr);
            }
            else if (colType == arrow::Int32Type::type_id)
            {
                this->valueInt = std::stoll(valueStr);
            }
            else if (colType == arrow::Int64Type::type_id)
            {
                this->valueInt = std::stoll(valueStr);
            }
            else if (colType == arrow::FloatType::type_id)
            {
                this->valueDouble = std::stod(valueStr);
            }
            else if (colType == arrow::DoubleType::type_id)
            {
                this->valueDouble = std::stod(valueStr);
            }
            else if (colType == arrow::HalfFloatType::type_id)
            {
                this->valueDouble = std::stod(valueStr);
            }
            else
            {
                throw std::runtime_error("build: Input col is an unhandled type.");
            }

            this->isValid = true;
        }
        catch (...)
        {
            this->isValid = false;
        }

        return this->isValid;
    }

    bool FilterExpressionVisitor::checkIntValue(int64_t v)
    {
        ArrowFilterOpEnum op = this->filterExpression.op;

        if ((op == ArrowFilterOpEnum::Equal) && (v == this->valueInt))
        {
            return true;
        }
        else if ((op == ArrowFilterOpEnum::NotEqual) && (v != this->valueInt))
        {
            return true;
        }
        else if ((op == ArrowFilterOpEnum::Gte) && (v >= this->valueInt))
        {
            return true;
        }
        else if ((op == ArrowFilterOpEnum::Lte) && (v <= this->valueInt))
        {
            return true;
        }

        return false;
    }

    bool FilterExpressionVisitor::checkDoubleValue(double v)
    {
        ArrowFilterOpEnum op = this->filterExpression.op;

        if ((op == ArrowFilterOpEnum::Equal) && checkClose(v, this->valueDouble))
        {
            return true;
        }
        else if ((op == ArrowFilterOpEnum::NotEqual) && !checkClose(v, this->valueDouble))
        {
            return true;
        }
        else if ((op == ArrowFilterOpEnum::Gte) && (v >= this->valueDouble - std::numeric_limits<float>::epsilon()))
        {
            return true;
        }
        else if ((op == ArrowFilterOpEnum::Lte) && (v <= this->valueDouble + std::numeric_limits<float>::epsilon()))
        {
            return true;
        }

        return false;
    }

    bool FilterExpressionVisitor::checkBoolValue(bool v)
    {
        ArrowFilterOpEnum op = this->filterExpression.op;

        if ((op == ArrowFilterOpEnum::Equal) && (v == this->valueBool))
        {
            return true;
        }
        else if ((op == ArrowFilterOpEnum::NotEqual) && (v != this->valueBool))
        {
            return true;
        }
        else if ((op == ArrowFilterOpEnum::Gte) && (v >= this->valueBool))
        {
            return true;
        }
        else if ((op == ArrowFilterOpEnum::Lte) && (v <= this->valueBool))
        {
            return true;
        }

        return false;
    }

    bool FilterExpressionVisitor::checkStringValue(std::string v)
    {
        ArrowFilterOpEnum op = this->filterExpression.op;
        string valueStr = this->filterExpression.valueStr;

        if ((op == ArrowFilterOpEnum::Equal) && (v == valueStr))
        {
            return true;
        }
        else if ((op == ArrowFilterOpEnum::NotEqual) && (v != valueStr))
        {
            return true;
        }
        else if ((op == ArrowFilterOpEnum::Gte) && (v >= valueStr))
        {
            return true;
        }
        else if ((op == ArrowFilterOpEnum::Lte) && (v <= valueStr))
        {
            return true;
        }

        return false;
    }

    arrow::Status FilterExpressionVisitor::Visit(const arrow::NullScalar& s)
    {
        return arrow::Status(arrow::StatusCode::Invalid, "No match");
    }

    arrow::Status FilterExpressionVisitor::Visit(const arrow::StringScalar& s)
    {
        return this->checkStringValue(s.ToString()) ? arrow::Status::OK() : arrow::Status(arrow::StatusCode::Invalid, "mismatch");
    }

    arrow::Status FilterExpressionVisitor::Visit(const arrow::BooleanScalar& s)
    {
        return this->checkBoolValue(s.value) ? arrow::Status::OK() : arrow::Status(arrow::StatusCode::Invalid, "mismatch");
    }

    arrow::Status FilterExpressionVisitor::Visit(const arrow::Int8Scalar& s)
    {
        return this->checkIntValue((int64_t)s.value) ? arrow::Status::OK() : arrow::Status(arrow::StatusCode::Invalid, "mismatch");
    }

    arrow::Status FilterExpressionVisitor::Visit(const arrow::Int16Scalar& s)
    {
        return this->checkIntValue((int64_t)s.value) ? arrow::Status::OK() : arrow::Status(arrow::StatusCode::Invalid, "mismatch");
    }

    arrow::Status FilterExpressionVisitor::Visit(const arrow::Int32Scalar& s)
    {
        return this->checkIntValue((int64_t)s.value) ? arrow::Status::OK() : arrow::Status(arrow::StatusCode::Invalid, "mismatch");
    }

    arrow::Status FilterExpressionVisitor::Visit(const arrow::Int64Scalar& s)
    {
        return this->checkIntValue((int64_t)s.value) ? arrow::Status::OK() : arrow::Status(arrow::StatusCode::Invalid, "mismatch");
    }

    arrow::Status FilterExpressionVisitor::Visit(const arrow::UInt8Scalar& s)
    {
        return this->checkIntValue((int64_t)s.value) ? arrow::Status::OK() : arrow::Status(arrow::StatusCode::Invalid, "mismatch");
    }

    arrow::Status FilterExpressionVisitor::Visit(const arrow::UInt16Scalar& s)
    {
        return this->checkIntValue((int64_t)s.value) ? arrow::Status::OK() : arrow::Status(arrow::StatusCode::Invalid, "mismatch");
    }

    arrow::Status FilterExpressionVisitor::Visit(const arrow::UInt32Scalar& s)
    {
        return this->checkIntValue((int64_t)s.value) ? arrow::Status::OK() : arrow::Status(arrow::StatusCode::Invalid, "mismatch");
    }

    arrow::Status FilterExpressionVisitor::Visit(const arrow::UInt64Scalar& s)
    {
        return this->checkIntValue((int64_t)s.value) ? arrow::Status::OK() : arrow::Status(arrow::StatusCode::Invalid, "mismatch");
    }

    arrow::Status FilterExpressionVisitor::Visit(const arrow::HalfFloatScalar& s)
    {
        return this->checkDoubleValue((double)s.value) ? arrow::Status::OK() : arrow::Status(arrow::StatusCode::Invalid, "mismatch");
    }

    arrow::Status FilterExpressionVisitor::Visit(const arrow::FloatScalar& s)
    {
        return this->checkDoubleValue((double)s.value) ? arrow::Status::OK() : arrow::Status(arrow::StatusCode::Invalid, "mismatch");
    }

    arrow::Status FilterExpressionVisitor::Visit(const arrow::DoubleScalar& s)
    {
        return this->checkDoubleValue((double)s.value) ? arrow::Status::OK() : arrow::Status(arrow::StatusCode::Invalid, "mismatch");
    }

    /**
     * @brief Check if the specified row (in the table used to build this visitor) matches this filter spec.
     * On any kind of failure this returns false which is mostly arbitrary but seems to be
     * better for perf: on bad filter spec nothing survives the filter.
     * Null/empty values do not pass the filter.
     * @param rowIndex
     * @return Whether the row survives the filter or not.
     */
    bool FilterExpressionVisitor::checkMatchRow(int rowIndex)
    {
        auto rowResult = this->col->GetScalar(rowIndex);

        if (rowResult.ok())
        {
            auto ps = rowResult.ValueUnsafe();

            if (ps->is_valid)
            {
                return ps->Accept(this).ok();
            }
        }

        return false;
    }
}
