// http://turtle.sourceforge.net
//
// Copyright Mathieu Champlon 2014
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MOCK_MUTEX_HPP_INCLUDED
#define MOCK_MUTEX_HPP_INCLUDED

#include "../config.hpp"
#include "singleton.hpp"
#include <boost/move/move.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

#ifdef MOCK_THREAD_SAFE

#ifdef MOCK_HDR_MUTEX
#include <mutex>
#else
#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/lock_guard.hpp>
#endif

namespace mock
{
namespace detail
{
#ifdef MOCK_HDR_MUTEX
    typedef std::recursive_mutex mutex;
    typedef std::lock_guard< mutex > scoped_lock;
#else
    typedef boost::recursive_mutex mutex;
    typedef boost::lock_guard< mutex > scoped_lock;
#endif

    struct lock
    {
    private:
        BOOST_MOVABLE_BUT_NOT_COPYABLE(lock)

    public:
        lock( const boost::shared_ptr< mutex >& m )
            : m_( m )
        {
            m_->lock();
        }
        ~lock()
        {
            if( m_ )
                m_->unlock();
        }
        lock( BOOST_RV_REF( lock ) x )
            : m_( x.m_ )
        {
            // Explicit reset to avoid unlock in destructor
            x.m_.reset();
        }
        lock& operator=( BOOST_RV_REF( lock ) x )
        {
            m_ = x.m_;
            x.m_.reset();
            return *this;
        }

    private:
        boost::shared_ptr< mutex > m_;
    };
}
} // mock

#else // MOCK_THREAD_SAFE

namespace mock
{
namespace detail
{
    struct mutex : boost::noncopyable
    {
        void lock()
        {}
        void unlock()
        {}
    };
    // Dummy lock classes.
    // Constructor + Destructor make it RAII classes for compilers and avoid unused variable warnings
    struct scoped_lock : boost::noncopyable
    {
        scoped_lock( mutex& )
        {}
        ~scoped_lock()
        {}
    };
    class lock : boost::noncopyable
    {
    private:
        BOOST_MOVABLE_BUT_NOT_COPYABLE(lock)

    public:
        lock( const boost::shared_ptr< mutex >& )
        {}
        ~lock()
        {}
        lock( BOOST_RV_REF( lock ) x )
        {}
        lock& operator=( BOOST_RV_REF( lock ) x )
        {
            return *this;
        }
    };
}
} // mock

#endif // MOCK_THREAD_SAFE

namespace mock
{
namespace detail
{
    class error_mutex_t : public singleton< error_mutex_t >,
        public mutex
    {
        MOCK_SINGLETON_CONS( error_mutex_t );
    };
    MOCK_SINGLETON_INST( error_mutex )

#ifdef BOOST_MSVC
#   pragma warning( push )
#   pragma warning( disable: 4702 )
#endif
    template< typename Result, typename Error >
    struct safe_error
    {
        static Result abort()
        {
            scoped_lock _( error_mutex );
            return Error::abort();
        }
        template< typename Context >
        static void fail( const char* message, const Context& context,
            const char* file = "unknown location", int line = 0 )
        {
            scoped_lock _( error_mutex );
            Error::fail( message, context, file, line );
        }
        template< typename Context >
        static void call( const Context& context, const char* file, int line )
        {
            scoped_lock _( error_mutex );
            Error::call( context, file, line );
        }
        static void pass( const char* file, int line )
        {
            scoped_lock _( error_mutex );
            Error::pass( file, line );
        }
    };
#ifdef BOOST_MSVC
#   pragma warning( pop )
#endif
}
} // mock

#endif // MOCK_MUTEX_HPP_INCLUDED
