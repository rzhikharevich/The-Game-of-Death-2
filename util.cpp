#include "util.hpp"
#include <cstdlib>
#include <cstring>


char *DuplicateString(const char *string) {
    size_t size = strlen(string) + 1;
    char *dup = (char *)malloc(size);
    return (char *)memcpy(dup, string, size);
}

void FreeString(char *string) {
    free(string);
}
