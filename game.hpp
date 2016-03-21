#ifndef GAME_HPP
#define GAME_HPP


#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <exception>
#include "ui.hpp"
#include "config.hpp"


// rework Executable, create UnitKind

class Game;
class League;
class Unit;
class Executable;

/*
 * Game.
 */

class Game {
private:
    UIDisplay display;
    const Config &config;
    
public:
    Game(const Config &config)
    : config(config), display(
        0, 0, 0,
        config.getColumnNumber() * config.getSpriteWidth(),
        config.getRowNumber()    * config.getSpriteHeight(),
        config.getColumnNumber(), config.getRowNumber(),
        false, "The Game of Death"
    ) {};
    
    const Config &getConfig() const noexcept {return config;};
    
    int getX()      const noexcept {return display.getX();}
    int getY()      const noexcept {return display.getY();}
    int getWidth()  const noexcept {return display.getWidth();}
    int getHeight() const noexcept {return display.getHeight();}
    
    void start();
};

/*
 * League.
 */

class League {
private:
    std::vector<Unit> units;
    std::unordered_map<std::string, Executable> exec;
    
public:
    League(Game &game, const LeagueInfo &info);
};

/*
 * Unit.
 */

class Unit {
public:
    Game &game;
    const League &league;
    
    enum class Direction {
        North = 0,
        East  = 1,
        South = 2,
        West  = 3,
        Max   = West
    };
    
    static Direction getRandomDirection();
    
    void randomizeDirection();
    
    enum class InsnRep {
        Eat,
        Go,
        Str
    };
    
    typedef long Weight;
    
    Unit(Game &game, const League &league, const Executable &exec);
    
private:
    Direction direction;
    
    InsnRep insnRep;
    int     insnRepCnt;
    
    Weight weight;
    
    const Executable &exec;
    
    std::size_t pc;
};

/*
 * Executable.
 */

class Executable {
private:
    typedef std::vector<std::vector<std::string>> Lines;
    Lines lines;
    
public:
    Executable(const std::string &path);
    
    const Lines &getLines() const noexcept {return lines;}
    
    const std::size_t getLineNumber() const noexcept {return lines.size();}
    const std::vector<std::string> &operator [](std::size_t i) const {
        return lines[i];
    }
};

#endif
