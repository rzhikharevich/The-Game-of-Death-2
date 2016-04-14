#include "game.hpp"
#include <fstream>
#include <cstring>
#include <functional>
#include <array>
#include <cassert>
#include "util.hpp"

//#include <iostream>


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
            
            if (auto unit = league.getNextUnit()) {
                unit->execInsn(*this, league);
                
                if (++move == maxMoves)
                    break;
            } else {
                auto next = std::next(ileague);
                
                leagues.erase(ileague);
                if (leagues.size() < 2) {
                    display.stopRefreshing();
                    break;
                }
                
                ileague = next == leagues.end() ? leagues.begin() : next;
                
                continue;
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
            
            if (++ileague == leagues.end())
                ileague = leagues.begin();
        }
    });
    
    display.startRefreshing();
    
    threadCont = false;
}

/*std::ostream &operator<<(std::ostream &os, const Game &game) {
    
}*/

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

std::uint64_t League::getTotalBiomass() const {
    std::uint64_t total = 0;
    
    for (auto &unit : units)
        total += unit.getWeight();
    
    return total;
}

void Unit::Position::move(const Game &game, Direction dir) {
    int x1 = x;
    int y1 = y;
    
    switch (dir) {
        case Direction::North:
            y1--;
            break;
        case Direction::East:
            x1++;
            break;
        case Direction::South:
            y1++;
            break;
        case Direction::West:
            x1--;
            break;
    }
    
    if (game.isFreePosition(x1, y1)) {
        x = x1;
        y = y1;
    }
}

void Unit::execInsn(Game &game, League &league) {
    //std::cout << "pc == " << pc << "        ";
    
    auto eat = [this] {
        weight++;
    };
    
    auto go = [this, &game] {
        //std::cout << "go\n";
        
        if (!loseWeight(game, 1))
            return;
        
        game.board[position.getY() * game.getConfig().getColumnNumber() + position.getX()] = nullptr;
        game.display.blitSprite(position.getX(), position.getY(), 0);
        
        position.move(game, direction);
        
        game.board[position.getY() * game.getConfig().getColumnNumber() + position.getX()] = this;
        game.display.blitSprite(position.getX(), position.getY(), sprite);
    };
    
    auto clon = [this, &game, &league] {
        if (!loseWeight(game, 10))
            return;
        
        auto pos = position;
        pos.move(game, direction);
        
        auto &unit = game.board[pos.getY() * game.getConfig().getColumnNumber() + pos.getX()];
        
        if (unit)
            unit->weight += 2;
        else {
            league.units.push_back(Unit(sprite, *exec, pos.getX(), pos.getY(), true));
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
        //std::cout << "turn\n";
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
                        pc = (*exec)[pc + 2] - 3;
                },
                .size = 3, .pseudo = true
            },
            
            Handler {
                .fn = [this] {
                    if (weight < (*exec)[pc + 1])
                        pc = (*exec)[pc + 2] - 3;
                },
                .size = 3, .pseudo = true
            },
            
            Handler {
                .fn = [this] {
                    //std::cout << "j " << (*exec)[pc + 1] << '\n';
                    pc = (*exec)[pc + 1] - 2;
                },
                .size = 2, .pseudo = true
            },
            
            Handler {
                .fn = [this, &game] {
                    if (findEnemy(game))
                        pc = (*exec)[pc + 1] - 2;
                },
                .size = 2, .pseudo = true
            }
            
#undef REP_HANDLER
        };
        
        int mad = 0;
        while (true) {
            auto opcode = (*exec)[pc];
            assert(opcode < handlers.size());
            
            auto &h = handlers[opcode];
            h.fn();
            pc += h.size;
            
            if (!h.pseudo)
                break;
            
            if (++mad >= 31) {
                loseWeight(game, 5);
                break;
            }
        }
        
        if (pc >= exec->size())
            pc = 0;
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
    game.display.blitSprite(position.getX(), position.getY(), 0);
    
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
    }
    
    if (++nextUnitIndex >= units.size())
        nextUnitIndex = 0;
    
    return unit;
}

Unit::Direction Unit::getRandomDirection() {
    return (Direction)GetRandom((std::uint32_t)Direction::Max + 1);
}
