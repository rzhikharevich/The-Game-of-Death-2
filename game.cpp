#include "game.hpp"
#include <fstream>
#include <cstring>
#include "util.hpp"


void Game::start() {
    display.startRefreshing();
}

League::League(Game &game, const LeagueInfo &info) {
    /*exec.reserve(info.unitKinds.size());
    for (auto &kind : info.unitKinds) {
        exec[kind.first] = Executable(kind.second.);
    }
    
    units.reserve(game.getConfig().getUnitsPerLeague());
    
    for (int i = 0; i < units.capacity(); i++)
        units.push_back(Unit(game, *this, ));*/
}

Unit::Direction Unit::getRandomDirection() {
    return (Direction)GetRandom((std::uint32_t)Unit::Direction::Max + 1);
}

void Unit::randomizeDirection() {
    direction = getRandomDirection();
}

Unit::Unit(Game &game, const League &league, const Executable &exec)
: game(game), league(league), exec(exec) {
    randomizeDirection();
    insnRepCnt = 0;
    weight     = 5;
    pc         = 0;
}

Executable::Executable(const std::string &path) {
    auto fs = FileOpenIn(path);
    
    std::string line;
    while (std::getline(fs, line)) {
        std::vector<std::string> tokens;
        
        std::size_t begin = 0;
        std::size_t end;
        while (true) {
            end = line.find(' ', begin);
            
            tokens.push_back(line.substr(begin, end - begin));
            
            if (end == line.npos)
                break;
            
            begin = end + 1;
            if (begin >= line.length())
                break;
        }
        
        lines.push_back(tokens);
    }
}