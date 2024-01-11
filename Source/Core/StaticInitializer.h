#pragma once

////////////////////////////////////////////////////////////////////////////////
//  Headers
////////////////////////////////////////////////////////////////////////////////

#include "PCH.h"
#include "Constants.h"
#include "Preprocessor.h"

namespace StaticInitializer
{
	/**
	 * Implements a static initializer, a helper class used for implementing
	 * the RAII principle. This is essentially a way to ensure that a certain
	 * operation goes off before application startup, before the main function
	 * begins its execution.
	 *
	 * Please note, that although the class may be used on its own, it's
	 * easiest to just rely on the STATIC_INITIALIZER macro.
	 */
	template <typename Function>
	class StaticInitializer
	{
	public:
		/**
		 * Constructs a static initializer, calling the parameter function.
		 */
		explicit StaticInitializer(Function fn)
		{
			fn();
		}
	};

	namespace Detail
	{
		/**
		 * Helper structure for creating static initializer objects without having to
		 * enclose the target lambda in parentheses or explicitly type the parameter of
		 * the object that is to be constructed.
		 */
		struct StaticInitializerFactory
		{
			template <typename Function>
			StaticInitializer<Function> operator<<(Function fn)
			{
				return StaticInitializer<Function>(fn);
			}
		};
	}
}

/**
 * Executes the following block on application start, before the main function
 * begins execution.
 *
 * Note that the order in which these blocks run relative to each other is
 * undefined, and thus it's not safe to assume that block A is executed before
 * block B.
 */
#define STATIC_INITIALIZER() \
    static const auto ANONYMOUS_VARIABLE(StaticInit_) = StaticInitializer::Detail::StaticInitializerFactory() << [&]()