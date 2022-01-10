#ifndef FILE_EXTRA_H_INCLUDED
#define FILE_EXTRA_H_INCLUDED

#include "base64/base64.h"
#include "file.h"
#include "md5/md5_interface.h"

inline std::string fileToBase64(const std::string &filepath)
{
    return base64Encode(fileGet(filepath));
}

inline std::string fileGetMD5(const std::string &filepath)
{
    return getMD5(fileGet(filepath));
}

#endif // FILE_EXTRA_H_INCLUDED
