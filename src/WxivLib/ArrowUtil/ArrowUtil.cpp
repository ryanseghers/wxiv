// Copyright(c) 2022 Ryan Seghers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
#include <string>
#include <filesystem>

#include "ArrowUtil.h"

#pragma warning(push, 0)
#include <arrow/api.h>
#include <arrow/io/file.h>
#include <arrow/io/api.h>
#include <arrow/csv/api.h>
#include <arrow/result.h>
#include <arrow/status.h>
#include <arrow/filesystem/localfs.h>
#include <arrow/csv/writer.h>

#include <parquet/api/reader.h>
#include <parquet/arrow/reader.h>
#include <parquet/arrow/writer.h>
#pragma warning(pop)

#include <fmt/core.h>
#include <fmt/xchar.h>

#include "MiscUtil.h"
#include "StringUtil.h"

using namespace std;
namespace fs = std::filesystem;

namespace Wxiv
{
    namespace ArrowUtil
    {
        /**
         * @brief Load a csv or parquet file into a Table.
         * @param path Local file path. Can be utf-8 on both Windows and Linux.
         * @return shared_ptr Table
         */
        std::shared_ptr<arrow::Table> loadFile(const std::string& path)
        {
            // open file
            arrow::fs::LocalFileSystem file_system;
            auto maybe_input = file_system.OpenInputFile(path);

            if (!maybe_input.ok())
            {
                return nullptr;
            }

            std::shared_ptr<arrow::io::RandomAccessFile> input = *maybe_input;

            if (path.ends_with(".csv"))
            {
                arrow::io::IOContext io_context = arrow::io::default_io_context();
                auto read_options = arrow::csv::ReadOptions::Defaults();
                auto parse_options = arrow::csv::ParseOptions::Defaults();
                auto convert_options = arrow::csv::ConvertOptions::Defaults();

                auto maybe_reader = arrow::csv::TableReader::Make(io_context, input, read_options, parse_options, convert_options);

                if (!maybe_reader.ok())
                {
                    throw std::runtime_error(fmt::format("Arrow failed to open the csv file: {}", path).c_str());
                }

                std::shared_ptr<arrow::csv::TableReader> reader = *maybe_reader;

                // read
                auto maybe_table = reader->Read();

                if (!maybe_table.ok())
                {
                    throw std::runtime_error(fmt::format("Arrow failed to parse the csv file: {}", path).c_str());
                }

                std::shared_ptr<arrow::Table> table = *maybe_table;
                return table;
            }
            else if (path.ends_with("parquet"))
            {
                std::shared_ptr<arrow::io::RandomAccessFile> input = file_system.OpenInputFile(path).ValueOrDie();

                std::unique_ptr<parquet::arrow::FileReader> arrow_reader;
                arrow::MemoryPool* pool = arrow::default_memory_pool();
                auto st = parquet::arrow::OpenFile(input, pool, &arrow_reader);

                if (!st.ok())
                {
                    throw std::runtime_error(fmt::format("Arrow failed to open the parquet file: {}", path).c_str());
                }

                // Read entire file as a single Arrow table
                std::shared_ptr<arrow::Table> table;
                st = arrow_reader->ReadTable(&table);

                if (!st.ok())
                {
                    throw std::runtime_error(fmt::format("Arrow failed to parse the parquet file: {}", path).c_str());
                }

                return table;
            }
            else
            {
                throw std::runtime_error("Unhandled file format.");
            }
        }

        /**
         * @brief Save a table to a parquet or csv file. Throws for any error.
         * @param path Output file path, must end with ".parquet" or ".csv". Can be utf-8 on both Windows and Linux.
         * @param ptable The table to write.
         * @return
         */
        void saveFile(const std::string& path, std::shared_ptr<arrow::Table> ptable)
        {
            auto outStream = arrow::io::FileOutputStream::Open(path).ValueOrDie();

            string ext = fs::path(path).extension().string();

            if (ext == ".parquet")
            {
                if (!parquet::arrow::WriteTable(*ptable, arrow::default_memory_pool(), outStream, 4 * 1024 * 1024).ok())
                {
                    bail("Error writing arrow table to parquet file.");
                }
            }
            else if (ext == ".csv")
            {
                if (!arrow::csv::WriteCSV(*ptable, arrow::csv::WriteOptions::Defaults(), outStream.get()).ok())
                {
                    bail("Error writing arrow table to csv file.");
                }
            }

            if (!outStream->Close().ok())
            {
                bail("Error closing output arrow file close failed.");
            }
        }

        /**
         * @brief Get column by name, and trim whitespace from column names, unlike GetColumnByName().
         * @param name
         * @return
         */
        std::shared_ptr<arrow::ChunkedArray> getColumn(std::shared_ptr<arrow::Table> ptable, std::string name)
        {
            vector<string> colNames = ptable->ColumnNames();

            for (int i = 0; i < (int)colNames.size(); i++)
            {
                string colName = trimSpaces(colNames[i]);

                if (name == colName)
                {
                    return ptable->columns().at(i);
                }
            }

            return nullptr;
        }

        /**
         * @brief Copy values from a column into a vector of int, via cast.
         * For strings this parses decimal or hex via "#ff00ff" or "0xff00ff" syntax.
         * @param col
         * @param values
         * @return Whether the col type could reasonably be converted to int.
         */
        bool getIntValues(std::shared_ptr<arrow::ChunkedArray> col, std::vector<int>& values, int defaultValue)
        {
            values.reserve(col->length());
            auto thisType = col->type()->id();

            for (int ci = 0; ci < col->num_chunks(); ci++)
            {
                if (thisType == arrow::UInt8Type::type_id)
                {
                    auto typeArray = std::static_pointer_cast<arrow::UInt8Array>(col->chunk(ci));
                    int64_t n = typeArray->length();

                    for (int i = 0; i < n; i++)
                    {
                        values.push_back((int)typeArray->Value(i));
                    }
                }
                else if (thisType == arrow::Int16Type::type_id)
                {
                    auto typeArray = std::static_pointer_cast<arrow::Int16Array>(col->chunk(ci));
                    int64_t n = typeArray->length();

                    for (int i = 0; i < n; i++)
                    {
                        values.push_back((int)typeArray->Value(i));
                    }
                }
                else if (thisType == arrow::Int32Type::type_id)
                {
                    auto typeArray = std::static_pointer_cast<arrow::Int32Array>(col->chunk(ci));
                    int64_t n = typeArray->length();

                    for (int i = 0; i < n; i++)
                    {
                        values.push_back(typeArray->Value(i));
                    }
                }
                else if (thisType == arrow::Int64Type::type_id)
                {
                    auto typeArray = std::static_pointer_cast<arrow::Int64Array>(col->chunk(ci));
                    int64_t n = typeArray->length();

                    for (int i = 0; i < n; i++)
                    {
                        values.push_back((int)typeArray->Value(i));
                    }
                }
                else if (thisType == arrow::FloatType::type_id)
                {
                    auto typeArray = std::static_pointer_cast<arrow::FloatArray>(col->chunk(ci));
                    int64_t n = typeArray->length();

                    for (int i = 0; i < n; i++)
                    {
                        values.push_back((int)typeArray->Value(i));
                    }
                }
                else if (thisType == arrow::DoubleType::type_id)
                {
                    auto typeArray = std::static_pointer_cast<arrow::DoubleArray>(col->chunk(ci));
                    int64_t n = typeArray->length();

                    for (int i = 0; i < n; i++)
                    {
                        values.push_back((int)typeArray->Value(i));
                    }
                }
                else if (thisType == arrow::StringType::type_id)
                {
                    auto typeArray = std::static_pointer_cast<arrow::StringArray>(col->chunk(ci));
                    int64_t n = typeArray->length();

                    for (int i = 0; i < n; i++)
                    {
                        string s = (string)typeArray->Value(i);
                        int v = -1;

                        if (s.empty())
                        {
                            v = defaultValue;
                        }
                        else
                        {
                            if (s[0] == '#')
                            {
                                v = stoi(s.substr(1), 0, 16);
                            }
                            else if (s.substr(0, 2) == "0x")
                            {
                                v = stoi(s.substr(2), 0, 16);
                            }
                            else
                            {
                                v = stoi(s);
                            }
                        }

                        values.push_back(v);
                    }
                }
                else
                {
                    return false;
                }
            }

            return true;
        }

        /**
         * @brief Copy values from a column into a vector of float, via cast.
         * @param col
         * @param values
         * @return Whether the col type could reasonably be converted to float.
         */
        bool getFloatValues(std::shared_ptr<arrow::ChunkedArray> col, std::vector<float>& values)
        {
            values.reserve(col->length());
            auto thisType = col->type()->id();

            for (int ci = 0; ci < col->num_chunks(); ci++)
            {
                if (thisType == arrow::FloatType::type_id)
                {
                    auto typeArray = std::static_pointer_cast<arrow::FloatArray>(col->chunk(ci));
                    int64_t n = typeArray->length();

                    for (int i = 0; i < n; i++)
                    {
                        values.push_back((float)typeArray->Value(i));
                    }
                }
                else if (thisType == arrow::DoubleType::type_id)
                {
                    auto typeArray = std::static_pointer_cast<arrow::DoubleArray>(col->chunk(ci));
                    int64_t n = typeArray->length();

                    for (int i = 0; i < n; i++)
                    {
                        values.push_back((float)typeArray->Value(i));
                    }
                }
                else if (thisType == arrow::HalfFloatType::type_id)
                {
                    auto typeArray = std::static_pointer_cast<arrow::HalfFloatArray>(col->chunk(ci));
                    int64_t n = typeArray->length();

                    for (int i = 0; i < n; i++)
                    {
                        values.push_back((float)typeArray->Value(i));
                    }
                }
                else if (thisType == arrow::UInt8Type::type_id)
                {
                    auto typeArray = std::static_pointer_cast<arrow::UInt8Array>(col->chunk(ci));
                    int64_t n = typeArray->length();

                    for (int i = 0; i < n; i++)
                    {
                        values.push_back((float)typeArray->Value(i));
                    }
                }
                else if (thisType == arrow::Int16Type::type_id)
                {
                    auto typeArray = std::static_pointer_cast<arrow::Int16Array>(col->chunk(ci));
                    int64_t n = typeArray->length();

                    for (int i = 0; i < n; i++)
                    {
                        values.push_back((float)typeArray->Value(i));
                    }
                }
                else if (thisType == arrow::Int32Type::type_id)
                {
                    auto typeArray = std::static_pointer_cast<arrow::Int32Array>(col->chunk(ci));
                    int64_t n = typeArray->length();

                    for (int i = 0; i < n; i++)
                    {
                        values.push_back((float)typeArray->Value(i));
                    }
                }
                else if (thisType == arrow::Int64Type::type_id)
                {
                    auto typeArray = std::static_pointer_cast<arrow::Int64Array>(col->chunk(ci));
                    int64_t n = typeArray->length();

                    for (int i = 0; i < n; i++)
                    {
                        values.push_back((float)typeArray->Value(i));
                    }
                }
                else if (thisType == arrow::BooleanType::type_id)
                {
                    auto typeArray = std::static_pointer_cast<arrow::BooleanArray>(col->chunk(ci));
                    int64_t n = typeArray->length();

                    for (int i = 0; i < n; i++)
                    {
                        values.push_back(typeArray->Value(i) ? 1.0f : 0.0f);
                    }
                }
                else
                {
                    return false;
                }
            }

            return true;
        }

        std::shared_ptr<arrow::Array> buildInt32Array(const std::vector<int>& values)
        {
            arrow::Int32Builder builder;

            if (!builder.AppendValues(values).ok())
            {
                bail("Failed to append values to build array.");
            }

            return builder.Finish().ValueOrDie();
        }

        std::shared_ptr<arrow::Array> buildBoolArray(const std::vector<bool>& values)
        {
            arrow::BooleanBuilder builder;

            if (!builder.AppendValues(values).ok())
            {
                bail("Failed to append values to build array.");
            }

            return builder.Finish().ValueOrDie();
        }

        std::shared_ptr<arrow::Array> buildFloatArray(const std::vector<float>& values)
        {
            arrow::FloatBuilder builder;

            if (!builder.AppendValues(values).ok())
            {
                bail("Failed to append values to build array.");
            }

            return builder.Finish().ValueOrDie();
        }

        std::shared_ptr<arrow::Array> buildDoubleArray(const std::vector<double>& values)
        {
            arrow::DoubleBuilder builder;

            if (!builder.AppendValues(values).ok())
            {
                bail("Failed to append values to build array.");
            }

            return builder.Finish().ValueOrDie();
        }

        std::shared_ptr<arrow::Array> buildStringArray(const std::vector<std::string>& values)
        {
            arrow::StringBuilder builder;

            if (!builder.AppendValues(values).ok())
            {
                bail("Failed to append values to build array.");
            }

            return builder.Finish().ValueOrDie();
        }

        /**
         * @brief Create a test table with some variety of col types and values.
         * @param nRows
         * @return
         */
        std::shared_ptr<arrow::Table> createTestTable(int nRows)
        {
            // int array
            std::vector<int> intValues;

            for (int i = 0; i < nRows; i++)
            {
                intValues.push_back(i);
            }

            auto intArray = buildInt32Array(intValues);

            // bool array
            std::vector<bool> boolValues;

            for (int i = 0; i < nRows; i++)
            {
                boolValues.push_back(i % 2 == 0);
            }

            auto boolArray = buildBoolArray(boolValues);

            // float array
            std::vector<float> floatValues;

            for (int i = 0; i < nRows; i++)
            {
                floatValues.push_back((float)i / 10.0f);
            }

            auto floatArray = buildFloatArray(floatValues);

            // double array
            std::vector<double> doubleValues;

            for (int i = 0; i < nRows; i++)
            {
                doubleValues.push_back((double)i / 10.0f);
            }

            auto doubleArray = buildDoubleArray(doubleValues);

            // string array
            std::vector<string> stringValues;

            for (int i = 0; i < nRows; i++)
            {
                stringValues.push_back(fmt::format("i={}_unicΩode", i));
            }

            auto stringArray = buildStringArray(stringValues);

            // schema
            auto schema = arrow::schema({arrow::field("int32s", arrow::int32()), arrow::field("float32s", arrow::float32()),
                arrow::field("float64s", arrow::float64()), arrow::field("unicΩodeFloat64s", arrow::float64()),
                arrow::field("strings", arrow::utf8()), arrow::field("bools", arrow::boolean())});

            arrow::ArrayVector arrayVector;
            arrayVector.push_back(intArray);
            arrayVector.push_back(floatArray);
            arrayVector.push_back(doubleArray);
            arrayVector.push_back(doubleArray);
            arrayVector.push_back(stringArray);
            arrayVector.push_back(boolArray);
            std::shared_ptr<arrow::Table> table = arrow::Table::Make(schema, arrayVector);

            return table;
        }

        /**
         * @brief Create a test csv or parquet file.
         * @param outputPath Can be utf-8 on both Windows and Linux.
         */
        void createTestFile(int nRows, const std::string& outputPath)
        {
            std::shared_ptr<arrow::Table> ptable = createTestTable(nRows);
            saveFile(outputPath, ptable);
        }
    }
}
