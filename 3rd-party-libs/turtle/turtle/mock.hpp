// http://turtle.sourceforge.net
//
// Copyright Mathieu Champlon 2008
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MOCK_MOCK_HPP_INCLUDED
#define MOCK_MOCK_HPP_INCLUDED

#include "config.hpp"
#include "object.hpp"
#include "reset.hpp"
#include "verify.hpp"
#include "cleanup.hpp"
#include "detail/functor.hpp"
#include "detail/function.hpp"
#include "detail/type_name.hpp"
#include "detail/signature.hpp"
#include "detail/parameter.hpp"
#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <boost/utility/identity_type.hpp>
#include <boost/mpl/assert.hpp>

#define MOCK_CLASS(T) \
    struct T : mock::object

#define MOCK_FUNCTION_TYPE(S, tpn) \
    tpn boost::remove_pointer< tpn BOOST_IDENTITY_TYPE(S) >::type

#ifdef MOCK_VARIADIC_MACROS

#define MOCK_BASE_CLASS(T, ...) \
    struct T : __VA_ARGS__, mock::object, mock::detail::base< __VA_ARGS__ >

#define MOCK_FUNCTOR(f, ...) \
    mock::detail::functor< MOCK_FUNCTION_TYPE((__VA_ARGS__),) > f, f##_mock
#define MOCK_FUNCTOR_TPL(f, ...) \
    mock::detail::functor< \
        MOCK_FUNCTION_TYPE((__VA_ARGS__), typename) > f, f##_mock

#else // MOCK_VARIADIC_MACROS

#define MOCK_BASE_CLASS(T, I) \
    struct T : I, mock::object, mock::detail::base< I >

#define MOCK_FUNCTOR(f, S) \
    mock::detail::functor< MOCK_FUNCTION_TYPE((S),) > f, f##_mock
#define MOCK_FUNCTOR_TPL(f, S) \
    mock::detail::functor< \
        MOCK_FUNCTION_TYPE((S), typename) > f, f##_mock

#endif // MOCK_VARIADIC_MACROS

#define MOCK_HELPER(t) \
    t##_mock( mock::detail::root, BOOST_PP_STRINGIZE(t) )
#define MOCK_ANONYMOUS_HELPER(t) \
    t##_mock( mock::detail::root, "?." )

#define MOCK_METHOD_HELPER(S, t, tpn) \
    mutable mock::detail::function< MOCK_FUNCTION_TYPE((S), tpn) > t##_mock_; \
    mock::detail::function< MOCK_FUNCTION_TYPE((S), tpn) >& t##_mock( \
        const mock::detail::context&, \
        boost::unit_test::const_string instance ) const \
    { \
        mock::detail::configure( *this, t##_mock_, \
            instance.substr( 0, instance.rfind( BOOST_PP_STRINGIZE(t) ) ), \
            MOCK_TYPE_NAME(*this), \
            BOOST_PP_STRINGIZE(t) ); \
        return t##_mock_; \
    }

#define MOCK_PARAM(S, tpn) \
    tpn mock::detail::parameter< MOCK_FUNCTION_TYPE((S), tpn)
#define MOCK_DECL_PARAM(z, n, d) \
    BOOST_PP_COMMA_IF(n) d, n >::type p##n
#define MOCK_DECL_PARAMS(n, S, tpn) \
    BOOST_PP_REPEAT(n, MOCK_DECL_PARAM, MOCK_PARAM(S, tpn))
#define MOCK_DECL(M, n, S, c, tpn) \
    tpn boost::function_types::result_type< \
        MOCK_FUNCTION_TYPE((S), tpn) >::type M( \
            MOCK_DECL_PARAMS(n, S, tpn) ) c

#define MOCK_FORWARD_PARAM(z, n, d) \
    BOOST_PP_COMMA_IF(n) d, n >::type >( p##n )
#define MOCK_FORWARD_PARAMS(n, S, tpn) \
    BOOST_PP_REPEAT(n, MOCK_FORWARD_PARAM, \
        mock::detail::move_if_not_lvalue_reference< MOCK_PARAM(S, tpn))
#define MOCK_METHOD_AUX(M, n, S, t, c, tpn) \
    MOCK_DECL(M, n, S, c, tpn) \
    { \
        BOOST_MPL_ASSERT_RELATION( n, ==, \
            boost::function_types::function_arity< \
                MOCK_FUNCTION_TYPE((S), tpn) >::value ); \
        return MOCK_ANONYMOUS_HELPER(t)( \
            MOCK_FORWARD_PARAMS(n, S, tpn) ); \
    }

#define MOCK_METHOD_EXT(M, n, S, t) \
    MOCK_METHOD_AUX(M, n, S, t,,) \
    MOCK_METHOD_AUX(M, n, S, t, const,) \
    MOCK_METHOD_HELPER(S, t,)
#define MOCK_CONST_METHOD_EXT(M, n, S, t) \
    MOCK_METHOD_AUX(M, n, S, t, const,) \
    MOCK_METHOD_HELPER(S, t,)
#define MOCK_NON_CONST_METHOD_EXT(M, n, S, t) \
    MOCK_METHOD_AUX(M, n, S, t,,) \
    MOCK_METHOD_HELPER(S, t,)

#define MOCK_METHOD_EXT_TPL(M, n, S, t) \
    MOCK_METHOD_AUX(M, n, S, t,, typename) \
    MOCK_METHOD_AUX(M, n, S, t, const, typename) \
    MOCK_METHOD_HELPER(S, t, typename)
#define MOCK_CONST_METHOD_EXT_TPL(M, n, S, t) \
    MOCK_METHOD_AUX(M, n, S, t, const, typename) \
    MOCK_METHOD_HELPER(S, t, typename)
#define MOCK_NON_CONST_METHOD_EXT_TPL(M, n, S, t) \
    MOCK_METHOD_AUX(M, n, S, t,, typename) \
    MOCK_METHOD_HELPER(S, t, typename)

#define MOCK_CONVERSION_OPERATOR(M, T, t) \
    M T() const { return MOCK_ANONYMOUS_HELPER(t)(); } \
    M T() { return MOCK_ANONYMOUS_HELPER(t)(); } \
    MOCK_METHOD_HELPER(T(), t,)
#define MOCK_CONST_CONVERSION_OPERATOR(M, T, t) \
    M T() const { return MOCK_ANONYMOUS_HELPER(t)(); } \
    MOCK_METHOD_HELPER(T(), t,)
#define MOCK_NON_CONST_CONVERSION_OPERATOR(M, T, t) \
    M T() { return MOCK_ANONYMOUS_HELPER(t)(); } \
    MOCK_METHOD_HELPER(T(), t,)

#define MOCK_CONVERSION_OPERATOR_TPL(M, T, t) \
    M T() const { return MOCK_ANONYMOUS_HELPER(t)(); } \
    M T() { return MOCK_ANONYMOUS_HELPER(t)(); } \
    MOCK_METHOD_HELPER(T(), t, typename)
#define MOCK_CONST_CONVERSION_OPERATOR_TPL(M, T, t) \
    M T() const { return MOCK_ANONYMOUS_HELPER(t)(); } \
    MOCK_METHOD_HELPER(T(), t, typename)
#define MOCK_NON_CONST_CONVERSION_OPERATOR_TPL(M, T, t) \
    M T() { return MOCK_ANONYMOUS_HELPER(t)(); } \
    MOCK_METHOD_HELPER(T(), t, typename)

#define MOCK_FUNCTION_HELPER(S, t, s, tpn) \
    s mock::detail::function< MOCK_FUNCTION_TYPE((S), tpn) >& t##_mock( \
        mock::detail::context& context, \
        boost::unit_test::const_string instance ) \
    { \
        static mock::detail::function< MOCK_FUNCTION_TYPE((S), tpn) > f; \
        return f( context, instance ); \
    }

#define MOCK_CONSTRUCTOR_AUX(T, n, A, t, tpn) \
    T( MOCK_DECL_PARAMS(n, void A, tpn) ) \
    { \
        MOCK_HELPER(t)( MOCK_FORWARD_PARAMS(n, void A, tpn) ); \
    } \
    MOCK_FUNCTION_HELPER(void A, t, static, tpn)

#define MOCK_CONSTRUCTOR(T, n, A, t) \
    MOCK_CONSTRUCTOR_AUX(T, n, A, t,)
#define MOCK_CONSTRUCTOR_TPL(T, n, A, t) \
    MOCK_CONSTRUCTOR_AUX(T, n, A, t, typename)

#define MOCK_DESTRUCTOR(T, t) \
    T() { try { MOCK_ANONYMOUS_HELPER(t)(); } catch( ... ) {} } \
    MOCK_METHOD_HELPER(void(), t,)

#define MOCK_FUNCTION_AUX(F, n, S, t, s, tpn) \
    MOCK_FUNCTION_HELPER(S, t, s, tpn) \
    s MOCK_DECL(F, n, S,,tpn) \
    { \
        BOOST_MPL_ASSERT_RELATION( n, ==, \
            boost::function_types::function_arity< \
                MOCK_FUNCTION_TYPE((S), tpn) >::value ); \
        return MOCK_HELPER(t)( MOCK_FORWARD_PARAMS(n, S, tpn) ); \
    }

#ifdef MOCK_VARIADIC_MACROS

#define MOCK_VARIADIC_ELEM_0(e0, ...) e0
#define MOCK_VARIADIC_ELEM_1(e0, e1, ...) e1
#define MOCK_VARIADIC_ELEM_2(e0, e1, e2, ...) e2

#define MOCK_METHOD(M, ...) \
    MOCK_METHOD_EXT(M, \
        MOCK_VARIADIC_ELEM_0(__VA_ARGS__, ), \
        MOCK_VARIADIC_ELEM_1(__VA_ARGS__, MOCK_SIGNATURE(M), ), \
        MOCK_VARIADIC_ELEM_2(__VA_ARGS__, M, M, ))
#define MOCK_CONST_METHOD(M, n, ...) \
    MOCK_CONST_METHOD_EXT(M, n, \
        MOCK_VARIADIC_ELEM_0(__VA_ARGS__, ), \
        MOCK_VARIADIC_ELEM_1(__VA_ARGS__, M, ))
#define MOCK_NON_CONST_METHOD(M, n, ...) \
    MOCK_NON_CONST_METHOD_EXT(M, n, \
        MOCK_VARIADIC_ELEM_0(__VA_ARGS__, ), \
        MOCK_VARIADIC_ELEM_1(__VA_ARGS__, M, ))

#define MOCK_METHOD_TPL(M, n, ...) \
    MOCK_METHOD_EXT_TPL(M, n, \
        MOCK_VARIADIC_ELEM_0(__VA_ARGS__, ), \
        MOCK_VARIADIC_ELEM_1(__VA_ARGS__, M, ))
#define MOCK_CONST_METHOD_TPL(M, n, ...) \
    MOCK_CONST_METHOD_EXT_TPL(M, n, \
        MOCK_VARIADIC_ELEM_0(__VA_ARGS__, ), \
        MOCK_VARIADIC_ELEM_1(__VA_ARGS__, M, ))
#define MOCK_NON_CONST_METHOD_TPL(M, n, ...) \
    MOCK_NON_CONST_METHOD_EXT_TPL(M, n, \
        MOCK_VARIADIC_ELEM_0(__VA_ARGS__, ), \
        MOCK_VARIADIC_ELEM_1(__VA_ARGS__, M, ))

#define MOCK_FUNCTION(F, n, ...) \
    MOCK_FUNCTION_AUX(F, n, \
        MOCK_VARIADIC_ELEM_0(__VA_ARGS__, ), \
        MOCK_VARIADIC_ELEM_1(__VA_ARGS__, F, ), \
        inline,)

#define MOCK_STATIC_METHOD(F, n, ...) \
    MOCK_FUNCTION_AUX(F, n, \
        MOCK_VARIADIC_ELEM_0(__VA_ARGS__, ), \
        MOCK_VARIADIC_ELEM_1(__VA_ARGS__, F, ), \
        static,)

#define MOCK_STATIC_METHOD_TPL(F, n, ...) \
    MOCK_FUNCTION_AUX(F, n, \
        MOCK_VARIADIC_ELEM_0(__VA_ARGS__, ), \
        MOCK_VARIADIC_ELEM_1(__VA_ARGS__, F, ), \
        static, typename)

#else // MOCK_VARIADIC_MACROS

#define MOCK_METHOD(M, n) \
    MOCK_METHOD_EXT(M, n, MOCK_SIGNATURE(M), M)

#define MOCK_FUNCTION(F, n, S, t) \
    MOCK_FUNCTION_AUX(F, n, S, t, inline,)

#define MOCK_STATIC_METHOD(F, n, S, t) \
    MOCK_FUNCTION_AUX(F, n, S, t, static,)

#define MOCK_STATIC_METHOD_TPL(F, n, S, t) \
    MOCK_FUNCTION_AUX(F, n, S, t, static, typename)

#endif // MOCK_VARIADIC_MACROS

#define MOCK_EXPECT(t) MOCK_HELPER(t).expect( __FILE__, __LINE__ )
#define MOCK_RESET(t) MOCK_HELPER(t).reset( __FILE__, __LINE__ )
#define MOCK_VERIFY(t) MOCK_HELPER(t).verify( __FILE__, __LINE__ )

#endif // MOCK_MOCK_HPP_INCLUDED
