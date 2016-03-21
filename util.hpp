#ifndef UTIL_HPP
#define UTIL_HPP


#include <string>
#include <fstream>
#include <memory>
#include <exception>
#include <cstdint>
#include <string.h>


uint32_t GetRandom(std::uint32_t lt = 0);

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


#endif
