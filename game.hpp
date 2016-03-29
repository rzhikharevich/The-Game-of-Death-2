#ifndef GAME_HPP
#define GAME_HPP


#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <stdexcept>
#include <thread>
#include "ui.hpp"
#include "config.hpp"


class  Game;
class  GameError;
class  League;
class  Unit;
struct UnitKind;
class  Executable;

/*
 * Game.
 */

class Game {
private:
    UIDisplay display;
    const Config &config;
    std::unordered_map<std::string, League> leagues;
    
    std::thread thread;
    
    std::vector<Unit *> board;
    
    void getRandomLocation(int &x, int &y);
    
public:
    Game(const Config &config);
    ~Game();
    
    SpriteID registerSprite(const Sprite &sprite) {return display.registerSprite(sprite);}
    
    const Config &getConfig() const noexcept {return config;};
    
    bool isValidPosition(int x, int y);
    bool isFreePosition(int x, int y);
    
    void start();
};

/*
 * League.
 */

class League {
private:
    std::unordered_map<std::string, UnitKind> unitKinds;
public:
    League() {}
    League(Game &game, const LeagueInfo &info);
    
    std::vector<Unit> units;
};

/*
 * Unit.
 */

class Unit {
public:
    enum class Direction {
        North = 0,
        East  = 1,
        South = 2,
        West  = 3,
        Max   = West
    };
    
    static Direction getRandomDirection();
    
    enum class InsnRep {
        Eat,
        Go,
        Str
    };
    
    typedef long Weight;
    
    Unit(Game &game, const League &league, SpriteID sprite, const Executable &exec)
    : game(game), league(league), sprite(sprite), exec(exec) {}
    
    SpriteID getSpriteID() {return sprite;}
    
private:
    Game &game;
    const League &league;
    
    Direction direction = getRandomDirection();
    
    InsnRep insnRep;
    int     insnRepCnt = 0;
    
    Weight weight = 5;
    
    SpriteID sprite;
    const Executable &exec;
    
    std::size_t pc = 0;
};

/*
 * Executable.
 */

class Executable {
private:
    typedef std::vector<std::vector<std::string>> Lines;
    Lines lines;
    
public:
    Executable() {}
    Executable(const std::string &path);
    
    const Lines &getLines() const noexcept {return lines;}
    
    const std::size_t getLineNumber() const noexcept {return lines.size();}
    const std::vector<std::string> &operator [](std::size_t i) const {
        return lines[i];
    }
};

/*
 * UnitKind.
 */

struct UnitKind {
    SpriteID   sprite;
    Executable exec;
};


#endif
