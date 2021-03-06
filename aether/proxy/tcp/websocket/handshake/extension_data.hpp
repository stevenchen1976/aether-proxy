/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <map>
#include <vector>
#include <string>
#include <iostream>

#include <aether/proxy/error/exceptions.hpp>
#include <aether/util/string.hpp>

namespace proxy::tcp::websocket::handshake {
    /*
        Represents the data for a single WebSocket extension.
    */
    class extension_data {
    public:
        static constexpr char extension_delim = ',';
        static constexpr char param_delim = ';';
        static constexpr char assign_delim = '=';

    private:
        std::string name;
        std::map<std::string, std::string> params;

    public:
        extension_data(const std::string &name);

        std::string get_name() const;
        void set_name(const std::string &name);
        bool has_param(const std::string &name) const;
        std::string get_param(const std::string &name) const;
        void set_param(const std::string &name, const std::string &value = "");

        /*
            Parses a single extension string and its parameters.
            The input string must not have an extension::extension_delim character
                anywhere in it, as this means there are two extensions within the string.
        */
        static extension_data from_header_value(const std::string &header);

        friend std::ostream &operator<<(std::ostream &out, const extension_data &ext);
        friend std::ostream &operator<<(std::ostream &out, const std::vector<extension_data> &ext_list);
    };
}