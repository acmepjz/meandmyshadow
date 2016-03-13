#include <SDL2/SDL_render.h>
#include <SDL2/SDL_ttf.h>

#include <iostream>

#include "Globals.h"
#include "Render.h"

TexturePtr checkAndConvert(SDL_Renderer& renderer, SurfacePtr surface, const char* text) {
    if (!surface) {
        std::cerr << "Fatal error! Failed to render text \"" << text << "\" to texture! Error:"
                  << TTF_GetError()
                  << std::endl;
        std::terminate();
    }
    TexturePtr ret = TexturePtr(SDL_CreateTextureFromSurface(&renderer, surface.get()));
    if (!ret.get()) {
        std::cerr << "Fatal error! Failed to create texture from surface! (" << text << "):"
                  << SDL_GetError()
                  << std::endl;
        std::terminate();
    }
    return ret;
}

TexturePtr textureFromText(SDL_Renderer &renderer,TTF_Font& font,const char *text,SDL_Color color) {
    if (!text || !*text) {
        return nullptr;
    }

    return checkAndConvert(renderer,SurfacePtr(TTF_RenderUTF8_Blended(&font, text, color)),text);
}

TexturePtr textureFromTextShaded(SDL_Renderer &renderer,TTF_Font &font,const char *text,SDL_Color fg,SDL_Color bg) {
    if (!text || !*text) {
        return nullptr;
    }

    return checkAndConvert(renderer,SurfacePtr(TTF_RenderUTF8_Shaded(&font, text, fg,bg)),text);
}

void applyTexture(int x, int y, SDL_Texture &texture, SDL_Renderer & renderer, const SDL_Rect *clip) {
    //Get width/height from the texture.
    const SDL_Rect dstRect = rectFromTexture(x, y, texture);
    SDL_RenderCopy(&renderer, &texture, clip, &dstRect);
}

SurfacePtr createSurface(int width, int height) {
    return SurfacePtr(SDL_CreateRGBSurface(0, width, height, SCREEN_BPP, RMASK, GMASK, BMASK, AMASK));
}

void drawTitleTexture(int screenWidth, SDL_Texture &texture, SDL_Renderer &renderer) {
    SDL_Rect dstRect = rectFromTexture(0,40-TITLE_FONT_RAISE, texture);
    dstRect.x = (screenWidth - dstRect.w) / 2;
    SDL_RenderCopy(&renderer, &texture, NULL, &dstRect);
}
