#include "game.hpp"
#include <fstream>
#include <cstring>
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
    
    for (auto &kv : leagues) {
        for (auto &unit : kv.second.units) {
            int x, y;
            getRandomLocation(x, y);
            
            board[y * config.getColumnNumber() + x] = &unit;
            
            display.blitSprite(x, y, unit.getSpriteID());
        }
    }
}

Game::~Game() {
    if (thread.joinable())
        thread.join();
}

bool Game::isValidPosition(int x, int y) {
    return x < config.getColumnNumber() && y < config.getRowNumber();
}

bool Game::isFreePosition(int x, int y) {
    return isValidPosition(x, y) && !board[y * config.getColumnNumber() + x];
}

void Game::getRandomLocation(int &x, int &y) {
    do {
        x = GetRandom(config.getColumnNumber());
        y = GetRandom(config.getRowNumber());
    } while (!isFreePosition(x, y));
}

void Game::start() {
    thread = std::thread([this]{
        
    });
    
    display.startRefreshing();
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
    for (int i = 0; i < cfg.getUnitsPerLeague(); i++)
        units.push_back(Unit(game, *this, skind.sprite, skind.exec));
}

Unit::Direction Unit::getRandomDirection() {
    return (Direction)GetRandom((std::uint32_t)Direction::Max + 1);
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