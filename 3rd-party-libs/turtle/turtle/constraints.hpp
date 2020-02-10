// http://turtle.sourceforge.net
//
// Copyright Mathieu Champlon 2008
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MOCK_CONSTRAINTS_HPP_INCLUDED
#define MOCK_CONSTRAINTS_HPP_INCLUDED

#include "config.hpp"
#include "constraint.hpp"
#include "detail/addressof.hpp"
#include "detail/move_helper.hpp"
#include <boost/ref.hpp>
#include <boost/version.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/common_type.hpp>
#include <boost/type_traits/is_convertible.hpp>
#include <boost/type_traits/has_equal_to.hpp>
#include <boost/test/floating_point_comparison.hpp>

namespace mock
{
    MOCK_UNARY_CONSTRAINT( any, 0,, ((void)actual, true) )
    MOCK_UNARY_CONSTRAINT( affirm, 0,, !! actual )
    MOCK_UNARY_CONSTRAINT( negate, 0,, ! actual )
    MOCK_UNARY_CONSTRAINT( evaluate, 0,, actual() )

    MOCK_NARY_CONSTRAINT( less, 1, ( expected ), actual < expected )
    MOCK_NARY_CONSTRAINT( greater, 1, ( expected ), actual > expected )
    MOCK_NARY_CONSTRAINT( less_equal, 1, ( expected ), actual <= expected )
    MOCK_NARY_CONSTRAINT( greater_equal, 1, ( expected ), actual >= expected )

#if BOOST_VERSION < 105900

#   define MOCK_SMALL() \
        boost::test_tools::check_is_small( actual, tolerance )
#   define MOCK_PERCENT_TOLERANCE() \
        boost::test_tools::check_is_close( \
            actual, \
            expected, \
            boost::test_tools::percent_tolerance( tolerance ) )
#   define MOCK_FRACTION_TOLERANCE() \
        boost::test_tools::check_is_close( \
            actual, \
            expected, \
            boost::test_tools::fraction_tolerance( tolerance ) )

#else // BOOST_VERSION < 105900

namespace detail
{
    template< typename T, typename Tolerance >
    bool is_small( const T& t, const Tolerance& tolerance )
    {
        return boost::math::fpc::small_with_tolerance< T >( tolerance )( t );
    }

    template< typename T1, typename T2, typename Tolerance >
    bool is_close( const T1& t1, const T2& t2, const Tolerance& tolerance )
    {
        typedef typename boost::common_type< T1, T2 >::type common_type;
        return boost::math::fpc::close_at_tolerance< common_type >(
            tolerance, boost::math::fpc::FPC_STRONG )( t1, t2 );
    }
}

#   define MOCK_SMALL() \
        detail::is_small( actual, tolerance )
#   define MOCK_PERCENT_TOLERANCE() \
        detail::is_close( actual, expected, \
            boost::math::fpc::percent_tolerance( tolerance ) )
#   define MOCK_FRACTION_TOLERANCE() \
        detail::is_close( actual, expected, tolerance )

#endif // BOOST_VERSION < 105900

#ifdef small
#   pragma push_macro( "small" )
#   undef small
#   define MOCK_SMALL_DEFINED
#endif
    MOCK_NARY_CONSTRAINT( small, 1, ( tolerance ),
        ( MOCK_SMALL() ) )
#ifdef MOCK_SMALL_DEFINED
#   pragma pop_macro( "small" )
#endif

    MOCK_NARY_CONSTRAINT( close, 2, ( expected, tolerance ),
        ( MOCK_PERCENT_TOLERANCE() ) )

    MOCK_NARY_CONSTRAINT( close_fraction, 2, ( expected, tolerance ),
        ( MOCK_FRACTION_TOLERANCE() ) )

#undef MOCK_PERCENT_TOLERANCE
#undef MOCK_FRACTION_TOLERANCE

#ifdef near
#   pragma push_macro( "near" )
#   undef near
#   define MOCK_NEAR_DEFINED
#endif
    MOCK_NARY_CONSTRAINT( near, 2, ( expected, tolerance ),
        std::abs( actual - expected ) < tolerance )
#ifdef MOCK_NEAR_DEFINED
#   pragma pop_macro( "near" )
#endif

namespace detail
{
    template< typename Expected >
    struct equal
    {
        explicit equal( Expected expected )
            : expected_( expected )
        {}
        template< typename Actual >
        bool operator()( const Actual& actual,
            typename boost::enable_if<
                boost::has_equal_to<
                    Actual,
                    typename
                        boost::unwrap_reference< Expected >::type
                >
            >::type* = 0 ) const
        {
            return actual == boost::unwrap_ref( expected_ );
        }
        template< typename Actual >
        bool operator()( const Actual& actual,
            typename boost::disable_if<
                boost::has_equal_to<
                    Actual,
                    typename
                        boost::unwrap_reference< Expected >::type
                >
            >::type* = 0 ) const
        {
            return actual && *actual == boost::unwrap_ref( expected_ );
        }
        friend std::ostream& operator<<( std::ostream& s, const equal& e )
        {
            return s << "equal( " << mock::format( e.expected_ ) << " )";
        }
        Expected expected_;
    };

    template< typename Expected >
    struct same
    {
        explicit same( const Expected& expected )
            : expected_( detail::addressof( boost::unwrap_ref( expected ) ) )
        {}
        template< typename Actual >
        bool operator()( const Actual& actual ) const
        {
            return detail::addressof( actual ) == expected_;
        }
        friend std::ostream& operator<<( std::ostream& os, const same& s )
        {
            return os << "same( " << mock::format( *s.expected_ ) << " )";
        }
        const typename
            boost::unwrap_reference< Expected >::type* expected_;
    };

    template< typename Expected >
    struct retrieve
    {
        explicit retrieve( Expected& expected )
            : expected_( detail::addressof( boost::unwrap_ref( expected ) ) )
        {}
        template< typename Actual >
        bool operator()( const Actual& actual,
            typename boost::disable_if<
                boost::is_convertible<
                    const Actual*,
                    typename
                        boost::unwrap_reference< Expected >::type
                >
            >::type* = 0 ) const
        {
            *expected_ = actual;
            return true;
        }
        template< typename Actual >
        bool operator()( BOOST_RV_REF(Actual) actual,
            typename boost::disable_if<
                boost::is_convertible<
                    const Actual*,
                    typename
                        boost::unwrap_reference< Expected >::type
                >
            >::type* = 0 ) const
        {
            *expected_ = boost::move( actual );
            return true;
        }
        template< typename Actual >
        bool operator()( Actual& actual,
            typename boost::enable_if<
                boost::is_convertible< Actual*,
                    typename
                        boost::unwrap_reference< Expected >::type
                >
            >::type* = 0 ) const
        {
            *expected_ = detail::addressof( actual );
            return true;
        }
        friend std::ostream& operator<<( std::ostream& s, const retrieve& r )
        {
            return s << "retrieve( " << mock::format( *r.expected_ ) << " )";
        }
        typename
            boost::unwrap_reference< Expected >::type* expected_;
    };

    template< typename Expected >
    struct assign
    {
        explicit assign( const Expected& expected )
            : expected_( expected )
        {}
        template< typename Actual >
        bool operator()( Actual& actual ) const
        {
            actual = boost::unwrap_ref( expected_ );
            return true;
        }
        template< typename Actual >
        bool operator()( Actual* actual,
            typename boost::enable_if<
                boost::is_convertible<
                    typename
                        boost::unwrap_reference< Expected >::type,
                    Actual
                >
            >::type* = 0 ) const
        {
            if( ! actual )
                return false;
            *actual = boost::unwrap_ref( expected_ );
            return true;
        }
        friend std::ostream& operator<<( std::ostream& s, const assign& a )
        {
            return s << "assign( " << mock::format( a.expected_ ) << " )";
        }
        Expected expected_;
    };

    template< typename Expected >
    struct contain
    {
        explicit contain( const Expected& expected )
            : expected_( expected )
        {}
        bool operator()( const std::string& actual ) const
        {
            return actual.find( boost::unwrap_ref( expected_ ) )
                != std::string::npos;
        }
        friend std::ostream& operator<<( std::ostream& s, const contain& n )
        {
            return s << "contain( " << mock::format( n.expected_ ) << " )";
        }
        Expected expected_;
    };
}

    template< typename T >
    constraint< detail::equal< typename detail::forward_type< T >::type > > equal( BOOST_FWD_REF(T) t )
    {
        return detail::equal< typename detail::forward_type< T >::type >( boost::forward< T >( t ) );
    }

    template< typename T >
    constraint< detail::same< T > > same( T& t )
    {
        return detail::same< T >( t );
    }
    template< typename T >
    constraint< detail::retrieve< T > > retrieve( T& t )
    {
        return detail::retrieve< T >( t );
    }
    template< typename T >
    constraint< detail::assign< T > > assign( T t )
    {
        return detail::assign< T >( t );
    }
    template< typename T >
    constraint< detail::contain< T > > contain( T t )
    {
        return detail::contain< T >( t );
    }

    template< typename T >
    constraint< T > call( T t )
    {
        return constraint< T >( t );
    }
} // mock

#endif // MOCK_CONSTRAINTS_HPP_INCLUDED
