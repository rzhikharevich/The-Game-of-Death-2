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
    "Options:\n"
    " -help         show this help text\n"
    " -bundle PATH  set game setup bundle path\n";
    
    std::exit(code);
}

int main(int argc, char *argv[]) {
    std::string bundlePath = "deathgame";
    bool hasBundlePath = false;
    
    for (int i = 1; i < argc; i++) {
        std::unordered_map<std::string, std::function<void()>> options = {
            {"-help", [argv] {
                help_exit(argv[0], 0);
            }},
            
            {"-bundle", [argv, argc, &i, &bundlePath, &hasBundlePath] {
                if (++i >= argc) {
                    std::cerr << "Expected an argument after '-bundle'.\n";
                    exit(1);
                }
                
                if (hasBundlePath) {
                    std::cerr << "Can't have multiple bundle paths.\n";
                    exit(1);
                }
                
                bundlePath = argv[i];
                hasBundlePath = true;
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
        
        Config config(bundlePath + "/config.json");
        
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
