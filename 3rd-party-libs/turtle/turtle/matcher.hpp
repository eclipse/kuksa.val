// http://turtle.sourceforge.net
//
// Copyright Mathieu Champlon 2008
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MOCK_MATCHER_HPP_INCLUDED
#define MOCK_MATCHER_HPP_INCLUDED

#include "config.hpp"
#include "log.hpp"
#include "constraints.hpp"
#include "detail/is_functor.hpp"
#include "detail/move_helper.hpp"
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/add_reference.hpp>
#include <boost/ref.hpp>
#include <cstring>

namespace mock
{
    template< typename Actual, typename Expected, typename Enable = void >
    class matcher
    {
    public:
        explicit matcher( Expected expected )
            : expected_( expected )
        {}
        bool operator()( typename boost::add_reference< const Actual >::type actual )
        {
            return mock::equal(
                boost::unwrap_ref( expected_ ) ).c_( actual );
        }
        friend std::ostream& operator<<(
            std::ostream& s, const matcher& m )
        {
            return s << mock::format( m.expected_ );
        }
    private:
        Expected expected_;
    };

    template<>
    class matcher< const char*, const char* >
    {
    public:
        explicit matcher( const char* expected )
            : expected_( expected )
        {}
        bool operator()( const char* actual )
        {
            return std::strcmp( actual, expected_ ) == 0;
        }
        friend std::ostream& operator<<(
            std::ostream& s, const matcher& m )
        {
            return s << mock::format( m.expected_ );
        }
    private:
        const char* expected_;
    };

    template< typename Actual, typename Constraint >
    class matcher< Actual, mock::constraint< Constraint > >
    {
    public:
        explicit matcher( const constraint< Constraint >& c )
            : c_( c.c_ )
        {}
        bool operator()( typename detail::ref_arg< Actual >::type actual )
        {
            return c_( mock::detail::move_if_not_lvalue_reference< typename detail::ref_arg< Actual >::type >( actual ) );
        }
        friend std::ostream& operator<<(
            std::ostream& s, const matcher& m )
        {
            return s << mock::format( m.c_ );
        }
    private:
        Constraint c_;
    };

    template< typename Actual, typename Functor >
    class matcher< Actual, Functor,
        typename boost::enable_if<
            detail::is_functor< Functor, Actual >
        >::type
    >
    {
    public:
        explicit matcher( const Functor& f )
            : c_( f )
        {}
        bool operator()( typename detail::ref_arg< Actual >::type actual )
        {
            return c_( mock::detail::move_if_not_lvalue_reference< typename detail::ref_arg< Actual >::type >( actual ) );
        }
        friend std::ostream& operator<<(
            std::ostream& s, const matcher& m )
        {
            return s << mock::format( m.c_ );
        }
    private:
        Functor c_;
    };
} // mock

#endif // MOCK_MATCHER_HPP_INCLUDED
