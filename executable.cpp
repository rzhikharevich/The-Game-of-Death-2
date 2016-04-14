#include "executable.hpp"
#include <unordered_map>
#include <functional>
#include "util.hpp"


Executable::Executable(const std::string &path) {
    auto in = FileOpenIn(path);
    
    std::vector<Word> jmp;
    
    std::string line;
    int n = 1;
    while (std::getline(in, line)) {
        jmp.push_back((Word)bytecode.size());
        
        std::vector<std::string> insn;
        
        std::size_t i = 0;
        std::size_t d;
        do {
            std::string s;
            d = line.find(' ', i);
            if (d == s.npos)
                s = std::string(line, i);
            else
                s = std::string(line, i, d - i);
            
            if (s.length())
                insn.push_back(s);
            
            i = d + 1;
        } while (d != std::string::npos);
        
        auto parseOpN = [this, insn, path, n] {
            switch (insn.size()) {
                case 1:
                    bytecode.push_back(1);
                    break;
                case 2:
                    try {
                        bytecode.push_back((Word)std::stoul(insn[1]));
                        break;
                    } catch (const std::invalid_argument &) {}
                default:
                    throw ExecutableError(path, n);
            }
        };
        
        auto parseOpR = [this, insn, path, n] {
            if (insn.size() != 2 || insn[1] != "r")
                throw ExecutableError(path, n);
        };
        
        auto parseOpNR = [this, insn, path, n] {
            switch (insn.size()) {
                case 1:
                    bytecode.push_back(1);
                    break;
                case 2:
                    if (insn[1] == "r")
                        bytecode.push_back(0);
                    else {
                        try {
                            bytecode.push_back(static_cast<Word>(std::stoul(insn[1])));
                        } catch (const std::invalid_argument &) {
                            throw ExecutableError(path, n);
                        }
                        
                        if (bytecode.back() < 2 || bytecode.back() > 99)
                            throw ExecutableError(path, n);
                    }
                    
                    break;
                default:
                    throw ExecutableError(path, n);
            }
        };
        
        auto parseOpNN = [this, insn, path, n] {
            try {
                bytecode.push_back(static_cast<Word>(std::stoul(insn.at(1))));
                bytecode.push_back(static_cast<Word>(std::stoul(insn.at(2))));
            } catch (const std::logic_error &) {
                throw ExecutableError(path, n);
            }
        };
        
        auto parseOpNone = [this, insn, path, n] {
            if (insn.size() != 1)
                throw ExecutableError(path, n);
        };
        
        auto leaveJumpAddr = [this, jmp, path, n](Word pc) {
            if (pc >= jmp.size())
                throw ExecutableError(path, n);
            
            bytecode.push_back(jmp[pc]);
        };
        
        auto parseOpNJ = [this, insn, path, n, leaveJumpAddr] {
            try {
                bytecode.push_back((Word)std::stoul(insn.at(1)));
                leaveJumpAddr((Word)std::stoul(insn[2]));
            } catch (const std::logic_error &) {
                throw ExecutableError(path, n);
            }
        };
        
        auto parseOpJ = [this, insn, path, n, leaveJumpAddr] {
            try {
                leaveJumpAddr((Word)std::stoul(insn.at(1)));
            } catch (const std::logic_error &) {
                throw ExecutableError(path, n);
            }
        };
        
        std::unordered_map<std::string, std::function<void()>> handle = {
#define INSN(n, i, o) \
{#n, [this, parseOp ## o] {\
    bytecode.push_back(Insn ## i);\
    parseOp ## o();\
}}
        
            INSN(eat,   Eat,   NR),
            INSN(go,    Go,    NR),
            INSN(clon,  Clon,  None),
            INSN(str,   Str,   N),
            INSN(left,  Left,  N),
            INSN(right, Right, N),
            INSN(back,  Back,  None),
            INSN(turn,  Turn,  R),
            INSN(jg,    JG,    NJ),
            INSN(jl,    JL,    NJ),
            INSN(j,     J,     J),
            INSN(je,    JE,    J)
            
#undef INSN
        };
        
        try {
            handle.at(insn[0])();
        } catch (const std::out_of_range &) {
            throw ExecutableError(path, n);
        }
        
        n++;
    }
}

ExecutableError::ExecutableError(const std::string &path, int line) {
    reason = path + ":" + std::to_string(line) + ": Syntax error.";
}
