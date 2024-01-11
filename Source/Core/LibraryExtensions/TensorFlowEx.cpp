#include "PCH.h"
#include "TensorFlowEx.h"
#include "Core/Debug.h"
#include "Core/EnginePaths.h"
#include "Core/DateTime.h"

namespace TensorFlow
{
    ////////////////////////////////////////////////////////////////////////////////
    DataSample::DataSample()
    {}

    ////////////////////////////////////////////////////////////////////////////////
    DataSample::DataSample(ColumnNames const& columns)
    {
        for (auto const& col : columns)
            m_data[col] = Cell{};
    }

    ////////////////////////////////////////////////////////////////////////////////
    DataSample::DataSample(ColumnNames const& columns, Matrix const& data, size_t rowId)
    {
        for (size_t c = 0; c < columns.size(); ++c)
            m_data[columns[c]] = data(rowId, c);
    }

    ////////////////////////////////////////////////////////////////////////////////
    DataSample::DataSample(Dataset dataset):
        m_data(dataset)
    {}

    ////////////////////////////////////////////////////////////////////////////////
    DataSample::Dataset& DataSample::data()
    {
        return m_data;
    }

    ////////////////////////////////////////////////////////////////////////////////
    DataSample::Dataset const& DataSample::data() const
    {
        return m_data;
    }

    ////////////////////////////////////////////////////////////////////////////////
    DataSample::Cell& DataSample::operator[](std::string const& colName)
    {
        return m_data[colName];
    }

    ////////////////////////////////////////////////////////////////////////////////
    DataSample::Cell const& DataSample::operator[](std::string const& colName) const
    {
        return m_data.find(colName)->second;
    }

    ////////////////////////////////////////////////////////////////////////////////
    // Returns the size of the dataset
    size_t DataSample::size() const
    {
        return m_data.size();
    }

    ////////////////////////////////////////////////////////////////////////////////
    // Checks whether the parameter column is among the data columns
    bool DataSample::hasColumn(std::string const& column) const
    {
        return m_data.find(column) != m_data.end();
    }

    ////////////////////////////////////////////////////////////////////////////////
    // Checks whether all the parameter columns are among the data columns
    bool DataSample::hasAllColumns(ColumnNames const& columns) const
    {
        for (auto const& column : columns)
            if (!hasColumn(column))
                return false;
        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////
    // Turns the dataframe to a matrix
    Matrix DataSample::toMatrix(ColumnNames const& columns) const
    {
        // Construct the output matrix
        Matrix result(1, columns.size());

        // Copy over the cells
        for (size_t c = 0; c < result.cols(); ++c)
            result(0, c) = m_data.find(columns[c])->second;

        // Return the resulting matrix
        return result;
    }

    ////////////////////////////////////////////////////////////////////////////////
    std::ostream& operator<<(std::ostream& stream, DataSample const& sample)
    {
        stream << "DataSample{" << std::endl;
        for (auto const& column : sample.data())
            stream << std::string(4, ' ') << column.first << ": " << column.second << std::endl;
        stream << "}";
        return stream;
    }

    ////////////////////////////////////////////////////////////////////////////////
    DataFrame::DataFrame()
    {}

    ////////////////////////////////////////////////////////////////////////////////
    DataFrame::DataFrame(ColumnNames const& columns)
    {
        for (auto const& col : columns)
            m_data[col] = Column{};
    }

    ////////////////////////////////////////////////////////////////////////////////
    DataFrame::DataFrame(ColumnNames const& columns, Matrix const& data)
    {
        for (size_t c = 0; c < columns.size(); ++c)
        {
            Column column(data.rows());
            for (size_t r = 0; r < data.rows(); ++r)
                column[r] = data(r, c);
            m_data[columns[c]] = column;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////
    DataFrame::DataFrame(DataSample const& sample)
    {
        for (auto const& cell : sample.data())
            m_data[cell.first] = Column{ cell.second };
    }

    ////////////////////////////////////////////////////////////////////////////////
    DataFrame::DataFrame(Dataset dataset):
        m_data(dataset)
    {}

    ////////////////////////////////////////////////////////////////////////////////
    DataFrame::Dataset& DataFrame::data()
    {
        return m_data;
    }

    ////////////////////////////////////////////////////////////////////////////////
    DataFrame::Dataset const& DataFrame::data() const
    {
        return m_data;
    }

    ////////////////////////////////////////////////////////////////////////////////
    DataFrame::Column& DataFrame::operator[](std::string const& colName)
    {
        return m_data[colName];
    }

    ////////////////////////////////////////////////////////////////////////////////
    DataFrame::Column const& DataFrame::operator[](std::string const& colName) const
    {
        return m_data.find(colName)->second;
    }

    ////////////////////////////////////////////////////////////////////////////////
    void DataFrame::resize(size_t newSize)
    {
        for (auto& col : m_data)
            col.second.resize(newSize);
    }

    ////////////////////////////////////////////////////////////////////////////////
    void DataFrame::reserve(size_t newSize)
    {
        for (auto& col : m_data)
            col.second.reserve(newSize);
    }

    ////////////////////////////////////////////////////////////////////////////////
    // Returns the shape of the dataset
    std::pair<size_t, size_t> DataFrame::shape() const
    {
        if (m_data.empty()) return std::make_pair(0, 0);
        return std::make_pair(m_data.begin()->second.size(), m_data.size());
    }

    ////////////////////////////////////////////////////////////////////////////////
    // Checks whether the parameter column is among the data columns
    bool DataFrame::hasColumn(std::string const& column) const
    {
        return m_data.find(column) != m_data.end();
    }

    ////////////////////////////////////////////////////////////////////////////////
    // Checks whether all the parameter columns are among the data columns
    bool DataFrame::hasAllColumns(ColumnNames const& columns) const
    {
        for (auto const& column : columns)
            if (!hasColumn(column))
                return false;
        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////
    // Append a sample to the dataframe
    void DataFrame::append(DataSample const& sample)
    {
        for (auto const& cell : sample.data())
            m_data[cell.first].push_back(cell.second);
    }

    ////////////////////////////////////////////////////////////////////////////////
    // Turns the dataframe to a sample
    DataSample DataFrame::toSample(size_t rowId) const
    {
        DataSample result;
        for (auto const& col : m_data)
            result[col.first] = col.second[rowId];
        return result;
    }

    ////////////////////////////////////////////////////////////////////////////////
    // Turns the dataframe to a matrix
    Matrix DataFrame::toMatrix(ColumnNames const& columns) const
    {
        // Construct the output matrix
        auto const& size = shape();
        Matrix result(size.first, columns.size());

        // Copy over the cells
        for (size_t c = 0; c < result.cols(); ++c)
        {
            Column const& column = m_data.find(columns[c])->second;
            for (size_t r = 0; r < result.rows(); ++r)
                result(r, c) = column[r];
        }

        // Return the resulting matrix
        return result;
    }

    ////////////////////////////////////////////////////////////////////////////////
    std::ostream& operator<<(std::ostream& stream, DataFrame const& df)
    {
        stream << "DataFrame{ " << std::endl;
        for (auto const& column : df.data())
        {
            stream << std::string(4, ' ') << column.first << ": [ " << std::endl;
            for (size_t i = 0; i < column.second.size(); ++i)
                stream << std::string(8, ' ') << column.second[i] << ", " << std::endl;
            stream << " ]" << std::endl;
        }
        stream << " }";
        return stream;
    }

    ////////////////////////////////////////////////////////////////////////////////
    #define TF_ERROR_CHECK(status, msg) if (TF_GetCode(status) != TF_OK) { Debug::log_error() << msg << "; cause: " << TF_Message(status) << Debug::end; }

    ////////////////////////////////////////////////////////////////////////////////
    void NoOpDeallocator(void* data, size_t a, void* b) {}

	////////////////////////////////////////////////////////////////////////////////
    std::optional<Model> restoreSavedModel(std::string const& modelName, std::string const& modelFolder, ModelSpec modelSpec)
    {
        Debug::log_trace() << "Loading existing TF SavedModel '" << modelName << " from " << modelFolder << Debug::end;

        // Make sure the model exists
        std::filesystem::path fullModelPath = std::filesystem::path(modelFolder) / "saved_model.pb";
        if (!std::filesystem::exists(fullModelPath))
            return std::optional<Model>();

        // Resulting structure
        Model model;

        // Name of the model
        model.m_modelName = modelName;
        model.m_modelFolder = modelFolder;

        // create a status variable
        TF_Status* status = TF_NewStatus();

        // Create the main computation graph
        model.m_graph = TF_NewGraph();

        // Create session options object
        model.m_sessionOpts = TF_NewSessionOptions();

        // Store the tag
        model.m_tagSet = modelSpec.m_tagSet;
        if (modelSpec.m_tagSet.empty())
        {
            model.m_tagSet = "serve";

            Debug::log_debug() << "No tag-set supplied, using the default tag-set: " << model.m_tagSet << Debug::end;
        }

        // Restore the model
        const char* tags = model.m_tagSet.c_str();
        model.m_session = TF_LoadSessionFromSavedModel(model.m_sessionOpts, nullptr, modelFolder.c_str(), &tags, 1, model.m_graph, NULL, status);
        TF_ERROR_CHECK(status, "Error loading TF model");

        // Store the supplied input and output tensor names
        model.m_inputTensorName = modelSpec.m_inputTensorName;
        model.m_outputTensorName = modelSpec.m_outputTensorName;

        // Extract the list of operations in the graph
        Debug::log_debug() << "Operations found inside the model: " << Debug::end;

        size_t pos = 0;
        TF_Operation* oper;
        while ((oper = TF_GraphNextOperation(model.m_graph, &pos)) != nullptr)
        {
            const std::string operationName = TF_OperationName(oper);
            model.m_operations.emplace_back(operationName);

            Debug::log_debug() << "  - " << operationName << Debug::end;

            // try to find a suitable input tensor, if no specific one was supplied
            if (model.m_inputTensorName.empty() && operationName.find("serving_default_input") != std::string::npos)
            {
                Debug::log_debug() << "No user supplied input tensor; using " << operationName << " as the default input tensor." << Debug::end;

                model.m_inputTensorName = operationName;
            }

            // try to find a suitable output tensor, if no specific one was supplied
            if (model.m_outputTensorName.empty() && operationName.find("StatefulPartitionedCall") != std::string::npos)
            {
                Debug::log_debug() << "No user supplied output tensor; using " << operationName << " as the default output tensor." << Debug::end;

                model.m_outputTensorName = operationName;
            }
        }

        // Extract the input operation
        model.m_inputOp = { TF_GraphOperationByName(model.m_graph, model.m_inputTensorName.c_str()), 0 };
        if (model.m_inputOp.oper == nullptr)
        {
            Debug::log_error() << "Input operation '" << model.m_inputTensorName << "' not found among model operations" << Debug::end;
        }

        // Compute the input dimensions
        TF_GraphGetTensorShape(model.m_graph, model.m_inputOp, model.m_inputDims, 2, status);
        TF_ERROR_CHECK(status, "Error trying to access input tensor shape");

        // Extract the output operation
        model.m_outputOp = { TF_GraphOperationByName(model.m_graph, model.m_outputTensorName.c_str()), 0 };
        if (model.m_outputOp.oper == nullptr)
        {
            Debug::log_error() << "Output operation '" << model.m_outputTensorName << "' not found among model operations" << Debug::end;
        }

        // Compute the output dimensions
        TF_GraphGetTensorShape(model.m_graph, model.m_outputOp, model.m_outputDims, 2, status);
        TF_ERROR_CHECK(status, "Error trying to access input tensor shape");

        // Delete the unnecessary variables
        TF_DeleteStatus(status);

        Debug::log_trace() << "TF SavedModel '" << modelFolder << "' successfully loaded." << Debug::end;

        // Store the input and output column names
        model.m_inputColumnNames = modelSpec.m_inputColumnNames;
        model.m_outputColumnNames = modelSpec.m_outputColumnNames;

        // Try to load the model metadata
        std::filesystem::path metadataPath = std::filesystem::path(modelFolder) / "metadata.json";

        if (std::filesystem::exists(metadataPath))
        {
            Debug::log_trace() << "Attempting to read metadata from: " << metadataPath << Debug::end;

            // Load the metadata contents
            std::ifstream metadataFile(metadataPath.string());
            if (metadataFile.good())
            {
                // Read and parse the metadata file contents
                model.m_metadata = nlohmann::json::parse(metadataFile);

                if (model.m_inputColumnNames.size() > 0 || model.m_outputColumnNames.size() > 0)
                    Debug::log_trace() << "Discarding user-supplied column information in favour of the information from the exported model metadata." << Debug::end;
                else

                // Extract the input and output column names
                model.m_inputColumnNames = model.m_metadata["features"]["names"].get<std::vector<std::string>>();
                model.m_outputColumnNames = model.m_metadata["targets"]["names"].get<std::vector<std::string>>();

                Debug::log_debug() << "Input columns from metadata: " << model.m_inputColumnNames << Debug::end;
                Debug::log_debug() << "Output columns from metadata: " << model.m_outputColumnNames << Debug::end;
            }
            else
            {
                Debug::log_warning() << "Unable to open existing model metadata file at: " << metadataPath << Debug::end;
            }
        }

        // Check if we managed to acquire input column information
        if (model.m_inputColumnNames.empty())
            Debug::log_warning() << "Input column information is empty for TF model " << model.m_modelName << "; only matrix inputs are supported in this mode." << Debug::end;

        // Check if we managed to acquire output column information
        if (model.m_outputColumnNames.empty())
            Debug::log_warning() << "Output column information is empty for TF model " << model.m_modelName << "; only matrix outputs are supported in this mode." << Debug::end;

        // Make sure the input column and input tensor dimensions match
        if (!model.m_inputColumnNames.empty() && model.m_inputColumnNames.size() != model.m_inputDims[1])
            Debug::log_error() << "Number of input columns (" << model.m_inputColumnNames.size() << ") and input tensor columns (" << model.m_inputDims[1] << ") do not match." << Debug::end;

        // Make sure the output column and input tensor dimensions match
        if (!model.m_outputColumnNames.empty() && model.m_outputColumnNames.size() != model.m_outputDims[1])
            Debug::log_error() << "Number of output columns (" << model.m_outputColumnNames.size() << ") and output tensor columns (" << model.m_inputDims[1] << ") do not match." << Debug::end;

        // return the restored model
        return model;
    }

    ////////////////////////////////////////////////////////////////////////////////
    void releaseModel(Model const& model)
    {
        // create a status variable
        TF_Status* status = TF_NewStatus();

        TF_DeleteGraph(model.m_graph);
        TF_DeleteSession(model.m_session, status);
        TF_DeleteSessionOptions(model.m_sessionOpts);

        // Delete the unnecessary variables
        TF_DeleteStatus(status);
    }

    ////////////////////////////////////////////////////////////////////////////////
    Matrix predict(Model const& model, DataSample const& input)
    {
        if (!input.hasAllColumns(model.m_inputColumnNames))
        {
            Debug::log_error() << "Unable to predict; input data missing required input columns." << Debug::end;
        }
        return predict(model, input.toMatrix(model.m_inputColumnNames));
    }

    ////////////////////////////////////////////////////////////////////////////////
    Matrix predict(Model const& model, DataFrame const& input)
    {
        if (!input.hasAllColumns(model.m_inputColumnNames))
        {
            Debug::log_error() << "Unable to predict; input data missing required input columns." << Debug::end;
        }
        return predict(model, input.toMatrix(model.m_inputColumnNames));
    }

    ////////////////////////////////////////////////////////////////////////////////
    Matrix predict(Model const& model, Matrix const& input)
    {
        return predict(model, input.data(), input.rows(), input.cols());
    }

    ////////////////////////////////////////////////////////////////////////////////
    Matrix predict(Model const& model, const float* input, size_t rows, size_t cols)
    {
        DateTime::ScopedTimer timer = DateTime::ScopedTimer(Debug::Debug, rows, DateTime::Microseconds, "Network Prediction");

        Debug::log_trace() << "Predicting via " << model.m_modelName << ", with " << rows << " samples " << Debug::end;

        // create a status variable
        TF_Status* status = TF_NewStatus();
        
        // Compute the input and output dimensions
        int64_t inputDims[2] = { rows, model.m_inputDims[1] };
        int64_t outputDims[2] = { rows, model.m_outputDims[1] };

        if (model.m_inputDims[1] != cols)
        {
            Debug::log_error() << "Error during model predict: input data columns (" << cols << ") differs from model input columns (" << model.m_inputDims[1] << ")" << Debug::end;
        }

        // Convert the input matrix to a TF tensor
        TF_Tensor* inputTensor = TF_NewTensor(TF_FLOAT, inputDims, 2, (void*) input, sizeof(float) * inputDims[0] * inputDims[1], &NoOpDeallocator, 0);

        // Run the Session
        TF_Tensor* outputTensor = nullptr;
        TF_SessionRun(model.m_session, NULL, &model.m_inputOp, &inputTensor, 1, &model.m_outputOp, &outputTensor, 1, NULL, 0, NULL, status);
        TF_ERROR_CHECK(status, "Error during TF session run");

        // Wrap the result in an Eigen matrix
        Matrix result = Matrix::Map((float*)TF_TensorData(outputTensor), outputDims[0], outputDims[1]);

        // Delete the unnecessary variables
        TF_DeleteTensor(outputTensor);
        TF_DeleteStatus(status);

        // Return the result
        return result;
    }

    ////////////////////////////////////////////////////////////////////////////////
    DataFrame predictDf(Model const& model, DataSample const& input)
    {
        if (!input.hasAllColumns(model.m_inputColumnNames))
        {
            Debug::log_error() << "Unable to predict; input data missing required input columns." << Debug::end;
        }
        return predictDf(model, input.toMatrix(model.m_inputColumnNames));
    }

    ////////////////////////////////////////////////////////////////////////////////
    DataFrame predictDf(Model const& model, DataFrame const& input)
    {
        if (!input.hasAllColumns(model.m_inputColumnNames))
        {
            Debug::log_error() << "Unable to predict; input data missing required input columns." << Debug::end;
        }
        return predictDf(model, input.toMatrix(model.m_inputColumnNames));
    }

    ////////////////////////////////////////////////////////////////////////////////
    DataFrame predictDf(Model const& model, Matrix const& input)
    {
        return predictDf(model, input.data(), input.rows(), input.cols());
    }

    ////////////////////////////////////////////////////////////////////////////////
    DataFrame predictDf(Model const& model, const float* input, size_t rows, size_t cols)
    {
        return DataFrame(model.m_outputColumnNames, predict(model, input, rows, cols));
    }
}