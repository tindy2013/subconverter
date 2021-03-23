#ifndef CODEPAGE_H_INCLUDED
#define CODEPAGE_H_INCLUDED

#include <string>

std::string utf8ToACP(const std::string &str_src);
std::string acpToUTF8(const std::string &str_src);

#endif // CODEPAGE_H_INCLUDED
