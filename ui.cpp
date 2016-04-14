#include "ui.hpp"
#include <chrono>
#include <algorithm>
#include <SDL.h>
#include <SDL_image.h>

using std::shared_ptr;
using std::vector;


enum {
    BG_RED   = 0,
    BG_GREEN = 0,
    BG_BLUE  = 0
};

static vector<UIDisplayInfo> displayInfo;

void UIInit() {
    if (SDL_Init(SDL_INIT_VIDEO))
        throw UIDisplayError();
    
    int displayNumber = SDL_GetNumVideoDisplays();
    
    displayInfo.resize(displayNumber);
    
    for (int i = 0; i < displayNumber; i++) {
        if (SDL_GetDisplayBounds(i, &displayInfo[i].rect))
            throw UIDisplayError();
        
        int modeNumber = SDL_GetNumDisplayModes(i);
        
        displayInfo[i].modes.resize(modeNumber);
        
        for (int j = 0; j < modeNumber; j++)
            if (SDL_GetDisplayMode(i, j, &displayInfo[i].modes[j])) {
                SDL_Quit();
                throw UIDisplayError();
            }
    }
    
    const auto flags = IMG_INIT_PNG;
    
    if ((IMG_Init(flags) & flags) != flags) {
        SDL_Quit();
        throw UIDisplayError();
    }
}

void UIQuit() {
    IMG_Quit();
    SDL_Quit();
    
    displayInfo.clear();
}

Sprite::Sprite(const char *path) : sourceType(SOURCE_IMAGE) {
    if (!(surface = IMG_Load(path)))
        throw UIDisplayError();
}

void Sprite::render(SDL_Renderer *renderer, SDL_Rect *rect) {
    switch (sourceType) {
        case SOURCE_RGB:
            if (SDL_SetRenderDrawColor(renderer, red, green, blue, SDL_ALPHA_OPAQUE))
                throw UIDisplayError();
            
            if (SDL_RenderFillRect(renderer, rect))
                throw UIDisplayError();
            
            break;
        case SOURCE_IMAGE:
            if (!texture) {
                if (!(texture = ImplicitPtr<SDL_Texture>(SDL_CreateTextureFromSurface(renderer, surface), SDL_DestroyTexture)))
                    throw UIDisplayError();
                
                // SDL_FreeSurface may accept NULL, so this should be safe.
                surface.reset((SDL_Surface *)nullptr, SDL_FreeSurface);
            }
            
            SDL_RenderCopy(renderer, texture, nullptr, rect);
            
            break;
    }
}

/*static void setMaxMode(SDL_Window *window) {
    auto maxMode = std::max_element(
        displayInfo[0].modes.begin(), displayInfo[1].modes.end(),
        [](const SDL_DisplayMode &a, const SDL_DisplayMode &b) {
            return a.w * a.h < b.w * b.h;
        }
    );
    
    SDL_SetWindowDisplayMode(window, &*maxMode);
}*/

UIDisplay::UIDisplay(
    int display,
    int x, int y,
    int width, int height,
    int columnNumber, int rowNumber,
    bool fullscreen,
    const char *title) {
    if (x < 0)
        x = SDL_WINDOWPOS_CENTERED_DISPLAY(display);
    else
        x += displayInfo[display].rect.x;
    
    if (y < 0)
        x = SDL_WINDOWPOS_CENTERED_DISPLAY(display);
    else
        y += displayInfo[display].rect.y;
    
    if (width < 0)
        width = displayInfo[display].rect.w;
    
    if (height < 0)
        height = displayInfo[display].rect.h;
    
    this->x = x;
    this->y = y;
    this->width = width;
    this->height = height;
    
    window = ImplicitPtr<SDL_Window>(
        SDL_CreateWindow(
            title,
            x, y, width, height,
            fullscreen? SDL_WINDOW_FULLSCREEN : 0
        ), SDL_DestroyWindow
    );
    
    if (!window)
        throw UIDisplayError();
    
    renderer = ImplicitPtr<SDL_Renderer>(
        SDL_CreateRenderer(
            window, -1,
            SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
        ), SDL_DestroyRenderer
    );
    
    if (!renderer)
        throw UIDisplayError();
    
    if (SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE))
        throw UIDisplayError();
    
    if (SDL_RenderClear(renderer))
        throw UIDisplayError();
    
    SDL_RenderPresent(renderer);
    
    this->columnNumber = columnNumber;
    this->rowNumber = rowNumber;
    
    spriteWidth = width / columnNumber;
    spriteHeight = height / rowNumber;
    
    screen.resize(columnNumber * rowNumber, 0);
    
    // 0 is always the background sprite.
    registerSprite(Sprite(BG_RED, BG_GREEN, BG_BLUE));
}

SpriteID UIDisplay::registerSprite(const Sprite &sprite) {
    sprites.push_back(sprite);
    return (SpriteID)(sprites.size() - 1);
}

void UIDisplay::blitSprite(int x, int y, SpriteID id) {
    screen[y * columnNumber + x] = id;
}

void UIDisplay::refresh() {
    if (SDL_SetRenderDrawColor(renderer, BG_RED, BG_GREEN, BG_BLUE, SDL_ALPHA_OPAQUE) ||
        SDL_RenderClear(renderer))
        throw UIDisplayError();
    
    SDL_Rect rect = {
        .x = x,
        .y = y,
        .w = spriteWidth,
        .h = spriteHeight
    };
    
    for (auto id : screen) {
        sprites[id].render(renderer, &rect);

        if ((rect.x += spriteWidth) >= x + width) {
            rect.x = x;
            rect.y += spriteHeight;
        }
    }
    
    SDL_RenderPresent(renderer);
}

void UIDisplay::startRefreshing() {
    cont = true;
    
    while (cont) {
        SDL_Event evt;
        while (SDL_PollEvent(&evt)) {
            switch (evt.type) {
                case SDL_QUIT:
                    cont = false;
                    break;
            }
        }
        
        refresh();
        
        SDL_Delay(30);
    }
}

void UIDisplay::stopRefreshing() {
    cont = false;
}

UIDisplayError::UIDisplayError() {
    reason = shared_ptr<char>(strdup(SDL_GetError()), FreeString);
}

vector<UIDisplayInfo> UIGetDisplayInfo() {
    return displayInfo;
}
