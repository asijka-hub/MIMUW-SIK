//
// Created by Andrzej on 29.05.2023.
//

#ifndef SIKRADIO_UTILS_H
#define SIKRADIO_UTILS_H

#include <charconv>

template <class T>
bool parse_to_uint_T(const std::string& str, T& value) {
    auto result = std::from_chars(str.data(), str.data() + str.size(), value);
    return result.ec == std::errc{} && result.ptr == str.data() + str.size();
}


#endif //SIKRADIO_UTILS_H
