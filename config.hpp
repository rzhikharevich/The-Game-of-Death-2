#ifndef CONFIG_HPP
#define CONFIG_HPP


#include <memory>
#include <string>
#include <unordered_map>
#include <exception>
#include <cstdint>
#include <json/json.h>


class  Config;
class  ConfigError;
struct LeagueInfo;
struct UnitKindInfo;
struct SpriteInfo;

/*
 * Config.
 */

class Config {
private:
    Json::Value root;
    
    std::unordered_map<std::string, LeagueInfo> leagueInfo;
    
public:
    void parse(const char *path, Json::Value &root);
    
    Config(const char *path);
    Config(const std::string &path) : Config(path.c_str()) {};
    
    bool override(const char *path);
    
    int getSpriteWidth()  const {return root.get("spriteWidth",  20).asInt();}
    int getSpriteHeight() const {return root.get("spriteHeight", 20).asInt();}
    int getColumnNumber() const {return root.get("columnNumber", 20).asInt();}
    int getRowNumber()    const {return root.get("rowNumber",    20).asInt();}
    
    void setSpriteSize(int w, int h) {
        root["spriteWidth"]  = w;
        root["spriteHeight"] = h;
    }
    
    int  getMoveDelay() const    {return root.get("moveDelay", 250).asInt();}
    void setMoveDelay(int delay) {root["moveDelay"] = delay;}
    
    int getMaxMoves() const {return root.get("maxMoves", 1000000).asInt();}
    
    int getUnitsPerLeague() const noexcept {return root.get("unitsPerLeague", 10).asInt();}
    const std::unordered_map<std::string, LeagueInfo> &getLeagueInfo() const noexcept {return leagueInfo;}
};

/*
 * ConfigError.
 */

class ConfigError : public std::exception {
private:
    std::string reason;
    
public:
    ConfigError(const std::string &reason);
    ConfigError(const Json::Reader &reader);
    
    virtual const char *what() const noexcept {return reason.c_str();}
};

/*
 * LeagueInfo.
 */

struct LeagueInfo {
    std::string directory;
    std::string startKind;
    std::unordered_map<std::string, UnitKindInfo> unitKinds;
};

/*
 * UnitKindInfo.
 */

struct UnitKindInfo {
    std::string exec;
    std::shared_ptr<SpriteInfo> sprite;
};

/*
 * SpriteInfo.
 */

struct SpriteInfo {
    enum class Type {
        SquareRGB,
        ImagePath
    };
    
    virtual Type getType() const noexcept = 0;
};

struct SpriteInfoSquareRGB : public SpriteInfo {
    virtual Type getType() const noexcept {return Type::SquareRGB;}
    
    std::uint8_t red, green, blue;
    
    SpriteInfoSquareRGB(int i) {
        red   = i >> 16;
        green = i >> 8;
        blue  = i;
    }
};

struct SpriteInfoImagePath : std::string, SpriteInfo {
    virtual Type getType() const noexcept {return Type::ImagePath;}
    
    SpriteInfoImagePath(const std::string &string) : std::string(string) {}
};

std::ostream &operator <<(std::ostream &os, const SpriteInfo *info);


#endif
