#include <iostream>
#include <string>
#include <unordered_map>
#include <functional>
#include <stdexcept>
#include "game.hpp"
#include "util.hpp"


static void help_exit(const char *exec, int code) {
    (code ? std::cerr : std::cout) <<
    "The Game of Death\n"
    "\n"
    "Usage: " << exec << " [options]\n"
    "\n"
    "Game setup files must be in the current working directory.\n"
    "\n"
    "Options:\n"
    " -help             show this help text\n"
    " -sprite-size WxH  set sprite size overriding configuration\n";
    
    std::exit(code);
}

int main(int argc, char *argv[]) {
    int spriteWidth  = -1;
    int spriteHeight = -1;
    
    for (int i = 1; i < argc; i++) {
        std::unordered_map<std::string, std::function<void()>> options = {
            {"-help", [argv] {
                help_exit(argv[0], 0);
            }},
            
            {"-sprite-size", [argv, argc, &i, &spriteWidth, &spriteHeight] {
                if (++i >= argc) {
                    std::cerr << "Argument expected after flag '-sprite-size'.\n";
                    help_exit(argv[0], 1);
                }
                
                std::string wh = argv[i];
                
                auto sep = wh.find('x');
                
                try {
                    spriteWidth  = std::stoi(wh.substr(0, sep));
                    spriteHeight = std::stoi(wh.substr(sep + 1));
                } catch (...) {
                    std::cerr << "Flag '-sprite-size' argument is invalid, it must be two integers separated by 'x'.\n";
                    help_exit(argv[0], 1);
                }
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
        
        if (spriteWidth > 0)
            config.setSpriteSize(spriteWidth, spriteHeight);
        
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
