#ifndef GAME_HPP
#define GAME_HPP


#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <stdexcept>
#include <thread>
#include <cstdlib>
#include <ostream>

#include "executable.hpp"
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
    friend std::ostream &operator<<(std::ostream &os, const Game &game);
    
    friend League;
    friend Unit;
    
private:
    UIDisplay display;
    const Config &config;
    
    typedef std::unordered_map<std::string, League> LeagueMap;
    
    LeagueMap leagues;
    
    std::thread thread;
    volatile bool threadCont;
    
    std::vector<Unit *> board;
    
public:
    Game(const Config &config);
    ~Game();
    
    SpriteID registerSprite(const Sprite &sprite) {return display.registerSprite(sprite);}
    
    const Config &getConfig() const {return config;};
    
    const LeagueMap &getLeagues() const {return leagues;}
    
    bool isValidPosition(int x, int y) const;
    bool isFreePosition(int x, int y) const;
    
    void getRandomLocation(int &x, int &y);
    
    void start();
};

/*
 * League.
 */

class League {
private:
    std::unordered_map<std::string, UnitKind> unitKinds;
    std::size_t nextUnitIndex = 0;
    
    std::vector<Unit> staging;
    
public:
    League() {}
    League(Game &game, const LeagueInfo &info);
    
    std::vector<Unit> units;
    Unit *getNextUnit();
    
    std::uint64_t getTotalBiomass() const;
};

static inline bool operator<(const League &a, const League &b) {
    return a.getTotalBiomass() < b.getTotalBiomass();
}

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
    
    class Position {
    private:
        int x, y;
    public:
        Position(int x, int y) : x(x), y(y) {};
        
        int getX() const {return x;}
        int getY() const {return y;}
        
        void move(const Game &game, Direction dir);
        void move(Direction dir);
        
        std::string stringValue();
    };
    
    static Direction getRandomDirection();
    
    enum class InsnRep {
        Eat,
        Go,
        Str
    };
    
    typedef long Weight;
    
    Unit(/*Game &game, const League &league,*/ SpriteID sprite, Executable *exec, int x, int y, bool newborn = false)
    : /*game(game), league(league),*/ sprite(sprite), exec(exec), position(x, y), weight(newborn? 2 : 5) {}
    
    /*Unit(const Unit &src) :
    position(src.position), direction(src.direction),
    insnRep(src.insnRep), insnRepCnt(src.insnRepCnt),
    weight(src.weight),
    sprite(src.sprite), exec(src.exec),
    pc(src.pc) {}*/
    
    //const Unit &operator=(const Unit &src) {return *this = Unit(src);}
    
    void execInsn(Game &game, League &league);
    
    const Position getPosition() const {return position;}
    
    Weight getWeight() const {return weight;}
    bool isDead() const {return weight <= 0;}
    
    SpriteID getSpriteID() const {return sprite;}
    
private:
    /*Game &game;
    const League &league;*/
    
    Position position;
    
    Unit *findEnemy(Game &game);
    
    Direction direction = getRandomDirection();
    
    InsnRep insnRep;
    int     insnRepCnt = 0;
    
    Weight weight;
    
    bool loseWeight(Game &game, Weight loss = 1);
    
    void damage(Game &game, Weight attackerWeight) {
        loseWeight(game, GetRandom(static_cast<std::uint32_t>(3 + attackerWeight / 2)));
    }
    
    SpriteID sprite;
    Executable *exec;
    
    Executable::Word pc = 0;
};

static inline Unit::Direction operator+(Unit::Direction dir, int offs) {
    return static_cast<Unit::Direction>((static_cast<int>(dir) + offs) % 4);
}

static inline Unit::Direction &operator+=(Unit::Direction &dir, int offs) {
    return dir = dir + 1;
}

static inline Unit::Direction &operator++(Unit::Direction &dir) {
    return dir += 1;
}

static inline Unit::Direction operator++(Unit::Direction &dir, int) {
    auto ret = dir;
    dir += 1;
    return ret;
}

static inline Unit::Direction operator-(Unit::Direction dir, int offs) {
    return static_cast<Unit::Direction>(std::abs((static_cast<int>(dir) - offs) % 4));
}

static inline Unit::Direction &operator-=(Unit::Direction &dir, int offs) {
    return dir = dir - 1;
}

static inline Unit::Direction &operator--(Unit::Direction &dir) {
    return dir -= 1;
}

static inline Unit::Direction operator--(Unit::Direction &dir, int) {
    auto ret = dir;
    dir -= 1;
    return ret;
}

static inline Unit::Direction operator~(Unit::Direction dir) {
    return static_cast<Unit::Direction>((static_cast<int>(dir) + 2) % 4);
}

/*
 * UnitKind.
 */

struct UnitKind {
    SpriteID   sprite;
    Executable exec;
};


#endif
