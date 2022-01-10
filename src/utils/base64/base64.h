#ifndef BASE64_H_INCLUDED
#define BASE64_H_INCLUDED

#include <string>

std::string base64Decode(const std::string &encoded_string, bool accept_urlsafe = false);
std::string base64Encode(const std::string &string_to_encode);

std::string urlSafeBase64Apply(const std::string &encoded_string);
std::string urlSafeBase64Reverse(const std::string &encoded_string);
std::string urlSafeBase64Decode(const std::string &encoded_string);
std::string urlSafeBase64Encode(const std::string &string_to_encode);

#endif // BASE64_H_INCLUDED
