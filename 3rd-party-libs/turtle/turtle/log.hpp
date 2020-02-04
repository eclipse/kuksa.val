// http://turtle.sourceforge.net
//
// Copyright Mathieu Champlon 2011
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MOCK_LOG_HPP_INCLUDED
#define MOCK_LOG_HPP_INCLUDED

#include "config.hpp"
#include "stream.hpp"
#include "format.hpp"
#include <boost/utility/enable_if.hpp>
#include <boost/detail/container_fwd.hpp>
#include <boost/function_types/is_callable_builtin.hpp>
#include <boost/none.hpp>
#include <memory>

namespace boost
{
    template< typename T > class shared_ptr;
    template< typename T > class weak_ptr;
    template< typename T > class reference_wrapper;
    template< typename T > class optional;

namespace phoenix
{
    template< typename T > struct actor;
}
namespace lambda
{
    template< typename T > class lambda_functor;
}
namespace assign_detail
{
    template< typename T > class generic_list;
}
}

namespace mock
{
namespace detail
{
    template< typename T >
    void serialize( stream& s, const T& begin, const T& end )
    {
        s << '(';
        for( T it = begin; it != end; ++it )
            s << (it == begin ? "" : ",") << mock::format( *it );
        s << ')';
    }
}

#ifdef MOCK_AUTO_PTR
    template< typename T >
    stream& operator<<( stream& s, const std::auto_ptr< T >& t )
    {
        return s << mock::format( t.get() );
    }
#endif // MOCK_AUTO_PTR

    template< typename T1, typename T2 >
    stream& operator<<( stream& s, const std::pair< T1, T2 >& p )
    {
        return s << '(' << mock::format( p.first )
            << ',' << mock::format( p.second ) << ')';
    }

    template< typename T, typename A >
    stream& operator<<( stream& s, const std::deque< T, A >& t )
    {
        detail::serialize( s, t.begin(), t.end() );
        return s;
    }
    template< typename T, typename A >
    stream& operator<<( stream& s, const std::list< T, A >& t )
    {
        detail::serialize( s, t.begin(), t.end() );
        return s;
    }
    template< typename T, typename A >
    stream& operator<<( stream& s, const std::vector< T, A >& t )
    {
        detail::serialize( s, t.begin(), t.end() );
        return s;
    }
    template< typename K, typename T, typename C, typename A >
    stream& operator<<( stream& s, const std::map< K, T, C, A >& t )
    {
        detail::serialize( s, t.begin(), t.end() );
        return s;
    }
    template< typename K, typename T, typename C, typename A >
    stream& operator<<( stream& s, const std::multimap< K, T, C, A >& t )
    {
        detail::serialize( s, t.begin(), t.end() );
        return s;
    }
    template< typename T, typename C, typename A >
    stream& operator<<( stream& s, const std::set< T, C, A >& t )
    {
        detail::serialize( s, t.begin(), t.end() );
        return s;
    }
    template< typename T, typename C, typename A >
    stream& operator<<( stream& s, const std::multiset< T, C, A >& t )
    {
        detail::serialize( s, t.begin(), t.end() );
        return s;
    }
    template< typename T >
    stream& operator<<( stream& s,
        const boost::assign_detail::generic_list< T >& t )
    {
        detail::serialize( s, t.begin(), t.end() );
        return s;
    }
    template< typename T >
    stream& operator<<( stream& s, const boost::reference_wrapper< T >& t )
    {
        return s << mock::format( t.get() );
    }
    template< typename T >
    stream& operator<<( stream& s, const boost::shared_ptr< T >& t )
    {
        return s << mock::format( t.get() );
    }
    template< typename T >
    stream& operator<<( stream& s, const boost::weak_ptr< T >& t )
    {
        return s << mock::format( t.lock() );
    }
    inline stream& operator<<( stream& s, const boost::none_t& )
    {
        return s << "none";
    }
    template< typename T >
    stream& operator<<( stream& s, const boost::optional< T >& t )
    {
        if( t )
            return s << mock::format( t.get() );
        return s << boost::none;
    }

#ifdef MOCK_SMART_PTR
    template< typename T >
    stream& operator<<( stream& s, const std::shared_ptr< T >& t )
    {
        return s << mock::format( t.get() );
    }
    template< typename T >
    stream& operator<<( stream& s, const std::weak_ptr< T >& t )
    {
        return s << mock::format( t.lock() );
    }
    template< typename T, typename D >
    stream& operator<<( stream& s, const std::unique_ptr< T, D >& p )
    {
        return s << mock::format( p.get() );
    }
#endif

    template< typename T >
    stream& operator<<( stream& s, const boost::lambda::lambda_functor< T >& )
    {
        return s << '?';
    }
    template< typename T >
    stream& operator<<( stream& s, const boost::phoenix::actor< T >& )
    {
        return s << '?';
    }

#ifdef MOCK_NULLPTR
    inline stream& operator<<( stream& s, std::nullptr_t )
    {
        return s << "nullptr";
    }
#endif

    template< typename T >
    typename boost::enable_if<
        boost::function_types::is_callable_builtin< T >,
        stream&
    >::type
    operator<<( stream& s, T* )
    {
        return s << '?';
    }
    template< typename T >
    typename boost::disable_if<
        boost::function_types::is_callable_builtin< T >,
        stream&
    >::type
    operator<<( stream& s, T* t )
    {
        *s.s_ << t;
        return s;
    }
} // mock

#endif // MOCK_LOG_HPP_INCLUDED
