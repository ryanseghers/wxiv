// Copyright(c) 2022 Ryan Seghers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
#pragma once
#include <string>
#include <vector>
#include <memory>

#pragma warning(push, 0)
#include <arrow/api.h>
#pragma warning(pop)

#include "MiscUtil.h"

namespace Wxiv
{
    /**
     * @brief The filesystem handling in Arrow handles utf-8 std::string on both Windows and Linux, e.g. std::string(s.mb_str(wxConvUTF8));
     */
    namespace ArrowUtil
    {
        std::shared_ptr<arrow::Table> loadFile(const std::string& path);
        void saveFile(const std::string& path, std::shared_ptr<arrow::Table> ptable);

        std::shared_ptr<arrow::ChunkedArray> getColumn(std::shared_ptr<arrow::Table> ptable, std::string name);
        bool getIntValues(std::shared_ptr<arrow::ChunkedArray> col, std::vector<int>& values, int defaultValue);
        bool getFloatValues(std::shared_ptr<arrow::ChunkedArray> col, std::vector<float>& values);

        std::shared_ptr<arrow::Array> buildInt32Array(const std::vector<int>& values);
        std::shared_ptr<arrow::Array> buildBoolArray(const std::vector<bool>& values);
        std::shared_ptr<arrow::Array> buildFloatArray(const std::vector<float>& values);
        std::shared_ptr<arrow::Array> buildDoubleArray(const std::vector<double>& values);
        std::shared_ptr<arrow::Array> buildStringArray(const std::vector<std::string>& values);

        std::shared_ptr<arrow::Table> createTestTable(int nRows);
    }
}
