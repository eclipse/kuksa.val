// http://turtle.sourceforge.net
//
// Copyright Mathieu Champlon 2011
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MOCK_GROUP_HPP_INCLUDED
#define MOCK_GROUP_HPP_INCLUDED

#include "../config.hpp"
#include "verifiable.hpp"
#include <functional>
#include <algorithm>
#include <vector>

namespace mock
{
namespace detail
{
    class group
    {
    public:
        void add( verifiable& v )
        {
            verifiables_.push_back( &v );
        }
        void remove( verifiable& v )
        {
            verifiables_.erase(
                std::remove( verifiables_.begin(), verifiables_.end(), &v ),
                verifiables_.end() );
        }

        bool verify() const
        {
            bool valid = true;
            for( verifiables_cit it = verifiables_.begin();
                    it != verifiables_.end(); ++it )
                if( ! (*it)->verify() )
                    valid = false;
            return valid;
        }
        void reset()
        {
            const verifiables_t verifiables = verifiables_;
            for( verifiables_cit it = verifiables.begin();
                it != verifiables.end(); ++it )
                if( std::find( verifiables_.begin(), verifiables_.end(), *it )
                    != verifiables_.end() )
                    (*it)->reset();
        }

    private:
        typedef std::vector< verifiable* > verifiables_t;
        typedef verifiables_t::const_iterator verifiables_cit;

        verifiables_t verifiables_;
    };
}
} // mock

#endif // MOCK_GROUP_HPP_INCLUDED
