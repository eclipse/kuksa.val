// http://turtle.sourceforge.net
//
// Copyright Mathieu Champlon 2008
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MOCK_FUNCTION_HPP_INCLUDED
#define MOCK_FUNCTION_HPP_INCLUDED

#include "../config.hpp"
#include "../error.hpp"
#include "../log.hpp"
#include "../constraints.hpp"
#include "../sequence.hpp"
#include "../matcher.hpp"
#include "action.hpp"
#include "verifiable.hpp"
#include "invocation.hpp"
#include "type_name.hpp"
#include "context.hpp"
#include "mutex.hpp"
#include "move_helper.hpp"
#include <boost/preprocessor/iteration/iterate.hpp>
#include <boost/preprocessor/repetition/repeat_from_to.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_binary_params.hpp>
#include <boost/preprocessor/comparison/equal.hpp>
#include <boost/preprocessor/comparison/greater.hpp>
#include <boost/test/utils/basic_cstring/basic_cstring.hpp>
#include <boost/test/utils/lazy_ostream.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/call_traits.hpp>
#include <boost/make_shared.hpp>
#include <boost/noncopyable.hpp>
#include <boost/move/move.hpp>
#include <boost/optional.hpp>
#include <ostream>
#include <vector>
#include <list>

namespace mock
{
namespace detail
{
    template< typename R, typename E >
    struct wrapper_base
    {
        wrapper_base( E& e )
            : e_( &e )
        {}

        template< typename T >
        void returns( T t )
        {
            e_->returns( t );
        }

        E* e_;
    };
    template< typename E >
    struct wrapper_base< void, E >
    {
        wrapper_base( E& e )
            : e_( &e )
        {}

        E* e_;
    };
    template< typename R, typename E >
    struct wrapper_base< R*, E >
    {
        wrapper_base( E& e )
            : e_( &e )
        {}

        void returns( R* r )
        {
            e_->returns( r );
        }
        template< typename Y >
        void returns( const boost::reference_wrapper< Y >& r )
        {
            e_->returns( r );
        }

        E* e_;
    };

    inline int exceptions()
    {
#ifdef MOCK_UNCAUGHT_EXCEPTIONS
        using namespace std;
        return uncaught_exceptions();
#else
        return std::uncaught_exception() ? 1 : 0;
#endif
    }
}
} // mock

#define MOCK_NUM_ARGS 0
#define MOCK_NUM_ARGS_0
#include "function_template.hpp"
#undef MOCK_NUM_ARGS_0
#undef MOCK_NUM_ARGS

#define BOOST_PP_FILENAME_1 <turtle/detail/function_iterate.hpp>
#define BOOST_PP_ITERATION_LIMITS (1, MOCK_MAX_ARGS)
#include BOOST_PP_ITERATE()
#undef BOOST_PP_FILENAME_1
#undef BOOST_PP_ITERATION_LIMITS

#endif // MOCK_FUNCTION_HPP_INCLUDED
