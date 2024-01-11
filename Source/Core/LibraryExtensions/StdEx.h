#pragma once

////////////////////////////////////////////////////////////////////////////////
//  Headers
////////////////////////////////////////////////////////////////////////////////

#include "PCH.h"
#include "Core/Constants.h"

////////////////////////////////////////////////////////////////////////////////
/// STD FUNCTIONS
////////////////////////////////////////////////////////////////////////////////
namespace std
{
	////////////////////////////////////////////////////////////////////////////////
	// Represent an INI-esque data structure with key-value pairs organized into blocks.
	using unordered_pairs_in_blocks = map<string, map<string, string>>;
	using ordered_pairs_in_blocks = map<string, vector<pair<string, string>>>;

	////////////////////////////////////////////////////////////////////////////////
	// String type detection helper
	namespace is_string_impl
	{
		template <typename T>       struct is_string:                                         std::false_type {};
		template <>                 struct is_string<char*>:                                  std::true_type {};
		template <typename... Args> struct is_string<std::basic_string            <Args...>>: std::true_type {};
		template <typename... Args> struct is_string<std::basic_string_view       <Args...>>: std::true_type {};
	}

	// type trait to utilize the implementation type traits as well as decay the type
	template <typename T> struct is_string
	{
		static constexpr bool const value = is_string_impl::is_string<std::decay_t<T>>::value;
	};

	////////////////////////////////////////////////////////////////////////////////
	// Iomanip operator detection helper
	namespace is_iomanip_impl
	{
		// type trait to utilize the implementation type traits as well as decay the type
		template <typename T> struct is_iomanip
		{
			//static constexpr bool const value = is_iomanip_impl::is_iomanip<std::decay_t<T>>::value;
			static constexpr bool const value =
				std::is_same<T, decltype(std::setw(0))>::value ||
				std::is_same<T, decltype(std::setbase(0))>::value ||
				std::is_same<T, decltype(std::setiosflags(std::ios::showbase))>::value ||
				std::is_same<T, decltype(std::resetiosflags(std::ios::showbase))>::value;
		};
	}

	// type trait to utilize the implementation type traits as well as decay the type
	template <typename T> struct is_iomanip
	{
		static constexpr bool const value = is_iomanip_impl::is_iomanip<std::decay_t<T>>::value;
	};

	////////////////////////////////////////////////////////////////////////////////
	template<class CharT, class Traits=char_traits<CharT>>
	struct line_iterator
	{
		using string_type = basic_string<CharT, Traits>;

		using iterator_category = input_iterator_tag;
		using difference_type = ptrdiff_t;
		using value_type = string_type;
		using pointer = string_type*;
		using reference = string_type&;

		line_iterator() :
			m_valid(false)
		{}

		line_iterator(string_type const& input, CharT delim = '\n'):
			m_input(input),
			m_delim(delim),
			m_valid(false)
		{
			step_iterator();
		}

		string_type const& operator*() const { return m_line; }

		line_iterator& operator++() { step_iterator(); return *this; }
		line_iterator operator++(int) { line_iterator tmp = *this; step_iterator(); return tmp; }

		friend bool operator== (line_iterator const& a, line_iterator const& b) { return a.m_input == b.m_input && a.m_valid == b.m_valid; };
		friend bool operator!= (line_iterator const& a, line_iterator const& b) { return a.m_input != b.m_input || a.m_valid != b.m_valid; };

	private:
		void step_iterator()
		{
			m_line.erase();

			if (m_input.empty()) { m_valid = false; return; }

			auto dp = m_input.find_first_of(m_delim);
			if (dp == string_type::npos)
			{
				m_line = m_input;
				m_input.erase();
			}
			else
			{
				m_line = m_input.substr(0, dp);
				m_input = m_input.substr(dp + 1);
			}
			m_valid = true;
		}

		string_type m_line;
		string_type m_input;
		CharT m_delim;
		bool m_valid;
	};

	////////////////////////////////////////////////////////////////////////////////
	// Container type detection helper
	namespace is_container_impl
	{
		template <typename T>       struct is_container :std::false_type {};
		template <typename T, std::size_t N> struct is_container<std::array    <T, N>> :std::true_type {};
		template <typename... Args> struct is_container<std::vector            <Args...>> :std::true_type {};
		template <typename... Args> struct is_container<std::deque             <Args...>> :std::true_type {};
		template <typename... Args> struct is_container<std::list              <Args...>> :std::true_type {};
		template <typename... Args> struct is_container<std::forward_list      <Args...>> :std::true_type {};
		template <typename... Args> struct is_container<std::set               <Args...>> :std::true_type {};
		template <typename... Args> struct is_container<std::multiset          <Args...>> :std::true_type {};
		template <typename... Args> struct is_container<std::map               <Args...>> :std::true_type {};
		template <typename... Args> struct is_container<std::multimap          <Args...>> :std::true_type {};
		template <typename... Args> struct is_container<std::unordered_set     <Args...>> :std::true_type {};
		template <typename... Args> struct is_container<std::unordered_multiset<Args...>> :std::true_type {};
		template <typename... Args> struct is_container<std::unordered_map     <Args...>> :std::true_type {};
		template <typename... Args> struct is_container<std::unordered_multimap<Args...>> :std::true_type {};
		template <typename... Args> struct is_container<std::stack             <Args...>> :std::true_type {};
		template <typename... Args> struct is_container<std::queue             <Args...>> :std::true_type {};
		template <typename... Args> struct is_container<std::priority_queue    <Args...>> :std::true_type {};
	}

	// type trait to utilize the implementation type traits as well as decay the type
	template <typename T> struct is_container
	{
		static constexpr bool const value = is_container_impl::is_container<std::decay_t<T>>::value;
	};

	////////////////////////////////////////////////////////////////////////////////
	// Iterator detection helper
	template<typename T, typename = void>
	struct is_iterator
	{
		static constexpr bool value = false;
	};

	template<typename T>
	struct is_iterator<T, typename std::enable_if<!std::is_same<typename std::iterator_traits<T>::value_type, void>::value>::type>
	{
		static constexpr bool value = true;
	};

	////////////////////////////////////////////////////////////////////////////////
	// Check for whether a certain type has serialization and deserialization operators
	namespace has_iostream_operator_impl
	{
		// Fallback struct
		struct false_case {}; 
		
		// Default fallback operators 
		// (T1, ..., are dummy arguments to avoid ambiguity in case the actual operator is also a template)
		// This is based on the following rule for overload resolution:
		//   "F1 is determined to be a better function than F2 if implicit conversions for all arguments of F1 
		//    are not worse than the implicit conversions for all arguments of F2, and if F1 and F2 
		//    are both template specializations and F1 is more specialized according to the partial ordering
		//    rules for template specializations"
		// Source: https://en.cppreference.com/w/cpp/language/overload_resolution
		template<typename T, typename T1, typename T2, typename T3, typename T4, typename T5> false_case operator<< (std::ostream& stream, T const&);
		template<typename T, typename T1, typename T2, typename T3, typename T4, typename T5> false_case operator>> (std::istream& stream, T&);

		// Test struct for operator>>(stream, val)
		template<typename T>
		struct test_in_struct
		{
			enum { value = !std::is_same<decltype(std::declval<std::istream&>() >> std::declval<T&>()), false_case>::value };
		};

		// Test struct for operator<<(stream, val)
		template<typename T>
		struct test_out_struct
		{
			enum { value = !std::is_same<decltype(std::declval<std::ostream&>() << std::declval<T const&>()), false_case>::value };
		};
	}

	template<typename T>
	struct has_istream_operator
	{
		static constexpr bool value = has_iostream_operator_impl::test_in_struct<T>::value;
	};

	template<typename T>
	struct has_ostream_operator
	{
		static constexpr bool value = has_iostream_operator_impl::test_out_struct<T>::value;
	};

	////////////////////////////////////////////////////////////////////////////////
	// Check for whether a certain type has serialization and deserialization operators
	namespace is_trivially_callable_impl
	{
		template <typename T>
		constexpr bool is_trivially_callable_impl(typename std::enable_if<bool(sizeof((std::declval<T>()(), 0)))>::type*)
		{
			return true;
		}

		template<typename T>
		constexpr bool is_trivially_callable_impl(...)
		{
			return false;
		}

		template<typename T>
		constexpr bool is_trivially_callable()
		{
			return is_trivially_callable_impl::is_trivially_callable_impl<T>(nullptr);
		}
	}

	template<typename T>
	struct is_trivially_callable
	{
		static constexpr bool value = is_trivially_callable_impl::is_trivially_callable<T>();
	};

	////////////////////////////////////////////////////////////////////////////////
	/** Helper functions to easily access the first and second elements of a pair. */
	const auto pair_first = [](auto const& pair) -> auto const& { return pair.first; };
	const auto pair_second = [](auto const& pair) -> auto const& { return pair.second; };

	////////////////////////////////////////////////////////////////////////////////
	/** ostream operator for iterator pairs. */
	template<typename T>
	std::ostream& printRange(std::ostream& stream, T begin, T end)
	{
		for (T it = begin; it != end; ++it)
		{
			if (it != begin) stream << ",\t";
			stream << (*it);
		}
		return stream;
	}

	////////////////////////////////////////////////////////////////////////////////
	/** ostream operator for pairs. */
	template<typename T1, typename T2>
	std::ostream& operator<<(std::ostream& stream, std::pair<T1, T2> const& container)
	{
		stream << "(" << container.first << ", " << container.second << ")";
		return stream;
	}

	////////////////////////////////////////////////////////////////////////////////
	/** ostream operator for arrays. */
	template<typename T, size_t N>
	std::ostream& operator<<(std::ostream& stream, std::array<T, N> const& container)
	{
		stream << "[";
		printRange(stream, container.begin(), container.end());
		stream << "]";
		return stream;
	}

	////////////////////////////////////////////////////////////////////////////////
	/** ostream operator for vectors. */
	template<typename T>
	std::ostream& operator<<(std::ostream& stream, std::vector<T> const& container)
	{
		stream << "[";
		printRange(stream, container.begin(), container.end());
		stream << "]";
		return stream;
	}

	////////////////////////////////////////////////////////////////////////////////
	/** ostream operator for sets. */
	template<typename T>
	std::ostream& operator<<(std::ostream& stream, std::set<T> const& container)
	{
		stream << "{";
		printRange(stream, container.begin(), container.end());
		stream << "}";
		return stream;
	}

	////////////////////////////////////////////////////////////////////////////////
	/** ostream operator for sets. */
	template<typename T>
	std::ostream& operator<<(std::ostream& stream, std::unordered_set<T> const& container)
	{
		stream << "{";
		printRange(stream, container.begin(), container.end());
		stream << "}";
		return stream;
	}

	////////////////////////////////////////////////////////////////////////////////
	/** ostream operator for maps. */
	template<typename K, typename V>
	std::ostream& operator<<(std::ostream& stream, std::map<K, V> const& container)
	{
		stream << "{";
		printRange(stream, container.begin(), container.end());
		stream << "}";
		return stream;
	}

	////////////////////////////////////////////////////////////////////////////////
	/** ostream operator for maps. */
	template<typename K, typename V>
	std::ostream& operator<<(std::ostream& stream, std::unordered_map<K, V> const& container)
	{
		stream << "{";
		printRange(stream, container.begin(), container.end());
		stream << "}";
		return stream;
	}
	////////////////////////////////////////////////////////////////////////////////
	/** ostream operator for variants. */
	template<typename... Ts, typename std::enable_if<sizeof...(Ts) != 0, int>::type = 0>
	std::ostream& operator<<(std::ostream& stream, std::variant<Ts...> const& variant)
	{
		return stream << std::visit([](auto const& val) { return std::to_string(val); }, variant);
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename T, typename std::enable_if<has_ostream_operator<T>::value, int>::type = 0>
	std::string to_string(T const& val)
	{
		std::stringstream ss;
		ss << val;
		return ss.str();
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename T, typename std::enable_if<has_istream_operator<T>::value, int>::type = 0>
	T from_string(std::string const& str)
	{
		std::istringstream iss(str);
		T result;
		iss >> result;
		return result;
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename Vs, typename Vd>
	Vd get_or(Vs const& variant, Vd def)
	{
		return variant.index() == 0 ? def: std::get<Vd>(variant);
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename T>
	std::vector<T> iota(size_t count, T start = 0)
	{
		std::vector<T> result(count);
		std::iota(result.begin(), result.end(), start);
		return result;
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename T>
	inline T exp256(T x)
	{
		x = 1.0 + x / 256.0;
		x *= x; x *= x; x *= x; x *= x;
		x *= x; x *= x; x *= x; x *= x;
		return x;
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename T>
	inline T exp1024(T x)
	{
		x = 1.0 + x / 1024;
		x *= x; x *= x; x *= x; x *= x;
		x *= x; x *= x; x *= x; x *= x;
		x *= x; x *= x;
		return x;
	}

	////////////////////////////////////////////////////////////////////////////////
	int pow(int base, int exp);

	////////////////////////////////////////////////////////////////////////////////
	/** Function for generating bitmasks from a series of bit ids. */
	template<typename T>
	static constexpr size_t bit_mask(T id)
	{
		return (1 << id);
	}

	////////////////////////////////////////////////////////////////////////////////
	/** Function for generating bitmasks from a series of bit ids. */
	template<typename T>
	static constexpr size_t bit_mask(std::initializer_list<T> ids)
	{
		size_t result = 0;
		for (auto it = ids.begin(); it != ids.end(); ++it)
		{
			result = result | (1 << *it);
		}
		return result;
	}

	////////////////////////////////////////////////////////////////////////////////
	unsigned number_of_digits(unsigned i);

	////////////////////////////////////////////////////////////////////////////////
	// compute the next highest power of 2 of 32-bit v
	size_t next_pow2(size_t v);
	
	////////////////////////////////////////////////////////////////////////////////
	int64_t factorial(int64_t n);

	////////////////////////////////////////////////////////////////////////////////
	double round_to_digits(double val, int accuracy);

	////////////////////////////////////////////////////////////////////////////////
	namespace resize_N_impl
	{
		////////////////////////////////////////////////////////////////////////////////
		template<typename C, typename S>
		void resizeN(C& container, S size)
		{
			container.resize(size);
		}

		////////////////////////////////////////////////////////////////////////////////
		template<typename C, typename S, typename... S_rest>
		void resizeN(C& container, S size, S_rest... sizes)
		{
			container.resize(size);

			for (size_t i = 0; i < size; ++i)
			{
				resizeN(container[i], sizes...);
			}
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename C, typename... S>
	void resizeN(C& container, S... sizes)
	{
		resize_N_impl::resizeN(container, sizes...);
	}

	////////////////////////////////////////////////////////////////////////////////
	std::vector<const char*> to_cstr(std::vector<std::string> const& values);

	////////////////////////////////////////////////////////////////////////////////
	template<typename T>
	std::vector<std::string> meta_enum_names(T const& meta)
	{
		std::vector<std::string> result(meta.members.size());

		for (size_t i = 0; i < result.size(); ++i)
		{
			result[i] = std::string(meta.members[i].name.begin(), meta.members[i].name.end());
		}

		return result;
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename T>
	std::vector<std::string> meta_enum_names(T const& meta, std::vector<typename T::EnumType> const& enums)
	{
		std::vector<std::string> result(enums.size());

		for (size_t i = 0; i < result.size(); ++i)
		for (size_t j = 0; j < meta.members.size(); ++j)
		{
			if (meta.members[j].value == enums[i])
			{
				result[i] = std::string(meta.members[j].name.begin(), meta.members[j].name.end());
				break;
			}
		}

		return result;
	}

	////////////////////////////////////////////////////////////////////////////////
	inline bool string_replace_first(std::string& str, std::string const& from, std::string const& to)
	{
		size_t start_pos = str.find(from);
		if (start_pos == std::string::npos) return false;
		str.replace(start_pos, from.length(), to);
		return true;
	}

	////////////////////////////////////////////////////////////////////////////////
	inline std::string string_replace_first(std::string const& str, std::string const& from, std::string const& to)
	{
		std::string result = str;
		string_replace_first(result, from, to);
		return result;
	}

	////////////////////////////////////////////////////////////////////////////////
	inline void string_replace_all(std::string& str, std::string const& from, std::string const& to)
	{
		if (from.empty()) return;

		size_t start_pos = 0;
		while ((start_pos = str.find(from, start_pos)) != std::string::npos)
		{
			str.replace(start_pos, from.length(), to);
			start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	inline std::string string_replace_all(std::string const& str, std::string const& from, std::string const& to)
	{
		std::string result = str;
		string_replace_all(result, from, to);
		return result;
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename It, typename T>
	inline std::string string_join(std::string const& separator, It begin, It end, T const& transform)
	{
		// Make sure we are not dealing with an empty range
		if (begin == end) return std::string();

		// Add the first element
		stringstream ss;
		ss << transform(*begin);
		++begin;

		// Append the tail of the list
		for (; begin != end; ++begin)
			ss << separator << transform(*begin);

		// Return the result
		return ss.str();
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename It>
	inline std::string string_join(std::string const& separator, It begin, It end)
	{
		// Make sure we are not dealing with an empty range
		if (begin == end) return std::string();

		// Add the first element
		stringstream ss;
		ss << *begin;
		++begin;

		// Append the tail of the list
		for (; begin != end; ++begin)
			ss << separator << *begin;

		// Return the result
		return ss.str();
	}

	////////////////////////////////////////////////////////////////////////////////
	inline std::string string_erase_all(std::string const& string, std::string const& chars)
	{
		std::string result = string;
		for (const char ch: chars)
			result.erase(std::remove(result.begin(), result.end(), ch), result.end());
		return result;
	}

	////////////////////////////////////////////////////////////////////////////////
	inline void string_erase_all(std::string& string, std::string const& chars)
	{
		for (const char ch : chars)
			string.erase(std::remove(string.begin(), string.end(), ch), string.end());
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename T>
	void atomic_min(std::atomic<T>& min_value, T value)
	{
		T prev_value = min_value;
		while (prev_value > value && min_value.compare_exchange_weak(prev_value, value) == false)
			;
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename T>
	void atomic_max(std::atomic<T>& max_value, T value)
	{
		T prev_value = max_value;
		while (prev_value < value && max_value.compare_exchange_weak(prev_value, value) == false)
			;
	}

	////////////////////////////////////////////////////////////////////////////////
	// trim from start (in place)
	static inline void ltrim(std::string& s)
	{
		s.erase(s.begin(), std::find_if(s.begin(), s.end(), 
			[](unsigned char ch) { return !std::isspace(ch); }));
	}

	////////////////////////////////////////////////////////////////////////////////
	// trim from end (in place)
	static inline void rtrim(std::string& s)
	{
		s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
	}

	////////////////////////////////////////////////////////////////////////////////
	// trim from both ends (in place)
	static inline void trim(std::string& s)
	{
		ltrim(s);
		rtrim(s);
	}

	////////////////////////////////////////////////////////////////////////////////
	// trim from start (copying)
	static inline std::string ltrim_copy(std::string s)
	{
		ltrim(s);
		return s;
	}

	////////////////////////////////////////////////////////////////////////////////
	// trim from end (copying)
	static inline std::string rtrim_copy(std::string s)
	{
		rtrim(s);
		return s;
	}

	////////////////////////////////////////////////////////////////////////////////
	// trim from both ends (copying)
	static inline std::string trim_copy(std::string s)
	{
		trim(s);
		return s;
	}

	////////////////////////////////////////////////////////////////////////////////
	// size of a stringstream
	static inline size_t stringstream_length(std::stringstream& ss)
	{
		ss.seekg(0, ios::end);
		const size_t ssl = size_t(ss.tellg());
		ss.seekg(0, ios::beg);
		return ssl;
	}
}