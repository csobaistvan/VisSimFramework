#pragma once

////////////////////////////////////////////////////////////////////////////////
//  Headers
////////////////////////////////////////////////////////////////////////////////

#include "PCH.h"
#include "Core/Constants.h"

////////////////////////////////////////////////////////////////////////////////
/// TensorFlow functions
////////////////////////////////////////////////////////////////////////////////
namespace TensorFlow
{
	////////////////////////////////////////////////////////////////////////////////
    /** Represents a row-major matrix, which is also how TF represents tensors internally. */
    using Matrix = Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;

    ////////////////////////////////////////////////////////////////////////////////
    /** Represents a singular data sample object. */
    struct DataSample
    {
        // Various helper typesets
        using Cell = float;
        using ColumnNames = std::vector<std::string>;
        using Dataset = std::unordered_map<std::string, Cell>;

        // Constructors
        DataSample();
        DataSample(ColumnNames const& columns);
        DataSample(ColumnNames const& columns, Matrix const& data, size_t rowId = 0);
        DataSample(Dataset dataset);

        // Data accessor
        Dataset& data();
        Dataset const& data() const;

        // Cell accessor
        Cell& operator[](std::string const& colName);
        Cell const& operator[](std::string const& colName) const;

        // Checks whether the parameter column is among the data columns
        bool hasColumn(std::string const& column) const;

        // Checks whether all the parameter columns are among the data columns
        bool hasAllColumns(ColumnNames const& columns) const;

        // Returns the size of the dataset
        size_t size() const;

        // Turns the dataframe to a matrix
        Matrix toMatrix(ColumnNames const& columns) const;

        // The data storage
        Dataset m_data;
    };

    ////////////////////////////////////////////////////////////////////////////////
    std::ostream& operator<<(std::ostream& stream, DataSample const& sample);

    ////////////////////////////////////////////////////////////////////////////////
    /** Represents a dataframe object. */
    struct DataFrame
    {
        // Various helper typesets
        using Cell = DataSample::Cell;
        using Column = std::vector<Cell>;
        using ColumnNames = DataSample::ColumnNames;
        using Dataset = std::unordered_map<std::string, Column>;

        // Constructors
        DataFrame();
        DataFrame(ColumnNames const& columns);
        DataFrame(ColumnNames const& columns, Matrix const& data);
        DataFrame(DataSample const& sample);
        DataFrame(Dataset dataset);

        // Data accessor
        Dataset& data();
        Dataset const& data() const;

        // Column accessor
        Column& operator[](std::string const& colName);
        Column const& operator[](std::string const& colName) const;

        // Resizes each column to the given size
        void resize(size_t newSize);

        // Reserves space in each column for the given number of samples
        void reserve(size_t newSize);

        // Returns the shape of the dataset
        std::pair<size_t, size_t> shape() const;

        // Checks whether the parameter column is among the data columns
        bool hasColumn(std::string const& column) const;

        // Checks whether all the parameter columns are among the data columns
        bool hasAllColumns(ColumnNames const& columns) const;

        // Append a sample to the dataframe
        void append(DataSample const& sample);

        // Turns the parameter row to a sample
        DataSample toSample(size_t rowId = 0) const;

        // Turns the dataframe to a matrix
        Matrix toMatrix(ColumnNames const& columns) const;

        // The data storage
        Dataset m_data;
    };

    ////////////////////////////////////////////////////////////////////////////////
    std::ostream& operator<<(std::ostream& stream, DataFrame const& df);

    ////////////////////////////////////////////////////////////////////////////////
    struct Model
    {
        // Name and source folder of the model
        std::string m_modelName;
        std::string m_modelFolder;

        // Session options made for creation
        TF_SessionOptions* m_sessionOpts;

        // Main computation graph
        TF_Graph* m_graph;

        // Main session
        TF_Session* m_session;

        // Name of the tag-set to use
        std::string m_tagSet;

        // Network metadata
        nlohmann::json m_metadata;

        // List of operations in the model
        std::vector<std::string> m_operations;

        // Input and output tensor names
        std::string m_inputTensorName;
        std::string m_outputTensorName;

        // Input and output tensors
        TF_Output m_inputOp;
        TF_Output m_outputOp;

        // input and output dimensions
        int64_t m_inputDims[2];
        int64_t m_outputDims[2];

        // Name of the input and output columns
        std::vector<std::string> m_inputColumnNames;
        std::vector<std::string> m_outputColumnNames;
    };

	////////////////////////////////////////////////////////////////////////////////
    struct ModelSpec
    {
        // Name of the tag-set to use
        std::string m_tagSet = "";

        // Input and output tensor names
        std::string m_inputTensorName = "";
        std::string m_outputTensorName = "";

        // Name of the input and output columns
        std::vector<std::string> m_inputColumnNames;
        std::vector<std::string> m_outputColumnNames;
    };

	////////////////////////////////////////////////////////////////////////////////
    std::optional<Model> restoreSavedModel(std::string const& modelName, std::string const& modelFolder, ModelSpec modelSpec);

    ////////////////////////////////////////////////////////////////////////////////
    void releaseModel(Model const& model);

    ////////////////////////////////////////////////////////////////////////////////
    Matrix predict(Model const& model, DataSample const& input);

    ////////////////////////////////////////////////////////////////////////////////
    Matrix predict(Model const& model, DataFrame const& input);

	////////////////////////////////////////////////////////////////////////////////
    Matrix predict(Model const& model, Matrix const& input);

    ////////////////////////////////////////////////////////////////////////////////
    Matrix predict(Model const& model, const float* input, size_t rows, size_t cols);

    ////////////////////////////////////////////////////////////////////////////////
    DataFrame predictDf(Model const& model, DataSample const& input);

    ////////////////////////////////////////////////////////////////////////////////
    DataFrame predictDf(Model const& model, DataFrame const& input);

    ////////////////////////////////////////////////////////////////////////////////
    DataFrame predictDf(Model const& model, Matrix const& input);

    ////////////////////////////////////////////////////////////////////////////////
    DataFrame predictDf(Model const& model, const float* input, size_t rows, size_t cols);
}