#ifndef UTIL_HPP
#define UTIL_HPP


#include <string>
#include <fstream>
#include <memory>
#include <exception>
#include <functional>
#include <cstdint>
#include <string.h>


namespace sys {
#if defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__))
#define UNIX
#include <unistd.h>
#include <sys/param.h>
#elif defined(_WIN32)
#define WINDOWS
#include <windows.h>
#include <direct.h>
#else
#error "Operating system unsupported."
#endif
}


uint32_t GetRandom(std::uint32_t lt = 0);
uint32_t GetRandom(std::uint32_t from, std::uint32_t to);

static inline void FreeString(char *string) {std::free((void *)string);}

template <class T>
class ImplicitPtr : public std::shared_ptr<T> {
public:
    using std::shared_ptr<T>::shared_ptr;
    
    using std::shared_ptr<T>::operator =;
    using std::shared_ptr<T>::operator ->;
    using std::shared_ptr<T>::operator bool;
    
    operator T *() const {
        return std::shared_ptr<T>::get();
    }
    
    ImplicitPtr<T> operator =(T *ptr) {
        std::shared_ptr<T>::reset(ptr);
        return *this;
    }
};

void ChangeDir(const char *path);

std::ifstream FileOpenIn(const char *path, bool binary = false);
static inline std::ifstream FileOpenIn(const std::string &path, bool binary = false) {
    return FileOpenIn(path.c_str());
}

class FileError : public std::exception {
private:
    std::string reason;
    
public:
    FileError(const char *path);
    
    virtual const char *what() const noexcept {return reason.c_str();}
};

template <class Value>
class Lazy {
private:
    typedef std::function<Value()> Getter;
    
    Getter get;
    Value value;

    bool ready = false;
    
public:
    Lazy(const Getter &get) : get(get) {};
    
    operator Value() {
        if (ready)
            return value;
        
        return value = get();
    }
    
    Value *operator->() {
        return &value;
    }
    
    void reset() {
        ready = false;
    }
};


#endif
