#ifndef UI_HPP
#define UI_HPP


#include <memory>
#include <exception>
#include <vector>
#include <thread>
#include <SDL2/SDL.h>
#include "util.hpp"


void UIInit();
void UIQuit();

class Sprite {
private:
    enum {
        SOURCE_RGB,
        SOURCE_IMAGE
    } sourceType;
    
    std::uint8_t red, green, blue;
    
    ImplicitPtr<SDL_Surface> surface;
    
    ImplicitPtr<SDL_Texture> texture;
    
public:
    Sprite(std::uint8_t red, std::uint8_t green, std::uint8_t blue) : sourceType(SOURCE_RGB), red(red), green(green), blue(blue) {};
    Sprite(const char *path);
    
    void render(SDL_Renderer *renderer, SDL_Rect *rect);
};

typedef unsigned int SpriteID;

class UIDisplay {
private:
    int x, y, width, height;
    
    std::vector<Sprite> sprites;
    
    std::vector<SpriteID> screen;
    
    ImplicitPtr<SDL_Window> window;
    ImplicitPtr<SDL_Renderer> renderer;
    
    int spriteWidth, spriteHeight;
    int columnNumber, rowNumber;
    
    volatile bool cont;
    
public:
    UIDisplay(
        int display = 0,
        int x = 0, int y = 0,
        int width = -1, int height = -1,
        int columnNumber = 20, int rowNumber = 20,
        bool fullscreen = false,
        const char *title = ""
    );
    
    int getX()      const noexcept {return x;}
    int getY()      const noexcept {return y;}
    int getWidth()  const noexcept {return width;}
    int getHeight() const noexcept {return height;}
    
    SpriteID registerSprite(const Sprite &sprite);
    void blitSprite(int x, int y, SpriteID id);
    
    void refresh();
    void startRefreshing();
    void stopRefreshing();
};

class UIDisplayError : public std::exception {
private:
    ImplicitPtr<char> reason;
    
public:
    UIDisplayError();
    virtual const char *what() const noexcept {return reason;}
};

typedef struct {
    SDL_Rect rect;
    std::vector<SDL_DisplayMode> modes;
} UIDisplayInfo;

std::vector<UIDisplayInfo> UIGetDisplayInfo();


#endif
