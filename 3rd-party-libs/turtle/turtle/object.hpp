// http://turtle.sourceforge.net
//
// Copyright Mathieu Champlon 2008
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MOCK_OBJECT_HPP_INCLUDED
#define MOCK_OBJECT_HPP_INCLUDED

#include "config.hpp"
#include "detail/root.hpp"
#include "detail/type_name.hpp"
#include "detail/object_impl.hpp"
#include <boost/test/utils/basic_cstring/basic_cstring.hpp>
#include <boost/type_traits/is_base_of.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/make_shared.hpp>
#include <boost/optional.hpp>

namespace mock
{
    class object;

namespace detail
{
    template< typename E >
    E& configure( const object& o, E& e,
        boost::unit_test::const_string instance,
        boost::optional< type_name > type,
        boost::unit_test::const_string name );

    template< typename T, typename E >
    E& configure( const T& t, E& e,
        boost::unit_test::const_string instance,
        boost::optional< type_name > type,
        boost::unit_test::const_string name,
        typename boost::disable_if<
            typename boost::is_base_of< object, T >
        >::type* = 0 )
    {
        e.configure( detail::root, &t, instance, type, name );
        return e;
    }
}
    class object
    {
    public:
        object()
            : impl_( boost::make_shared< detail::object_impl >() )
        {}
    protected:
        ~object()
        {}
    public:
        boost::shared_ptr< detail::object_impl > impl_;
    };

namespace detail
{
    template< typename E >
    E& configure( const object& o, E& e,
                 boost::unit_test::const_string instance,
                 boost::optional< type_name > type,
                 boost::unit_test::const_string name )
    {
        e.configure( *o.impl_, o.impl_.get(), instance, type, name );
        return e;
    }
}
} // mock

#endif // MOCK_OBJECT_HPP_INCLUDED
