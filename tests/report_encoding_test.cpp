#include <cassert>
#include <cstdio>
#include <string>
#include <vector>

#include "../src/report/report_encoding.h"

int main()
{
    assert(html_escape("") == "");
    assert(html_escape("a<b>c&d\"e'f") == "a&lt;b&gt;c&amp;d&quot;e&#39;f");
    assert(html_escape("plain text") == "plain text");
    assert(html_escape("void main(){}") == "void main(){}");

    assert(base64_encode({}) == "");
    assert(base64_encode({static_cast<unsigned char>('M')}) == "TQ==");
    assert(base64_encode({static_cast<unsigned char>('M'), static_cast<unsigned char>('a')}) == "TWE=");
    assert(base64_encode({static_cast<unsigned char>('M'), static_cast<unsigned char>('a'), static_cast<unsigned char>('n')}) == "TWFu");

    {
        std::string s = "Many hands make light work.";
        std::vector<unsigned char> bytes(s.begin(), s.end());
        assert(base64_encode(bytes) == "TWFueSBoYW5kcyBtYWtlIGxpZ2h0IHdvcmsu");
    }

    {
        std::vector<unsigned char> bytes{0x00, 0x01, 0x02, 0x03, 0xFF};
        std::string encoded = base64_encode(bytes);
        assert(!encoded.empty());
        assert(encoded.size() % 4 == 0);
    }

    std::printf("report_encoding_test: all tests passed\n");
    return 0;
}
