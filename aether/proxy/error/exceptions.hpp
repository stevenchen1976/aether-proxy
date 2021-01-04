/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <string>
#include <stdexcept>
#include <cstdint>

#define EXCEPTION_CATEGORIES(X, other) \
X(0, proxy, PROXY, "Proxy error", false, other) \
X(1, http, HTTP, "HTTP error", true, other) \
X(2, tls, TLS, "TLS error", true, other) \
X(3, websocket, WEBSOCKET, "WebSocket error", true, other)

#define PROXY_EXCEPTIONS(X, other) \
X(1, invalid_option, "Invalid option", other) \
X(2, ipv6_error, "IPv6 error", other) \
X(3, invalid_operation, "Invalid operation", other) \
X(4, acceptor_error, "Acceptor error", other) \
X(5, parser_error, "Parser error", other) \
X(6, server_not_connected, "Server is not connected", other) \
X(7, asio_error, "ASIO error", other) \
X(8, self_connect, "Proxy cannot connect to itself", other)

#define HTTP_EXCEPTIONS(X, other) \
X(1, invalid_method, "Invalid HTTP method", other) \
X(2, invalid_status, "Invalid HTTP status", other) \
X(3, invalid_version, "Invalid HTTP version", other) \
X(4, invalid_target_host, "Invalid target host", other) \
X(5, invalid_target_port, "Invalid target port", other) \
X(6, invalid_request_line, "Invalid HTTP request line", other) \
X(7, invalid_header, "Invalid HTTP header", other) \
X(8, header_not_found, "Header was not found", other) \
X(9, invalid_body_size, "Invalid HTTP body size", other) \
X(10, body_size_too_large, "Given body size exceeds limit", other) \
X(11, invalid_chunked_body, "Malformed chunked-encoding body", other) \
X(12, no_response, "HTTP exchange has no response", other) \
X(13, invalid_response_line, "Invalid HTTP response line", other) \
X(14, malformed_response_body, "Malformed response body", other)

#define TLS_EXCEPTIONS(X, other) \
X(1, invalid_client_hello, "Invalid Client Hello message", other) \
X(2, read_access_violation, "Read access violation (not enough data)", other) \
X(3, tls_service_error, "Exception in TLS service", other) \
X(4, invalid_ssl_method, "Invalid SSL version", other) \
X(5, invalid_cipher_suite, "Invalid cipher suite", other) \
X(6, invalid_trusted_certificates_file, "Invalid verify file", other) \
X(7, invalid_cipher_suite_list, "Invalid cipher suite list", other) \
X(8, invalid_alpn_protos_list, "Invalid ALPN protocol list", other) \
X(9, ssl_context_error, "Failed to create and configure SSL context", other) \
X(10, ssl_server_store_creation_error, "An error occurred when creating the SSL server's certificate store", other) \
X(11, certificate_creation_error, "An error occurred when generating an SSL certificate", other) \
X(12, certificate_issuer_not_found,"Certificate issuer could not be found", other) \
X(13, certificate_subject_not_found, "Certificate subject could not be found", other) \
X(14, certificate_name_entry_error, "Error accessing certificate name entry", other) \
X(15, alpn_not_found, "Negotiated ALPN could not be found", other) \
X(16, upstream_handshake_failed, "Upstream handshake failed", other) \
X(17, downstream_handshake_failed, "Downstream handshake failed", other) \
X(18, upstream_connect_error, "Could not connect to upstream server", other)

#define WEBSOCKET_EXCEPTIONS(X, other) \
X(1, invalid_opcode, "Invalid WebSocket opcode", other) \
X(2, extension_param_not_found, "Extension parameter was not found", other) \
X(3, invalid_extension_string, "Invalid extension string", other) \
X(4, invalid_frame, "Invalid frame", other) \
X(5, unexpected_opcode, "Unexpected opcode", other) \
X(6, serialization_error, "Frame serialization error", other) \
X(7, zlib_error, "zlib error", other)

namespace proxy {
    namespace errc {

// Error codes are organized by category, which is represented by a chunk of potentially active bits
// Each category can have 1 << CATEGORY_BITSIZE different error codes in it
#define CATEGORY_BITSIZE 5

// Assure that each category is big enough to hold the number of errors within it

#define ASSERT_VALID_CATEGORY_SIZE(num, name, macro_name, msg, nested, foreach) macro_name##_EXCEPTIONS(foreach, num)
#define ASSERT_VALID_ERROR_CODE(num, name, msg, offset) static_assert(num < ((1 << CATEGORY_BITSIZE) - 1), "Exception category size too small");

        EXCEPTION_CATEGORIES(ASSERT_VALID_CATEGORY_SIZE, ASSERT_VALID_ERROR_CODE)

#undef ASSERT_VALID_ERROR_CODE
#undef ASSERT_VALID_CATEGORY_SIZE

        /*
            Internal enumeration for all error codes.
        */
        enum errc_t : std::int32_t {
            success = 0,

// Generate error codes using categories and bit shifting

#define CREATE_CATEGORY_CODES(num, name, macro_name, msg, nested, foreach) \
name ## _error = ((1 << CATEGORY_BITSIZE) - 1) << (num * CATEGORY_BITSIZE), \
macro_name ## _EXCEPTIONS(foreach, num)

#define CREATE_ERROR_CODES(num, name, msg, offset) name = num << (offset * CATEGORY_BITSIZE),

            EXCEPTION_CATEGORIES(CREATE_CATEGORY_CODES, CREATE_ERROR_CODES)

#undef CREATE_ERROR_CODES
#undef CREATE_CATEGORY_CODES

#undef CATEGORY_BITSIZE
        };

// Functions for checking the category of a given error code

#define CATEGORY_CHECK_FUNCTION(num, name, macro_name, msg, nested, _) \
constexpr bool is_##name##_error(errc_t code) { \
        return code & name##_error; \
}

        EXCEPTION_CATEGORIES(CATEGORY_CHECK_FUNCTION,)

#undef CATEGORY_CHECK_FUNCTION
    }

        /*
            A parallel to boost::system::error_code for holding proxy-related exceptions.
        */
        class error_code {
        private:
            errc::errc_t val;

        public:
            constexpr error_code() noexcept
                : val(errc::success) { }

            constexpr error_code(errc::errc_t val) noexcept
                : val(val) { }

            constexpr errc::errc_t value() const noexcept {
                return val;
            }

            constexpr explicit operator bool() const noexcept {
                return val != errc::errc_t::success;
            }

            constexpr void clear() noexcept {
                val = errc::success;
            }

            constexpr friend bool operator==(const error_code &lhs, const error_code &rhs) noexcept {
                return lhs.val == rhs.val;
            }

            constexpr friend bool operator!=(const error_code &lhs, const error_code &rhs) noexcept {
                return !(lhs == rhs);
            }

            constexpr std::string_view message() const noexcept {
                switch (val) {
                    case errc::success: return "Success";
#define CATEGORY_SWITCH_CASE(num, name, macro_name, msg, nested, foreach) macro_name##_EXCEPTIONS(foreach, num)
#define SWITCH_CASE(num, name, msg, offset) case errc::##name: return msg;
                        
                        EXCEPTION_CATEGORIES(CATEGORY_SWITCH_CASE, SWITCH_CASE)

#undef SWITCH_CASE
#undef CATEGORY_SWITCH_CASE
                    default: return "Unknown error";
                }
            }

#define CATEGORY_CHECK_METHOD(num, name, macro_name, msg, nested, _) \
constexpr bool is_##name() { \
        return val & errc::##name##_error; \
}

            EXCEPTION_CATEGORIES(CATEGORY_CHECK_METHOD, )

#undef CATEGORY_CHECK_METHOD

            friend inline std::ostream &operator<<(std::ostream &output, const error_code &error) {
                output << error.message();
                return output;
            }
        };

        namespace error {
            /*
                Base exception class for any error that comes out of the proxy.
            */
            class base_exception
                : public std::runtime_error {
            private:
                errc::errc_t code;

            public:
                explicit base_exception(const std::string &msg, errc::errc_t code) : std::runtime_error(msg), code(code) { }

                constexpr proxy::error_code error_code() const noexcept {
                    return code;
                }
            };

// Generate an exception class for each error category and error code to enable try-catch blocks
// The proxy category is kept in the proxy::error:: namespace. All other categories get their own namespace.

#define GENERATE_ALL_EXCEPTIONS_false(name, macro_name, msg, foreach) \
macro_name##_EXCEPTIONS(foreach, base_exception)

#define GENERATE_ALL_EXCEPTIONS_true(name, macro_name, msg, foreach) \
class name##_exception : public base_exception { \
public: \
    explicit name##_exception(const std::string &str = msg, errc::errc_t code = errc::name##_error) \
        : base_exception(str, code) \
    { } \
}; \
namespace name { \
    macro_name##_EXCEPTIONS(foreach, name##_exception) \
}

#define GENERATE_ALL_EXCEPTIONS(num, name, macro_name, msg, nested, foreach) \
GENERATE_ALL_EXCEPTIONS_##nested(name, macro_name, msg, foreach)

#define GENERATE_EXCEPTION(num, name, msg, base) \
class name##_exception : public base { \
public: \
    explicit name##_exception(const std::string &str = msg) \
        : base(#name ": " + str, errc::errc_t::name) \
    { } \
};

            EXCEPTION_CATEGORIES(GENERATE_ALL_EXCEPTIONS, GENERATE_EXCEPTION)

#undef GENERATE_EXCEPTION
#undef GENERATE_ALL_EXCEPTIONS
#undef GENERATE_ALL_EXCEPTIONS_true
#undef GENERATE_ALL_EXCEPTIONS_false
    }
}