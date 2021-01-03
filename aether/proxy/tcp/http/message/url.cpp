/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "url.hpp"

namespace proxy::tcp::http {
    url url::make_authority_form(std::string_view host, port_t port) {
        return { target_form::authority, { }, { { }, { }, util::string::as_string(host), port } };
    }

    url url::make_origin_form(std::string_view path, std::string_view search) {
        return { target_form::origin, { }, { }, util::string::as_string(path), util::string::as_string(search) };
    }

    url url::parse_authority_form(std::string_view str) {
        // Host and port are both required
        std::size_t port_pos = str.find(':');
        if (port_pos == std::string::npos) {
            throw error::http::invalid_target_port_exception { "Missing port for authority form" };
        }
        port_t port = parse_port(str.substr(port_pos + 1));
        return make_authority_form(str.substr(0, port_pos), port);
    }

    url url::parse_origin_form(std::string_view str) {
        std::size_t earliest_delim = str.find_first_of(search_delims);
        // Split path and search
        if (earliest_delim != std::string::npos) {
            return make_origin_form(str.substr(0, earliest_delim), str.substr(earliest_delim));
        }
        // Whole string is the path
        else {
            return make_origin_form(str);
        }
    }

    // RFC 1808
    // <scheme>://<netloc>/<path>;<params>?<query>#<fragment>
    url url::parse_absolute_form(std::string_view str) {
        url result = { target_form::absolute };

        // Find split between scheme and netloc
        std::size_t netloc_start = str.find(':');

        if (netloc_start == std::string::npos || (str[0] == '/' && str[1] == '/')) {
            netloc_start = 0;
        }
        else {
            result.scheme = util::string::as_string(util::string::substring(str, 0, netloc_start));
        }
        
        // Has netloc
        // We found a scheme and the "//" segment
        // Or we didn't find a scheme, but the next part doesn't look like a path
        if (str[netloc_start + 1] == '/' && str[netloc_start + 2] == '/') {
            netloc_start += 3;

            // Find start of path and search (params/query/fragment)
            std::size_t earliest_nonslash_delim = str.find_first_of(search_delims, netloc_start);
            std::size_t first_slash = str.find('/', netloc_start);
            std::size_t earliest_delim = std::min(earliest_nonslash_delim, first_slash);

            /*
                           1       2      3
                <scheme>://<netloc>/<path>;<params>?<query>#<fragment>
                    
                1. netloc_start
                2. first_slash (may be std::string::npos)
                3. earliest_nonslash_delim (may be std::string::npos)
            */

            // Parse netloc
            result.netloc = parse_netloc(util::string::substring(str, netloc_start, earliest_delim));

            // Path exists
            if (first_slash != std::string::npos) {
                // Path and search exists
                if (earliest_nonslash_delim != std::string::npos && earliest_nonslash_delim > first_slash) {
                    result.path = util::string::substring(str, first_slash, earliest_nonslash_delim);
                    result.search = util::string::substring(str, earliest_nonslash_delim);
                }
                // Just path exists
                else if (first_slash != std::string::npos) {
                    result.path = util::string::substring(str, first_slash);
                }
            }
            // Search exists, but path does not exist
            else if (earliest_nonslash_delim != std::string::npos) {
                result.search = util::string::substring(str, earliest_nonslash_delim);
            }
            // else => Nothing after netloc
        }
        // No netloc, but there is a path
        else if (str[netloc_start + 1] == '/') {
            netloc_start += 2;
            std::size_t earliest_nonslash_delim = str.find_first_of(search_delims, netloc_start);

            // Search exists
            if (earliest_nonslash_delim != std::string::npos) {
                result.path = util::string::substring(str, netloc_start, earliest_nonslash_delim);
                result.search = util::string::substring(str, earliest_nonslash_delim);
            }
            // No search
            else {
                result.path = util::string::substring(str, netloc_start);
            }
        }
        // No netloc, no path, just search
        else {
            result.search = util::string::substring(str, netloc_start + 1);
        }

        return result;
    }

    // Netloc ==> RFC 1738
    // //<user>:<password>@<host>:<port>/<url-path>
    // We are parsing without /<url-path>
    url::network_location url::parse_netloc(std::string_view str) {
        url::network_location result;

        // Start after the two slashes
        std::size_t start = 0;
        if (str[0] == '/' && str[1] == '/') {
            start = 2;
        }

        // Get username and password if applicable
        std::size_t user_end = str.find('@', start);
        if (user_end != std::string::npos) {
            // Password is optional
            std::size_t password_start = str.find(':', start);
            if (password_start < user_end) {
                result.password = util::string::substring(str, password_start + 1, user_end);
            }

            result.username = util::string::substring(str, start, user_end);
            start = user_end;
        }

        // Port is optional
        std::size_t port_start = str.find(':', start);
        if (port_start != std::string::npos) {
            result.port = parse_port(util::string::substring(str, port_start + 1));
            result.host = util::string::substring(str, start, port_start);
        }
        else {
            result.host = util::string::substring(str, start);
        }

        return result;
    }

    // Parse port from string, validating its numerical value in the process
    port_t url::parse_port(std::string_view str) {
        std::size_t port_long;
        try {
            port_long = boost::lexical_cast<std::size_t>(str);
            if (port_long > std::numeric_limits<port_t>::max()) {
                throw error::http::invalid_target_port_exception { "Target port out of range" };
            }
            return static_cast<port_t>(port_long);
        }
        catch (const boost::bad_lexical_cast &) {
            throw error::http::invalid_target_port_exception { "Target port invalid" };
        }
    }

    url url::parse(std::string_view str) {
        if (str == "*") {
            return { target_form::asterisk };
        }
        else if (str[0] == '/') {
            return parse_origin_form(str);
        }
        // Authority form URLs are illegal outside of a CONNECT context
        return parse_absolute_form(str);
    }

    // RFC-7230 Section 5.3
    url url::parse_target(std::string_view str, method verb) {
        if (str == "*") {
            return { target_form::asterisk };
        }
        else if (str[0] == '/') {
            return parse_origin_form(str);
        }
        else if (verb == method::CONNECT) {
            return parse_authority_form(str);
        }
        else {
            return parse_absolute_form(str);
        }
    }

    bool url::network_location::empty() const {
        return username.empty() && password.empty() && host.empty();
    }

    bool url::network_location::has_hostname() const {
        return !host.empty();
    }

    bool url::network_location::has_port() const {
        return port.has_value();
    }

    std::string url::network_location::to_string() const {
        std::stringstream str;
        str << *this;
        return str.str();
    }

    std::string url::network_location::to_host_string() const {
        std::stringstream str;
        str << host;
        if (port.has_value()) {
            str << ':' << port.value();
        }
        return str.str();
    }

    std::string url::to_string() const {
        std::stringstream str;
        str << *this;
        return str.str();
    }

    std::string url::absolute_string() const {
        std::stringstream str;
        if (!scheme.empty()) {
            str << scheme << ':';
            if (!netloc.empty()) {
                str << "//";
            }
        }
        str << netloc;
        if (!path.empty()) {
            str << path;
        }
        str << search;
        return str.str();
    }

    std::string url::origin_string() const {
        std::stringstream str;
        if (!scheme.empty()) {
            str << scheme << ':';
            if (!netloc.empty()) {
                str << "//";
            }
        }
        str << netloc;
        return str.str();
    }

    std::string url::full_path() const {
        return path + search;
    }

    bool url::is_host(std::string_view host) const {
        return netloc.host == host;
    }

    bool url::is_host(std::string_view host, port_t port) const {
        return netloc.host == host && netloc.has_port() && netloc.port.value() == port;
    }

    std::ostream &operator<<(std::ostream &output, const url &u) {
        if (u.form == url::target_form::asterisk) {
            output << '*';
        }
        else {
            if (u.form != url::target_form::origin) {
                if (!u.scheme.empty()) {
                    output << u.scheme << ':';
                    if (!u.netloc.empty()) {
                        output << "//";
                    }
                }
                output << u.netloc;
            }
            if (!u.path.empty()) {
                output << u.path;
            }
            output << u.search;
        }
        return output;
    }

    std::ostream &operator<<(std::ostream &output, const url::network_location &netloc) {
        if (!netloc.empty()) {
            if (!netloc.username.empty()) {
                output << netloc.username;
                if (!netloc.password.empty()) {
                    output << ':' << netloc.password;
                }
                output << '@';
            }
            output << netloc.to_host_string();
        }
        return output;
    }
}