#include "util.hpp"
#include <cstdlib>
#include <cstring>


char *DuplicateString(const char *string) {
    std::size_t size = std::strlen(string) + 1;
    char *dup = (char *)std::malloc(size);
    return (char *)std::memcpy(dup, string, size);
}

void FreeString(char *string) {
    std::free(string);
}
