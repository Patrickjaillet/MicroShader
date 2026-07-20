#include "report_encoding.h"

std::string html_escape(const std::string& text)
{
    std::string out;
    out.reserve(text.size());
    for (char c : text)
    {
        switch (c)
        {
            case '&':
                out += "&amp;";
                break;
            case '<':
                out += "&lt;";
                break;
            case '>':
                out += "&gt;";
                break;
            case '"':
                out += "&quot;";
                break;
            case '\'':
                out += "&#39;";
                break;
            default:
                out += c;
                break;
        }
    }
    return out;
}

std::string base64_encode(const std::vector<unsigned char>& bytes)
{
    static const char kAlphabet[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";

    std::string out;
    out.reserve(((bytes.size() + 2) / 3) * 4);

    std::size_t i = 0;
    while (i + 2 < bytes.size())
    {
        unsigned int chunk = (static_cast<unsigned int>(bytes[i]) << 16)
            | (static_cast<unsigned int>(bytes[i + 1]) << 8)
            | static_cast<unsigned int>(bytes[i + 2]);
        out += kAlphabet[(chunk >> 18) & 0x3F];
        out += kAlphabet[(chunk >> 12) & 0x3F];
        out += kAlphabet[(chunk >> 6) & 0x3F];
        out += kAlphabet[chunk & 0x3F];
        i += 3;
    }

    std::size_t remaining = bytes.size() - i;
    if (remaining == 1)
    {
        unsigned int chunk = static_cast<unsigned int>(bytes[i]) << 16;
        out += kAlphabet[(chunk >> 18) & 0x3F];
        out += kAlphabet[(chunk >> 12) & 0x3F];
        out += '=';
        out += '=';
    }
    else if (remaining == 2)
    {
        unsigned int chunk = (static_cast<unsigned int>(bytes[i]) << 16)
            | (static_cast<unsigned int>(bytes[i + 1]) << 8);
        out += kAlphabet[(chunk >> 18) & 0x3F];
        out += kAlphabet[(chunk >> 12) & 0x3F];
        out += kAlphabet[(chunk >> 6) & 0x3F];
        out += '=';
    }

    return out;
}
