#include "game.hpp"
#include <fstream>
#include <cstring>
#include <functional>
#include <array>
#include <iostream>
#include <cassert>
#include "util.hpp"


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

Game::~Game() noexcept {
    thread.join();
}

bool Game::isValidPosition(int x, int y) const {
    return x < config.getColumnNumber() && y < config.getRowNumber();
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
        
        while (threadCont) {
            auto &league = ileague->second;
            auto unit = league.getNextUnit();
            
            if (!unit) {
                auto next = ileague++;
                
                leagues.erase(ileague);
                if (leagues.size() < 2) {
                    display.stopRefreshing();
                    break;
                }
                
                ileague = next == leagues.end() ? leagues.begin() : next;
                
                continue;
            }
            
            unit->execInsn(*this, league);
            
            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
            
            if (++ileague == leagues.end())
                ileague = leagues.begin();
        }
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
    
    units.reserve(cfg.getUnitsPerLeague());
    
    for (int i = 0; i < cfg.getUnitsPerLeague(); i++) {
        int x, y;
        game.getRandomLocation(x, y);
        
        units.push_back(Unit(skind.sprite, skind.exec, x, y));
        
        game.board[y * game.getConfig().getColumnNumber() + x] = &units.back();
        
        game.display.blitSprite(x, y, skind.sprite);
    }
}

void Unit::execInsn(Game &game, League &league) {
    auto eat = [this] {
        weight++;
    };
    
    auto go = [this, &game] {
        if (!loseWeight(game, 1))
            return;
        
        std::cout << "go\n";
        
        game.board[position.getY() * game.getConfig().getColumnNumber() + position.getX()] = nullptr;
        game.display.blitSprite(position.getX(), position.getY(), 0);
        
        position.move(game, direction);
        
        game.board[position.getY() * game.getConfig().getColumnNumber() + position.getX()] = this;
        game.display.blitSprite(position.getX(), position.getY(), sprite);
    };
    
    auto clon = [this, &game, &league] {
        auto pos = position;
        pos.move(game, direction);
        
        auto &unit = game.board[pos.getY() * game.getConfig().getColumnNumber() + pos.getX()];
        
        if (unit)
            unit->weight += 2;
        else {
            league.units.push_back(Unit(sprite, *exec, pos.getX(), pos.getY()));
            unit = &league.units.back();
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
                eat();
                break;
            case InsnRep::Go:
                go();
                break;
            case InsnRep::Str:
                str();
                break;
            case InsnRep::Left:
                left();
                break;
            case InsnRep::Right:
                right();
                break;
        }
        
        insnRepCnt--;
    } else {
        struct Handler {
            std::function<void()> fn;
            std::size_t size;
        };
        
        std::cout << pc << '\n';
        
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
                }, .size = 2\
            }
            
            REP_HANDLER(Eat, eat),
            REP_HANDLER(Go,  go),
            
            Handler {.fn = clon, .size = 1},
            
            REP_HANDLER(Str,   str),
            REP_HANDLER(Left,  left),
            REP_HANDLER(Right, right),
            
            Handler {.fn = back, .size = 1},
            Handler {.fn = turn, .size = 1},
            
            Handler {
                .fn = [this] {
                    if (weight > (*exec)[pc + 1])
                        pc = (*exec)[pc + 2] - 3;
                },
                .size = 3
            },
            
            Handler {
                .fn = [this] {
                    if (weight < (*exec)[pc + 1])
                        pc = (*exec)[pc + 2] - 3;
                },
                .size = 3
            },
            
            Handler {
                .fn = [this] {
                    pc = (*exec)[pc + 1] - 2;
                },
                .size = 2
            },
            
            Handler {
                .fn = [this, &game] {
                    if (findEnemy(game))
                        pc = (*exec)[pc + 1] - 2;
                },
                .size = 2
            }
            
#undef REP_HANDLER
        };
        
        auto opcode = (*exec)[pc];
        assert(opcode < handlers.size());
        
        auto &h = handlers[opcode];
        h.fn();
        pc += h.size;
        
        std::cout << "pc == " << pc << '\n';
        
        //if (pc >= exec->size())
        //    pc = 0;
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
    
    game.board[position.getY() * game.getConfig().getColumnNumber() + position.getX()] = nullptr;
    
    return false;
}

Unit *League::getNextUnit() {
    auto *unit = &units[nextUnitIndex];
    
    while (unit->isDead()) {
        auto a = std::next(units.begin(), nextUnitIndex + 1);
        auto b = a;
        while (b->isDead())
            b++;
        
        units.erase(a, b);
        
        if (units.empty()) {
            
        }
        
        if (nextUnitIndex == units.size())
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
