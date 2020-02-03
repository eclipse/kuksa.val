// http://turtle.sourceforge.net
//
// Copyright Mathieu Champlon 2011
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MOCK_CONTEXT_HPP_INCLUDED
#define MOCK_CONTEXT_HPP_INCLUDED

#include "../config.hpp"
#include "type_name.hpp"
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <boost/test/utils/basic_cstring/basic_cstring.hpp>
#include <ostream>

namespace mock
{
namespace detail
{
    class verifiable;

    class context : boost::noncopyable
    {
    public:
        context() {}
        virtual ~context() {}

        virtual void add( const void* p, verifiable& v,
            boost::unit_test::const_string instance,
            boost::optional< type_name > type,
            boost::unit_test::const_string name ) = 0;
        virtual void add( verifiable& v ) = 0;
        virtual void remove( verifiable& v ) = 0;

        virtual void serialize( std::ostream& s,
            const verifiable& v ) const = 0;
    };
}
} // mock

#endif // MOCK_CONTEXT_HPP_INCLUDED
