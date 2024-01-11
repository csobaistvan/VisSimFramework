#pragma once

////////////////////////////////////////////////////////////////////////////////
//  Headers
////////////////////////////////////////////////////////////////////////////////

#include "PCH.h"
#include "Core/Constants.h"
#include "Core/Debug.h"
#include "TensorFlowEx.h"

////////////////////////////////////////////////////////////////////////////////
/// MATLAB HELPERS
////////////////////////////////////////////////////////////////////////////////
namespace Matlab
{
	#ifdef HAS_Matlab
	////////////////////////////////////////////////////////////////////////////////
	// Global Matlab session
	extern std::unique_ptr<matlab::engine::MATLABEngine> g_matlab;

	////////////////////////////////////////////////////////////////////////////////
	bool matlabEnabled();

	////////////////////////////////////////////////////////////////////////////////
	void initInstance();

	////////////////////////////////////////////////////////////////////////////////
	template<typename Scalar>
	matlab::data::TypedArray<Scalar> imageToDataArray(Scalar* ptr, size_t rows, size_t cols, size_t channels, size_t stride)
	{
		matlab::data::ArrayFactory factory;
		matlab::data::TypedArray<Scalar> result = factory.createArray<Scalar>({ rows, cols, channels });

		for (size_t r = 0; r < rows; ++r)
		for (size_t c = 0; c < cols; ++c)
		for (size_t ch = 0; ch < channels; ++ch)
		{
			result[r][c][ch] = ptr[(r * cols + c) * stride + ch];
		}

		return result;
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename T>
	matlab::data::TypedArray<typename T::Scalar> eigenToDataArray(T const& e)
	{
		using Scalar = typename T::Scalar;

		matlab::data::ArrayFactory factory;
		matlab::data::TypedArray<Scalar> result = factory.createArray<Scalar>({ (size_t)e.rows(), (size_t)e.cols() });

		for (size_t r = 0; r < e.rows(); ++r)
		for (size_t c = 0; c < e.cols(); ++c)
		{
			result[r][c] = e(r, c);
		}

		return result;
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename T>
	matlab::data::TypedArray<T> vectorToDataArray(std::vector<T> const& v)
	{
		matlab::data::ArrayFactory factory;
		matlab::data::TypedArray<T> result = factory.createArray<T>({ (size_t)v.size() });

		for (size_t i = 0; i < v.size(); ++i)
			result[i] = v[i];

		return result;
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename T, typename Q>
	matlab::data::TypedArray<T> vectorToDataArray(std::vector<Q> const& v)
	{
		matlab::data::ArrayFactory factory;
		matlab::data::TypedArray<T> result = factory.createArray<T>({ (size_t)v.size() });

		for (size_t i = 0; i < v.size(); ++i)
			result[i] = T(v[i]);

		return result;
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename Scalar>
	matlab::data::TypedArray<Scalar> scalarToDataArray(Scalar const& e)
	{
		matlab::data::ArrayFactory factory;
		return factory.createArray<Scalar>({ (size_t)1, (size_t)1 }, { e });
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename Scalar>
	matlab::data::TypedArray<Scalar> glmToDataArray(glm::tvec1<Scalar> const& e)
	{
		matlab::data::ArrayFactory factory;
		return factory.createArray<Scalar>({ (size_t)1, (size_t)1 }, { e[0] });
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename Scalar>
	matlab::data::TypedArray<Scalar> glmToDataArray(glm::tvec2<Scalar> const& e)
	{
		matlab::data::ArrayFactory factory;
		return factory.createArray<Scalar>({ (size_t)1, (size_t)2 }, { e[0], e[1] });
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename Scalar>
	matlab::data::TypedArray<Scalar> glmToDataArray(glm::tvec3<Scalar> const& e)
	{
		matlab::data::ArrayFactory factory;
		return factory.createArray<Scalar>({ (size_t)1, (size_t)3 }, { e[0], e[1], e[2] });
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename Scalar>
	matlab::data::TypedArray<Scalar> glmToDataArray(glm::tvec4<Scalar> const& e)
	{
		matlab::data::ArrayFactory factory;
		return factory.createArray<Scalar>({ (size_t)1, (size_t)4 }, { e[0], e[1], e[2], e[3] });
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename Scalar>
	matlab::data::CharArray stringToDataArray(Scalar const& s)
	{
		matlab::data::ArrayFactory factory;
		return factory.createCharArray(s);
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename Scalar>
	matlab::data::CellArray tfDataSampleToCellArray(TensorFlow::DataSample const& datasample)
	{
		matlab::data::ArrayFactory factory;
		matlab::data::CellArray result = factory.createCellArray({ datasample.size(), 2 });

		size_t outId = 0;
		for (auto const& cell : datasample.m_data)
		{
			result[outId][0] = stringToDataArray(cell.first);
			result[outId][1] = scalarToDataArray<Scalar>(cell.second);
			++outId;
		}

		return result;
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename Scalar>
	matlab::data::CellArray tfDataFrameToCellArray(TensorFlow::DataFrame const& dataframe)
	{
		auto shape = dataframe.shape();

		matlab::data::ArrayFactory factory;
		matlab::data::CellArray result = factory.createCellArray({ shape.first, shape.second });

		size_t outId = 0;
		for (auto const& column : dataframe.m_data)
		{
			result[outId][0] = stringToDataArray(column.first);
			for (size_t i = 0; i < column.second.size(); ++i)
				result[outId][i + 1] = scalarToDataArray<Scalar>(column.second[i]);
			++outId;
		}

		return result;
	}

	////////////////////////////////////////////////////////////////////////////////
	struct LoggedStreamBuffer : public std::basic_streambuf<char16_t>
	{
		LoggedStreamBuffer(Debug::DebugOutputLevel logLevel, const char* file, int line, const char* function);

		// Virtual sync function
		virtual std::streamsize xsputn(const char16_t* s, std::streamsize count);

		Debug::DebugOutputLevel m_logLevel;
		const char* m_file;
		int m_line;
		const char* m_function;
		std::string m_buffer;
	};

	////////////////////////////////////////////////////////////////////////////////
	/** Helper for creating logged buffers */
	std::shared_ptr<LoggedStreamBuffer> make_logged_buffer(Debug::DebugOutputLevel logLevel, const char* file, int line, const char* function);

	#define logged_buffer(logLevel) make_logged_buffer(logLevel, __FILE__, __LINE__, __FUNCTION__)
	#endif
}