// http://turtle.sourceforge.net
//
// Copyright Mathieu Champlon 2009
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MOCK_IS_FUNCTOR_HPP_INCLUDED
#define MOCK_IS_FUNCTOR_HPP_INCLUDED

#include "../config.hpp"
#include <boost/function_types/is_callable_builtin.hpp>
#include <boost/type_traits/detail/yes_no_type.hpp>
#include <boost/utility/declval.hpp>
#include <boost/mpl/has_xxx.hpp>
#include <boost/mpl/or.hpp>

namespace mock
{
namespace detail
{
    BOOST_MPL_HAS_XXX_TRAIT_DEF( result_type )
    BOOST_MPL_HAS_XXX_TEMPLATE_DEF( sig )
    BOOST_MPL_HAS_XXX_TEMPLATE_DEF( result )

#ifdef MOCK_DECLTYPE

    template< typename F, typename P >
    struct is_callable
    {
        typedef boost::type_traits::yes_type yes_type;
        typedef boost::type_traits::no_type no_type;

        template< typename T >
        static yes_type check(
           decltype( boost::declval< T >()( boost::declval< P >() ) )* );
        template< typename T >
        static no_type check( ... );

        typedef boost::mpl::bool_<
            sizeof( check< F >( 0 ) ) == sizeof( yes_type ) > type;
    };

#endif // MOCK_DECLTYPE

    template< typename T, typename P >
    struct is_functor
        : boost::mpl::or_<
            boost::function_types::is_callable_builtin< T >,
#ifdef MOCK_DECLTYPE
            is_callable< T, P >,
#endif
            has_result_type< T >,
            has_result< T >,
            has_sig< T >
        >
    {};
}
} // mock

#endif // MOCK_IS_FUNCTOR_HPP_INCLUDED
