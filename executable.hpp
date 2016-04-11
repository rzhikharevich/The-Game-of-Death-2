#ifndef EXECUTABLE_HPP
#define EXECUTABLE_HPP


#include <vector>
#include <string>
#include <fstream>
#include <cstdint>
#include <exception>


/*
 * Executable.
 */

class Executable {
public:
    /*
     * All opcodes and operands are 32-bit.
     * Random values are specified by 0 if applicable.
     */
    typedef std::uint32_t Word;
    typedef std::vector<Word> Bytecode;
    
    enum {
        InsnEat,
        InsnGo,
        InsnClon,
        InsnStr,
        InsnLeft,
        InsnRight,
        InsnBack,
        InsnTurn,
        InsnJG,
        InsnJL,
        InsnJ,
        InsnJE,
        InsnMax = InsnJE
    };

    Executable() {}
    Executable(const std::string &path);
    
    const Bytecode &getBytecode() const {return bytecode;}
    std::size_t size() const {return bytecode.size();}
    Word operator[](std::size_t i) const {return bytecode[i];}
    
private:
    Bytecode bytecode;
};

/*
 * ExecutableError.
 */

class ExecutableError : public std::exception {
private:
    std::string reason;
    
public:
    ExecutableError(const std::string &path, int line);
    
    virtual const char *what() const noexcept {return reason.c_str();}
};


#endif
