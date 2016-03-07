#ifndef UTIL_HPP
#define UTIL_HPP


#include <memory>


char *DuplicateString(const char *string);
void FreeString(char *string);

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


#endif
