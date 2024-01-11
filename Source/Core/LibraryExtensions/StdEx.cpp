#include "PCH.h"
#include "StdEx.h"

namespace std
{
	////////////////////////////////////////////////////////////////////////////////
	// compute the next highest power of 2 of 32-bit v
	size_t next_pow2(size_t v)
	{
		--v;
		v |= v >> 1;
		v |= v >> 2;
		v |= v >> 4;
		v |= v >> 8;
		v |= v >> 16;
		return ++v;
	}
	
	////////////////////////////////////////////////////////////////////////////////
	int64_t factorial(int64_t n)
	{
		/* Factorial LUT */
		static int64 s_factorials[] =
		{
			1,
			1,
			2,
			6,
			24,
			120,
			720,
			5040,
			40320,
			362880,
			3628800,
			39916800,
			479001600,
			6227020800,
			87178291200,
			1307674368000,
			20922789888000,
			355687428096000,
			6402373705728000,
			121645100408832000,
			2432902008176640000,
		};
		return s_factorials[n];
	}

	////////////////////////////////////////////////////////////////////////////////
	double round_to_digits(double val, int accuracy)
	{
		static const double s_pow10[] =
		{
			1.0,
			10.0,
			100.0,
			1000.0,
			10000.0,
			100000.0,
			1000000.0,
			10000000.0,
			100000000.0,
			1000000000.0,
			10000000000.0,
			100000000000.0,
			1000000000000.0,
			10000000000000.0,
			100000000000000.0,
			1000000000000000.0,
			10000000000000000.0
		};
		return double(int(val * s_pow10[accuracy])) / s_pow10[accuracy];
	}

	////////////////////////////////////////////////////////////////////////////////
	std::vector<const char*> to_cstr(std::vector<std::string> const& values)
	{
		std::vector<const char*> result(values.size(), nullptr);
		std::transform(values.begin(), values.end(), result.begin(), [](std::string const& val) { return val.c_str(); });
		return result;
	}

	////////////////////////////////////////////////////////////////////////////////
	unsigned number_of_digits(unsigned i)
	{
		return i > 0 ? (int)log10((double)i) + 1 : 1;
	}

	////////////////////////////////////////////////////////////////////////////////
	int pow(int base, int exp)
	{
		//return int(pow(float(base), exp));
		if (exp == 0) return 1;
		if (exp == 1) return base;

		int tmp = pow(base, exp / 2);
		if (exp % 2 == 0) return tmp * tmp;
		else return base * tmp * tmp;
	}
}