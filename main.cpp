#include <iostream>
#include <string>
#include <unordered_map>
#include <functional>
#include <stdexcept>
#include "game.hpp"


static void help_exit(const char *exec, int code) {
    (code? std::cerr : std::cout) <<
    "The Game of Death\n"
    "\n"
    "Usage: " << exec << " [options]\n"
    "\n"
    "Game setup files must be in the current working directory.\n"
    "\n"
    "Options:\n"
    " -help         show this help text\n";
    
    std::exit(code);
}

int main(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        std::unordered_map<std::string, std::function<void()>> options = {
            {"-help", [argv] {
                help_exit(argv[0], 0);
            }}
        };
        
        try {
            options.at(argv[i])();
        } catch (const std::out_of_range &exc) {
            std::cerr << "Unrecognized option: '" << argv[i] << "'.\n";
            return 1;
        }
    }
    
    try {
        UIInit();
        std::atexit(UIQuit);
        
        Config config("config.json");
        
        std::cout << "Dumping league information...\n";
        
        for (auto &kv : config.getLeagueInfo()) {
            auto &l = kv.second;
            
            std::cout <<
            kv.first << ":\n"
            "  directory: " << l.directory << "\n"
            "  startKind: " << l.startKind << "\n"
            "  unitKinds:\n";
            
            for (auto &kv : l.unitKinds) {
                auto &k = kv.second;
                
                std::cout <<
                "    " << kv.first << ":\n"
                "      exec: " << k.exec << "\n"
                "      sprite: " << k.sprite << '\n';
            }
        }
        
        Game game(config);
        game.start();
    } catch (const std::exception &exc) {
        std::cout << exc.what() << std::endl;
        return 1;
    }
    
    return 0;
}
