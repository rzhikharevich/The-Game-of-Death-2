#include "util.hpp"
#include <cstdlib>
#include <fcntl.h>

#ifdef __linux__
#include <linux/random.h>
#endif

using std::uint32_t;


uint32_t GetRandom(uint32_t lt) {
    uint32_t rnd;
    
#ifdef UNIX
    /*
     * UNIX.
     */
    
#ifdef BSD
    /*
     * UNIX: BSD.
     */
    
    rnd = lt > 0 ? arc4random_uniform(lt) : arc4random();
#elif defined(__linux__)
    /*
     * UNIX: Linux.
     */
    
    rnd = getrandom((void *)&rnd, sizeof(uint32_t), 0) % lt;
#else
    /*
     * UNIX: Other.
     */
    
    FileOpenIn("/dev/urandom", true).read((char *)&rnd, sizeof(uint32_t));
    rnd %= lt;
#endif
    
#else
    /*
     * Plain C.
     */
    
    rnd = rand() | rand() << 16;
#endif
    
    return rnd;
}

uint32_t GetRandom(std::uint32_t from, std::uint32_t to) {
    return from + GetRandom(to + 1);
}

void ChangeDir(const char *path) {
    if (sys::chdir(path))
        throw FileError(path);
}

std::ifstream FileOpenIn(const char *path, bool binary) {
    std::ifstream fs(path, binary? std::ios::binary : 0);
    if (!fs)
        throw FileError(path);
    
    return fs;
}

FileError::FileError(const char *path) {
    reason = std::string("Failed to access: '") + path + "'.";
}
