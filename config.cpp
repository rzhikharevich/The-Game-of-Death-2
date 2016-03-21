#include "config.hpp"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <array>
#include <unordered_map>
#include <cctype>
#include "util.hpp"


static const std::string &getTypeString(Json::ValueType type) {
    static const std::array<std::string, 8> strings = {
        "null",
        "int",
        "uint",
        "real",
        "string",
        "boolean",
        "array",
        "object"
    };
    
    static const std::string unknown = "unknown";
    
    if (static_cast<std::size_t>(type) >= strings.size())
        return unknown;
    
    return strings[type];
}

static void checkMembers(
    const std::string &objectPath,
    const Json::Value &object,
    const std::unordered_map<std::string, Json::ValueType> &recognized
) {
    std::string errors;
    
    for (auto &name : object.getMemberNames()) {
        Json::ValueType type;
        
        try {
            type = recognized.at(name);
        } catch (std::out_of_range &exc) {
            errors += "Unrecognized member '" + name +  "' found in the " + objectPath + " object.";
            continue;
        }
        
        if (object[name].type() != type &&
            type != Json::ValueType::nullValue) {
            if (!errors.empty())
                errors += '\n';
            
            errors +=
            "Member " + objectPath + "." + name + " must have type '" + getTypeString(type) +
            "', but has type '" + getTypeString(object[name].type()) + "'.";
        }
    }
    
    if (!errors.empty())
        throw ConfigError(errors);
}

static void checkSpriteInfo(const std::string &parentPath, const Json::Value &info) {
    if (info.isString())
        return;
    
    throw ConfigError(
        "Member " + parentPath + ".sprite must be either a valid RGB value "
        "or a path to an image file."
    );
}

static SpriteInfo *parseSpriteInfo(const Json::Value &value) {
    auto s = value.asString();
    
    if (s.front() == '#') {
        try {
            return new SpriteInfoSquareRGB(std::stoi(s.substr(1), nullptr, 16));
        } catch (...) {/* ignore */}
    }
    
    return new SpriteInfoImagePath(s);
}

void Config::parse(const char *path, Json::Value &root) {
    auto fs = FileOpenIn(path);
    
    Json::Reader reader;
    if (!reader.parse(fs, root))
        throw ConfigError(reader);
    
    if (!root.isObject())
        throw ConfigError("The root must an object.");
    
    checkMembers("root", root, {
        {"spriteWidth",    Json::ValueType::intValue},
        {"spriteHeight",   Json::ValueType::intValue},
        {"columnNumber",   Json::ValueType::intValue},
        {"rowNumber",      Json::ValueType::intValue},
        {"unitsPerLeague", Json::ValueType::intValue},
        {"leagues",        Json::ValueType::objectValue}
    });
    
    for (auto &league : root["leagues"].getMemberNames()) {
        const std::string leaguePath = "leagues." + league;
        
        checkMembers(leaguePath, root["leagues"][league], {
            {"directory",     Json::ValueType::stringValue},
            {"defaultSprite", Json::ValueType::nullValue},
            {"startKind",     Json::ValueType::stringValue},
            {"unitKinds",     Json::ValueType::objectValue}
        });
        
        Json::Value defaultSprite;
        
        if (root["leagues"][league].isMember("defaultSprite")) {
            checkSpriteInfo(leaguePath, root["leagues"][league]["defaultSprite"]);
            defaultSprite = root["leagues"][league]["defaultSprite"];
        } else
            defaultSprite = GetRandom(0x1000000);
        
        LeagueInfo info = {
            .directory = root["leagues"][league].get("directory", league).asString(),
            .startKind = root["leagues"][league].get("startKind", "start").asString()
        };
        
        for (auto &kind : root["leagues"][league]["unitKinds"].getMemberNames()) {
            const std::string unitKindPath = leaguePath + ".unitKinds." + kind;
            
            checkMembers(unitKindPath, root["leagues"][league]["unitKinds"][kind], {
                {"exec",   Json::ValueType::stringValue},
                {"sprite", Json::ValueType::nullValue}
            });
            
            if (root["leagues"][league]["unitKinds"][kind].isMember("sprite"))
                checkSpriteInfo(unitKindPath, root["leagues"][league]["unitKinds"][kind]["sprite"]);
            
            info.unitKinds[kind] = {
                .exec = root["leagues"][league]["unitKinds"][kind]["exec"].asString(),
                .sprite = std::shared_ptr<SpriteInfo>(
                    parseSpriteInfo(
                        root["leagues"][league]["unitKinds"][kind].get("sprite", defaultSprite)
                    )
                )
            };
        }
        
        leagueInfo[league] = info;
    }
}

Config::Config(const char *path) {
    parse(path, root);
}

bool Config::override(const char *path) {
    Json::Value root2;
    parse(path, root2);
    
    if (!root2.isObject())
        return true;
    
    for (auto &name : root2.getMemberNames()) {
        if (root[name].isArray() && root2[name].isArray())
            root[name].append(root2[name]);
        else
            root[name] = root2[name];
    }
    
    return true;
}

ConfigError::ConfigError(const std::string &reason) {
    this->reason = reason;
}

ConfigError::ConfigError(const Json::Reader &reader) {
    reason = reader.getFormattedErrorMessages();
}

std::ostream &operator <<(std::ostream &os, const SpriteInfo *info) {
    switch (info->getType()) {
        case SpriteInfo::Type::SquareRGB: {
            auto rgb = static_cast<const SpriteInfoSquareRGB *>(info);
            
            auto flags = std::cout.flags();
            auto fill  = std::cout.fill('0');
            
            os << '#' << std::uppercase << std::setfill('0') <<
            std::hex << std::setw(2) << (int)rgb->red <<
            std::hex << std::setw(2) << (int)rgb->green <<
            std::hex << std::setw(2) << (int)rgb->blue;
            
            std::cout.flags(flags);
            std::cout.fill(fill);
            
            return os;
        } case SpriteInfo::Type::ImagePath:
            return os << *static_cast<const SpriteInfoImagePath *>(info);
    }
}