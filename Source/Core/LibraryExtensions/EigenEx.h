#pragma once

////////////////////////////////////////////////////////////////////////////////
//  Headers
////////////////////////////////////////////////////////////////////////////////

#include "PCH.h"
#include "Core/Constants.h"

////////////////////////////////////////////////////////////////////////////////
/// CUSTOM EIGEN FUNCTIONS
////////////////////////////////////////////////////////////////////////////////
namespace Eigen
{
	////////////////////////////////////////////////////////////////////////////////
	template<typename T, typename S>
	T resize(T const& m, Vector2<S> size, int interpolation = cv::INTER_LANCZOS4)
	{
		cv::Mat cvIn, cvOut;

		cv::eigen2cv(m, cvIn);
		cv::resize(cvIn, cvOut, cv::Size(size.x(), size.y()), 0, 0, interpolation);

		T result;
		cv::cv2eigen(cvOut, result);

		return result;
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename T>
	T rescale(T const& m, Vector2f f, int interpolation = cv::INTER_LANCZOS4)
	{
		cv::Mat cvIn, cvOut;

		cv::eigen2cv(m, cvIn);
		cv::resize(cvIn, cvOut, cv::Size(), f.x(), f.y(), interpolation);

		T result;
		cv::cv2eigen(cvOut, result);

		return result;
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename T>
	T pad(T const& in, size_t newRows, size_t newCols)
	{
		assert(in.cols() <= newCols && in.rows() <= newRows);
		size_t padCols = newCols - in.cols(), padRows = newRows - in.rows();
		T result = T::Zero(newRows, newCols);
		result.block(padCols / 2, padRows / 2, in.cols(), in.rows()) = in;
		return result;
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename T>
	T fftShift(T const& in)
	{
		// Only handle square matrices
		assert(in.cols() == in.rows());

		// indices of center elements
		int blockSize = in.rows() / 2;

		// Perform the shifts
		T result(in.cols(), in.rows());

		result.block(0, 0, blockSize, blockSize) = in.block(blockSize + 1, blockSize + 1, blockSize, blockSize);
		result.block(blockSize, 0, blockSize + 1, blockSize) = in.block(0, blockSize + 1, blockSize + 1, blockSize);
		result.block(blockSize, blockSize, blockSize + 1, blockSize + 1) = in.block(0, 0, blockSize + 1, blockSize + 1);
		result.block(0, blockSize, blockSize, blockSize + 1) = in.block(blockSize + 1, 0, blockSize, blockSize + 1);

		// Return the result
		return result;
	}
}