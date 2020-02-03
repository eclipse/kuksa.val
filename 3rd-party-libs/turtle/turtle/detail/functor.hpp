// http://turtle.sourceforge.net
//
// Copyright Mathieu Champlon 2012
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MOCK_FUNCTOR_HPP_INCLUDED
#define MOCK_FUNCTOR_HPP_INCLUDED

#include "../config.hpp"
#include "function.hpp"
#include "mutex.hpp"
#include "singleton.hpp"

namespace mock
{
namespace detail
{
    class functor_mutex_t :
        public singleton< functor_mutex_t >,
        public mutex
    {
        MOCK_SINGLETON_CONS( functor_mutex_t );
    };
    MOCK_SINGLETON_INST( functor_mutex )

    template< typename Signature >
    struct functor : function< Signature >
    {
        functor()
        {
            scoped_lock _( functor_mutex );
            static functor* f = 0;
            if( f )
            {
                *this = *f;
                f = 0;
                functor_mutex.unlock();
            }
            else
            {
                functor_mutex.lock();
                f = this;
            }
        }
    };
}
} // mock

#endif // MOCK_FUNCTOR_HPP_INCLUDED
