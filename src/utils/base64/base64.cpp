#include <string>

#include "utils/string.h"

static const std::string base64_chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

std::string base64Encode(const std::string &string_to_encode)
{
    char const* bytes_to_encode = string_to_encode.data();
    unsigned int in_len = string_to_encode.size();

    std::string ret;
    int i = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];

    while (in_len--)
    {
        char_array_3[i++] = *(bytes_to_encode++);
        if (i == 3)
        {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for(i = 0; (i <4) ; i++)
                ret += base64_chars[char_array_4[i]];
            i = 0;
        }
    }

    if (i)
    {
        int j;
        for(j = i; j < 3; j++)
            char_array_3[j] = '\0';

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;

        for (j = 0; (j < i + 1); j++)
            ret += base64_chars[char_array_4[j]];

        while((i++ < 3))
            ret += '=';

    }

    return ret;

}

std::string base64Decode(const std::string &encoded_string, bool accept_urlsafe)
{
    string_size in_len = encoded_string.size();
    string_size i = 0;
    string_size in_ = 0;
    unsigned char char_array_4[4], char_array_3[3], uchar;
    static unsigned char dtable[256], itable[256], table_ready = 0;
    std::string ret;

    // Should not need thread_local with the flag...
    if (!table_ready)
    {
        // No memset needed for static/TLS
        for (string_size k = 0; k < base64_chars.length(); k++)
        {
            uchar = base64_chars[k]; // make compiler happy
            dtable[uchar] = k;  // decode (find)
            itable[uchar] = 1;  // is_base64
        }
        const unsigned char dash = '-', add = '+', under = '_', slash = '/';
        // Add urlsafe table
        dtable[dash] = dtable[add]; itable[dash] = 2;
        dtable[under] = dtable[slash]; itable[under] = 2;
        table_ready = 1;
    }

    while (in_len-- && (encoded_string[in_] != '='))
    {
        uchar = encoded_string[in_]; // make compiler happy
        if (!(accept_urlsafe ? itable[uchar] : (itable[uchar] == 1))) // break away from the while condition
        {
            ret += uchar; // not base64 encoded data, copy to result
            in_++;
            i = 0;
            continue;
        }
        char_array_4[i++] = uchar;
        in_++;
        if (i == 4)
        {
            for (string_size j = 0; j < 4; j++)
                char_array_4[j] = dtable[char_array_4[j]];

            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

            for (i = 0; (i < 3); i++)
                ret += char_array_3[i];
            i = 0;
        }
    }

    if (i)
    {
        for (string_size j = i; j <4; j++)
            char_array_4[j] = 0;

        for (string_size j = 0; j <4; j++)
            char_array_4[j] = dtable[char_array_4[j]];

        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
        char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

        for (string_size j = 0; (j < i - 1); j++)
            ret += char_array_3[j];
    }

    return ret;
}

std::string urlSafeBase64Reverse(const std::string &encoded_string)
{
    return replaceAllDistinct(replaceAllDistinct(encoded_string, "-", "+"), "_", "/");
}

std::string urlSafeBase64Apply(const std::string &encoded_string)
{
    return replaceAllDistinct(replaceAllDistinct(replaceAllDistinct(encoded_string, "+", "-"), "/", "_"), "=", "");
}

std::string urlSafeBase64Decode(const std::string &encoded_string)
{
    return base64Decode(encoded_string, true);
}

std::string urlSafeBase64Encode(const std::string &string_to_encode)
{
    return urlSafeBase64Apply(base64Encode(string_to_encode));
}
