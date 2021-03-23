#ifndef MD5_INTERFACE_H_INCLUDED
#define MD5_INTERFACE_H_INCLUDED

#include <string>

#include "md5.h"

inline std::string getMD5(const std::string &data)
{
    std::string result;

    /*
    unsigned int i = 0;
    unsigned char digest[16] = {};

#ifdef USE_MBEDTLS
    mbedtls_md5_context ctx;

    mbedtls_md5_init(&ctx);
    mbedtls_md5_starts_ret(&ctx);
    mbedtls_md5_update_ret(&ctx, reinterpret_cast<const unsigned char*>(data.data()), data.size());
    mbedtls_md5_finish_ret(&ctx, reinterpret_cast<unsigned char*>(&digest));
    mbedtls_md5_free(&ctx);
#else
    MD5_CTX ctx;

    MD5_Init(&ctx);
    MD5_Update(&ctx, data.data(), data.size());
    MD5_Final((unsigned char *)&digest, &ctx);
#endif // USE_MBEDTLS

    char tmp[3] = {};
    for(i = 0; i < 16; i++)
    {
        snprintf(tmp, 3, "%02x", digest[i]);
        result += tmp;
    }
    */

    char result_str[MD5_STRING_SIZE];
    md5::md5_t md5;
    md5.process(data.data(), data.size());
    md5.finish();
    md5.get_string(result_str);
    result.assign(result_str);

    return result;
}

#endif // MD5_INTERFACE_H_INCLUDED
