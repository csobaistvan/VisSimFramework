#pragma once

////////////////////////////////////////////////////////////////////////////////
//  Headers
////////////////////////////////////////////////////////////////////////////////

#include "PCH.h"
#include "Core/Constants.h"

////////////////////////////////////////////////////////////////////////////////
/// Glm extensions
////////////////////////////////////////////////////////////////////////////////
namespace glm
{
	////////////////////////////////////////////////////////////////////////////////
	template<typename T>
	glm::tvec2<T> fromComplex(std::complex<T> c)
	{
		return glm::tvec2<T>(c.real(), c.imag());
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename T>
	std::complex<T> toComplex(glm::tvec2<T> v)
	{
		return std::complex<T>(v.x, v.y);
	}

	////////////////////////////////////////////////////////////////////////////////
	namespace stream_operators_impl
	{
		////////////////////////////////////////////////////////////////////////////////
		//  Generic output operator
		template<typename T>
		std::ostream& out_operator(std::ostream& stream, std::string const& prefix, size_t precision, size_t numElements, T const& v)
		{
			stream << prefix << "(";
			for (size_t i = 0; i < numElements; ++i)
			{
				stream << std::setprecision(precision) << glm::value_ptr(v)[i];
				if (i < numElements - 1) stream << ", ";
			}
			return stream << ")";
		}

		////////////////////////////////////////////////////////////////////////////////
		//  Generic input operator
		template<typename T>
		std::istream& in_operator(std::istream& stream, std::string const& prefix, size_t precision, size_t numElements, T& v)
		{
			char ch = 0;

			// Eat the prefix
			while (stream >> ch && prefix.find(ch) != std::string::npos)
				;

			// Make sure we read the opening parenthesis
			if (ch != '(')
			{
				stream.putback(ch);
				stream.setstate(std::ios_base::failbit);
				return stream;
			}

			// Read each element
			for (size_t i = 0; i < numElements; ++i)
			{
				stream >> glm::value_ptr(v)[i] >> ch;
				if (i < numElements - 1 && ch != ',')
				{
					stream.putback(ch);
					stream.setstate(std::ios_base::failbit);
					return stream;
				}
			}

			// Make sure we read the closing parenthesis
			if (ch != ')')
			{
				stream.putback(ch);
				stream.setstate(std::ios_base::failbit);
				return stream;
			}

			return stream;
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	namespace hash_impl
	{
		////////////////////////////////////////////////////////////////////////////////
		//  Generic hash operator
		template<typename T, typename V>
		std::size_t hash(const V& v, const size_t dims)
		{
			size_t result = 0;
			for (size_t i = 0; i < dims; ++i)
				result ^= std::hash<T>{}(v[i]);
			return result;
		}
	}
}

namespace std
{
	////////////////////////////////////////////////////////////////////////////////
	template<typename T>
	struct std::hash<glm::tvec1<T>>
	{
		std::size_t operator()(glm::tvec1<T> const& v) const noexcept
		{
			return glm::hash_impl::hash< T, glm::tvec1<T>>(v, 1);
		}
	};

	////////////////////////////////////////////////////////////////////////////////
	template<typename T>
	struct std::hash<glm::tvec2<T>>
	{
		std::size_t operator()(glm::tvec2<T> const& v) const noexcept
		{
			return glm::hash_impl::hash< T, glm::tvec2<T>>(v, 2);
		}
	};

	////////////////////////////////////////////////////////////////////////////////
	template<typename T>
	struct std::hash<glm::tvec3<T>>
	{
		std::size_t operator()(glm::tvec3<T> const& v) const noexcept
		{
			return glm::hash_impl::hash< T, glm::tvec3<T>>(v, 3);
		}
	};

	////////////////////////////////////////////////////////////////////////////////
	template<typename T>
	struct std::hash<glm::tvec4<T>>
	{
		std::size_t operator()(glm::tvec4<T> const& v) const noexcept
		{
			return glm::hash_impl::hash< T, glm::tvec4<T>>(v, 4);
		}
	};

	////////////////////////////////////////////////////////////////////////////////
	template<typename T>
	std::ostream& operator<<(std::ostream& stream, glm::tvec1<T> const& v)
	{
		return glm::stream_operators_impl::out_operator(stream, "vec1", Constants::VEC_PRECISION, 1, v);
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename T>
	std::ostream& operator<<(std::ostream& stream, glm::tvec2<T> const& v)
	{
		return glm::stream_operators_impl::out_operator(stream, "vec2", Constants::VEC_PRECISION, 2, v);
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename T>
	std::ostream& operator<<(std::ostream& stream, glm::tvec3<T> const& v)
	{
		return glm::stream_operators_impl::out_operator(stream, "vec3", Constants::VEC_PRECISION, 3, v);
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename T>
	std::ostream& operator<<(std::ostream& stream, glm::tvec4<T> const& v)
	{
		return glm::stream_operators_impl::out_operator(stream, "vec4", Constants::VEC_PRECISION, 4, v);
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename T>
	std::ostream& operator<<(std::ostream& stream, glm::tmat2x2<T> const& v)
	{
		return glm::stream_operators_impl::out_operator(stream, "mat2", Constants::MAT_PRECISION, 2 * 2, v);
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename T>
	std::ostream& operator<<(std::ostream& stream, glm::tmat3x3<T> const& v)
	{
		return glm::stream_operators_impl::out_operator(stream, "mat3", Constants::MAT_PRECISION, 3 * 3, v);
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename T>
	std::ostream& operator<<(std::ostream& stream, glm::tmat4x4<T> const& v)
	{
		return glm::stream_operators_impl::out_operator(stream, "mat4", Constants::MAT_PRECISION, 4 * 4, v);
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename T>
	std::istream& operator>>(std::istream& stream, glm::tvec1<T>& v)
	{
		return glm::stream_operators_impl::in_operator(stream, "vec1", Constants::VEC_PRECISION, 1, v);
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename T>
	std::istream& operator>>(std::istream& stream, glm::tvec2<T>& v)
	{
		return glm::stream_operators_impl::in_operator(stream, "vec2", Constants::VEC_PRECISION, 2, v);
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename T>
	std::istream& operator>>(std::istream& stream, glm::tvec3<T>& v)
	{
		return glm::stream_operators_impl::in_operator(stream, "vec3", Constants::VEC_PRECISION, 3, v);
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename T>
	std::istream& operator>>(std::istream& stream, glm::tvec4<T>& v)
	{
		return glm::stream_operators_impl::in_operator(stream, "vec4", Constants::VEC_PRECISION, 4, v);
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename T>
	std::istream& operator>>(std::istream& stream, glm::tmat2x2<T>& v)
	{
		return glm::stream_operators_impl::in_operator(stream, "mat2", Constants::MAT_PRECISION, 2 * 2, v);
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename T>
	std::istream& operator>>(std::istream& stream, glm::tmat3x3<T>& v)
	{
		return glm::stream_operators_impl::in_operator(stream, "mat3", Constants::MAT_PRECISION, 3 * 3, v);
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename T>
	std::istream& operator>>(std::istream& stream, glm::tmat4x4<T>& v)
	{
		return glm::stream_operators_impl::in_operator(stream, "mat4", Constants::MAT_PRECISION, 4 * 4, v);
	}
}