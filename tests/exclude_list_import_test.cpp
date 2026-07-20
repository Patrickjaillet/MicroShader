#include <cassert>
#include <cstdio>
#include <string>

#include "../src/ui/exclude_list_import.h"

int main()
{
    assert(parse_exclude_name_list("") == "");
    assert(parse_exclude_name_list("myUniform\nresolution\n") == "myUniform, resolution");
    assert(parse_exclude_name_list("myUniform\r\nresolution\r\n") == "myUniform, resolution");
    assert(parse_exclude_name_list("\n\nmyUniform\n\n\nresolution\n\n") == "myUniform, resolution");
    assert(parse_exclude_name_list("  myUniform  \n  resolution  \n") == "myUniform, resolution");
    assert(parse_exclude_name_list("// comment\nmyUniform\n# also a comment\nresolution\n") == "myUniform, resolution");

    assert(merge_protected_names("", "myUniform, resolution") == "myUniform, resolution");
    assert(merge_protected_names("myUniform", "myUniform, resolution") == "myUniform, resolution");
    assert(merge_protected_names("existing", "myUniform, resolution") == "existing, myUniform, resolution");
    assert(merge_protected_names("a, b", "") == "a, b");
    assert(merge_protected_names("a,b", "b,c") == "a, b, c");

    std::printf("exclude_list_import_test: all tests passed\n");
    return 0;
}
