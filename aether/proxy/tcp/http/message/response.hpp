/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <aether/proxy/tcp/http/message/message.hpp>
#include <aether/proxy/tcp/http/message/status.hpp>
#include <aether/proxy/tcp/http/state/cookie.hpp>

namespace proxy::tcp::http {
    /*
        A single HTTP response.
    */
    class response 
        : public message {
    private:
        status status_code;

    public:
        response();
        response(version _version, status status_code, std::initializer_list<header_pair> headers, const std::string &content);
        response(const response &other);
        response &operator=(const response &other);
        response(response &&other) noexcept;
        response &operator=(response &&other) noexcept;

        status get_status() const;
        void set_status(status status_code);

        bool is_1xx() const;
        bool is_2xx() const;
        bool is_3xx() const;
        bool is_4xx() const;
        bool is_5xx() const;

        std::vector<cookie> set_cookie_headers() const;

        friend std::ostream &operator<<(std::ostream &out, const response &res);
    };
}