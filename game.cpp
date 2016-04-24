#include "game.hpp"
#include <fstream>
#include <cstring>
#include <functional>
#include <array>
#include <cassert>
#include "util.hpp"

#include <iostream>


Game::Game(const Config &config)
: config(config), display(
    0, 0, 0,
    config.getColumnNumber() * config.getSpriteWidth(),
    config.getRowNumber()    * config.getSpriteHeight(),
    config.getColumnNumber(), config.getRowNumber(),
    false, "The Game of Death"
) {
    board.resize(config.getColumnNumber() * config.getRowNumber(), nullptr);
    
    if (config.getUnitsPerLeague() * config.getLeagueInfo().size() > board.size())
        throw std::invalid_argument("Too many units requested.");
    
    for (auto &kv : config.getLeagueInfo())
        leagues[kv.first] = League(*this, kv.second);
}

Game::~Game() {
    thread.join();
}

void Game::placeUnit(Unit &unit) {
    auto &pos = unit.getPosition();
    board[pos.getY() * config.getColumnNumber() + pos.getX()] = &unit;
    display.blitSprite(pos.getX(), pos.getY(), unit.getSpriteID());
}

void Game::removeUnit(const Unit &unit) {
    auto pos = unit.getPosition();
    board[pos.getY() * config.getColumnNumber() + pos.getX()] = nullptr;
    display.blitSprite(pos.getX(), pos.getY(), 0);
}

bool Game::isValidPosition(int x, int y) const {
    return
    x > 0 && x < config.getColumnNumber() &&
    y > 0 && y < config.getRowNumber();
}

bool Game::isFreePosition(int x, int y) const {
    return isValidPosition(x, y) && !board[y * config.getColumnNumber() + x];
}

void Game::getRandomLocation(int &x, int &y) {
    do {
        x = GetRandom(config.getColumnNumber());
        y = GetRandom(config.getRowNumber());
    } while (!isFreePosition(x, y));
}

void Game::start() {
    threadCont = true;
    
    thread = std::thread([this] {
        int delay = config.getMoveDelay();
        
        auto ileague = std::next(leagues.begin(), GetRandom((std::uint32_t)leagues.size()));
        
        int move = 0;
        int maxMoves = config.getMaxMoves();
        
        while (threadCont) {
            auto &league = ileague->second;
            
            std::cout << ileague->first << ": " << league.getTotalBiomass() << '/' << league.units.size() << '\n';
            
            if (auto unit = league.getNextUnit()) {
                unit->execInsn(*this, league);
                
                if (++move == maxMoves)
                    break;
                
                std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                
                if (++ileague == leagues.end())
                    ileague = leagues.begin();
            } else {
                ileague = leagues.erase(ileague);
                if (leagues.size() < 2)
                    break;
                
                if (ileague == leagues.end())
                    ileague = leagues.begin();
            }
        }
        
        display.stopRefreshing();
    });
    
    display.startRefreshing();
    
    threadCont = false;
}

static Sprite spriteFromInfo(const std::string &directory, const SpriteInfo *info) {
    switch (info->getType()) {
        case SpriteInfo::Type::SquareRGB: {
            auto rgb = static_cast<const SpriteInfoSquareRGB *>(info);
            return Sprite(rgb->red, rgb->green, rgb->blue);
        }
        case SpriteInfo::Type::ImagePath:
            auto path = static_cast<const SpriteInfoImagePath *>(info);
            return Sprite((directory + '/' + *path).c_str());
    }
}

League::League(Game &game, const LeagueInfo &info) {
    unitKinds.reserve(info.unitKinds.size());
    for (auto &kv : info.unitKinds) {
        unitKinds[kv.first] = {
            .sprite = game.registerSprite(spriteFromInfo(info.directory, kv.second.sprite.get())),
            .exec   = Executable(info.directory + '/' + kv.second.exec)
        };
    }
    
    auto &cfg = game.getConfig();

    auto &skind = unitKinds[info.startKind];
    
    units.reserve(cfg.getColumnNumber() * cfg.getRowNumber());
    
    for (int i = 0; i < cfg.getUnitsPerLeague(); i++) {
        int x, y;
        game.getRandomLocation(x, y);
        
        units.push_back(Unit(skind.sprite, &skind.exec, x, y));
        
        game.placeUnit(units.back());
    }
}

std::uint64_t League::getTotalBiomass() const {
    std::uint64_t total = 0;
    
    for (auto &unit : units)
        if (!unit.isDead())
            total += unit.getWeight();
    
    return total;
}

void Unit::Position::move(const Game &game, Direction dir) {
    auto pos = *this;
    pos.move(dir);
    
    if (game.isFreePosition(pos.getX(), pos.getY()))
        *this = pos;
}

void Unit::Position::move(Unit::Direction dir) {
    switch (dir) {
        case Direction::North:
            y--;
            break;
        case Direction::East:
            x++;
            break;
        case Direction::South:
            y++;
            break;
        case Direction::West:
            x--;
            break;
    }
}

std::string Unit::Position::stringValue() {
    return std::to_string(getX()) + "," + std::to_string(getY());
}

void Unit::execInsn(Game &game, League &league) {
    auto eat = [this] {
        weight++;
    };
    
    auto go = [this, &game] {
        if (!loseWeight(game, 1))
            return;
        
        game.removeUnit(*this);
        position.move(game, direction);
        game.placeUnit(*this);
    };
    
    auto clon = [this, &game, &league] {
        if (!loseWeight(game, 10))
            return;
        
        auto pos = position;
        pos.move(game, direction);
        
        if (!game.isValidPosition(pos.getX(), pos.getY()))
            return;
        
        if (auto unit = game.board[pos.getY() * game.getConfig().getColumnNumber() + pos.getX()])
            unit->weight += 2;
        else {
            league.units.push_back(Unit(sprite, exec, pos.getX(), pos.getY(), true));
            game.placeUnit(league.units.back());
        }
    };
    
    auto str = [this, &game] {
        if (loseWeight(game, 1))
            if (auto enemy = findEnemy(game))
                enemy->damage(game, weight);
    };
    
    auto left = [this] {
        direction--;
    };
    
    auto right = [this] {
        direction++;
    };
    
    auto back = [this] {
        direction = ~direction;
    };
    
    auto turn = [this] {
        direction = getRandomDirection();
    };
    
    if (insnRepCnt) {
        switch (insnRep) {
            case InsnRep::Eat:
                std::cout << "rep eat\n";
                eat();
                break;
            case InsnRep::Go:
                std::cout << "rep go\n";
                go();
                break;
            case InsnRep::Str:
                std::cout << "rep str\n";
                str();
                break;
        }
        
        insnRepCnt--;
    } else {
        struct Handler {
            std::function<void()> fn;
            std::size_t size;
            bool pseudo;
        };
        
        std::array<Handler, Executable::InsnMax + 1> handlers = {
#define REP_HANDLER(c, f) \
            Handler {\
                .fn = [this, f] {\
                    insnRep = InsnRep::c;\
                    insnRepCnt = (*exec)[pc + 1] ? (*exec)[pc + 1] : GetRandom(5);\
                    \
                    if (!insnRepCnt)\
                        return;\
                    \
                    f();\
                    \
                    insnRepCnt--;\
                }, .size = 2, .pseudo = false\
            }
            
            REP_HANDLER(Eat, eat),
            REP_HANDLER(Go,  go),
            
            Handler {.fn = clon, .size = 1, .pseudo = false},
            
            REP_HANDLER(Str, str),
            
            Handler {.fn = left,  .size = 1, .pseudo = true},
            Handler {.fn = right, .size = 1, .pseudo = true},
            Handler {.fn = back,  .size = 1, .pseudo = true},
            Handler {.fn = turn,  .size = 1, .pseudo = true},
            
            Handler {
                .fn = [this] {
                    if (weight > (*exec)[pc + 1])
                        pc = (*exec)[pc + 2];
                    else
                        pc += 3;
                },
                .size = 0, .pseudo = true
            },
            
            Handler {
                .fn = [this] {
                    if (weight < (*exec)[pc + 1])
                        pc = (*exec)[pc + 2];
                    else
                        pc += 3;
                },
                .size = 0, .pseudo = true
            },
            
            Handler {
                .fn = [this] {
                    pc = (*exec)[pc + 1];
                },
                .size = 0, .pseudo = true
            },
            
            Handler {
                .fn = [this, &game] {
                    if (findEnemy(game))
                        pc = (*exec)[pc + 1];
                    else
                        pc += 2;
                },
                .size = 0, .pseudo = true
            }
            
#undef REP_HANDLER
        };
        
        int mad = 0;
        while (true) {
            auto opcode = (*exec)[pc];
            assert(opcode < handlers.size());
            
            std::cout << DisasmOpcode(opcode) << '\n';
            
            auto &h = handlers[opcode];
            h.fn();
            pc += h.size;
            
            if (pc >= exec->size())
                pc = 0;
            
            if (!h.pseudo || weight <= 0)
                break;
            
            if (++mad >= 31) {
                loseWeight(game, 5);
                break;
            }
        }
    }
}

Unit *Unit::findEnemy(Game &game) {
    auto pos = position;
    
    pos.move(game, direction);
    
    if (auto enemy = game.board[pos.getY() * game.getConfig().getColumnNumber() + pos.getX()])
        return enemy;
    
    auto dir = direction + 1;
    
    pos.move(game, dir);
    
    if (auto enemy = game.board[pos.getY() * game.getConfig().getColumnNumber() + pos.getX()])
        return enemy;
    
    for (int i = 0; i < 3; i++) {
        dir++;
        
        for (int i = 0; i < 2; i++) {
            if (auto enemy = game.board[pos.getY() * game.getConfig().getColumnNumber() + pos.getX()])
                return enemy;
            
            pos.move(game, dir);
        }
    }
    
    return game.board[pos.getY() * game.getConfig().getColumnNumber() + pos.getX()];
}

bool Unit::loseWeight(Game &game, Weight loss) {
    if ((weight -= loss) > 0)
        return true;
    
    game.removeUnit(*this);
    
    return false;
}

Unit *League::getNextUnit() {
    if (units.empty())
        return nullptr;
    
    auto *unit = &units[nextUnitIndex];
    
    while (unit->isDead()) {
        auto a = units.begin();
        std::advance(a, nextUnitIndex);
        
        auto b = a + 1;
        while (b != units.end() && b->isDead())
            b++;
        
        units.erase(a, b);
        
        if (units.empty())
            return nullptr;
        
        if (nextUnitIndex >= units.size())
            nextUnitIndex = 0;
        
        unit = &units[nextUnitIndex];
    }
    
    if (++nextUnitIndex >= units.size())
        nextUnitIndex = 0;
    
    return unit;
}

Unit::Direction Unit::getRandomDirection() {
    return (Direction)GetRandom((std::uint32_t)Direction::Max + 1);
}
