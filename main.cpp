#include <iostream>
#include "ui.hpp"


int main(int argc, char *argv[]) {
    try {
        UIInit();
        
        UIDisplay disp(0, 0, 0, 400, 400, 20, 20, false);
        
        auto red   = disp.registerSprite(Sprite(0xFF, 0x00, 0x00));
        auto green = disp.registerSprite(Sprite("/Users/roman/Downloads/slime_2.png"));
        
        disp.blitSprite(0, 0, red);
        disp.blitSprite(0, 1, green);
        disp.blitSprite(1, 0, green);
        
        disp.startRefreshing();
        
        UIQuit();
    } catch (const UIDisplayError &error) {
        std::cout << "UIDisplayError: " << error.what() << '\n';
    }
    
    return 0;
}
