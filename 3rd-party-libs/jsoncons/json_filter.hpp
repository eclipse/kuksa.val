// Copyright 2013 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_JSON_FILTER_HPP
#define JSONCONS_JSON_FILTER_HPP

#include <string>

#include <jsoncons/json_content_handler.hpp>
#include <jsoncons/parse_error_handler.hpp>

namespace jsoncons {

template <class CharT>
class basic_json_filter : public basic_json_content_handler<CharT>
{
public:
    using typename basic_json_content_handler<CharT>::string_view_type                                 ;
private:
    basic_json_content_handler<CharT>& content_handler_;
    basic_json_content_handler<CharT>& downstream_handler_;

    // noncopyable and nonmoveable
    basic_json_filter<CharT>(const basic_json_filter<CharT>&) = delete;
    basic_json_filter<CharT>& operator=(const basic_json_filter<CharT>&) = delete;
public:
    basic_json_filter(basic_json_content_handler<CharT>& handler)
        : content_handler_(*this),
          downstream_handler_(handler)
    {
    }

#if !defined(JSONCONS_NO_DEPRECATED)
    basic_json_content_handler<CharT>& input_handler()
    {
        return downstream_handler_;
    }
#endif

    basic_json_content_handler<CharT>& downstream_handler()
    {
        return downstream_handler_;
    }

private:
    void do_begin_json() override
    {
        downstream_handler_.begin_json();
    }

    void do_end_json() override
    {
        downstream_handler_.end_json();
    }

    void do_begin_object(const serializing_context& context) override
    {
        downstream_handler_.begin_object(context);
    }

    void do_begin_object(size_t length, const serializing_context& context) override
    {
        downstream_handler_.begin_object(length, context);
    }

    void do_end_object(const serializing_context& context) override
    {
        downstream_handler_.end_object(context);
    }

    void do_begin_array(const serializing_context& context) override
    {
        downstream_handler_.begin_array(context);
    }

    void do_begin_array(size_t length, const serializing_context& context) override
    {
        downstream_handler_.begin_array(length, context);
    }

    void do_end_array(const serializing_context& context) override
    {
        downstream_handler_.end_array(context);
    }

    void do_name(const string_view_type& name,
                 const serializing_context& context) override
    {
        downstream_handler_.name(name,context);
    }

    void do_string_value(const string_view_type& value,
                         const serializing_context& context) override
    {
        downstream_handler_.string_value(value,context);
    }

    void do_byte_string_value(const uint8_t* data, size_t length,
                              const serializing_context& context) override
    {
        downstream_handler_.byte_string_value(data, length, context);
    }

    void do_double_value(double value, const number_format& fmt,
                         const serializing_context& context) override
    {
        downstream_handler_.double_value(value, fmt, context);
    }

    void do_integer_value(int64_t value,
                          const serializing_context& context) override
    {
        downstream_handler_.integer_value(value,context);
    }

    void do_uinteger_value(uint64_t value,
                           const serializing_context& context) override
    {
        downstream_handler_.uinteger_value(value,context);
    }

    void do_bool_value(bool value,
                       const serializing_context& context) override
    {
        downstream_handler_.bool_value(value,context);
    }

    void do_null_value(const serializing_context& context) override
    {
        downstream_handler_.null_value(context);
    }

};

// Filters out begin_json and end_json events
template <class CharT>
class basic_json_fragment_filter : public basic_json_filter<CharT>
{
public:
    using typename basic_json_filter<CharT>::string_view_type;

    basic_json_fragment_filter(basic_json_content_handler<CharT>& handler)
        : basic_json_filter<CharT>(handler)
    {
    }
private:
    void do_begin_json() override
    {
    }

    void do_end_json() override
    {
    }
};

template <class CharT>
class basic_rename_object_member_filter : public basic_json_filter<CharT>
{
public:
    using typename basic_json_filter<CharT>::string_view_type;

private:
    std::basic_string<CharT> name_;
    std::basic_string<CharT> new_name_;
public:
    basic_rename_object_member_filter(const std::basic_string<CharT>& name,
                             const std::basic_string<CharT>& new_name,
                             basic_json_content_handler<CharT>& handler)
        : basic_json_filter<CharT>(handler), 
          name_(name), new_name_(new_name)
    {
    }

private:
    void do_name(const string_view_type& name,
                 const serializing_context& context) override
    {
        if (name == name_)
        {
            this->downstream_handler().name(new_name_,context);
        }
        else
        {
            this->downstream_handler().name(name,context);
        }
    }
};

typedef basic_json_filter<char> json_filter;
typedef basic_json_filter<wchar_t> wjson_filter;
typedef basic_rename_object_member_filter<char> rename_object_member_filter;
typedef basic_rename_object_member_filter<wchar_t> wrename_object_member_filter;

#if !defined(JSONCONS_NO_DEPRECATED)
typedef basic_rename_object_member_filter<char> rename_name_filter;
typedef basic_rename_object_member_filter<wchar_t> wrename_name_filter;
#endif

}

#endif
