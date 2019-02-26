#include <SDL_render.h>
#include <SDL_ttf_fontfallback.h>

#include <iostream>

#include "Globals.h"
#include "Render.h"
namespace {
    const char* NO_TEXT = " ";
}

TexturePtr checkAndConvert(SDL_Renderer& renderer, SurfacePtr surface, const char* text) {
    if (!surface) {
        std::cerr << "Fatal error! Failed to render text \"" << text << "\" to texture! Error:"
                  << TTF_GetError()
                  << std::endl;
        std::terminate();
    }
    TexturePtr ret = TexturePtr(SDL_CreateTextureFromSurface(&renderer, surface.get()));
    if (!ret) {
        std::cerr << "Fatal error! Failed to create texture from surface! (" << text << "):"
                  << SDL_GetError()
                  << std::endl;
        std::terminate();
    }
    return ret;
}

TexturePtr textureFromText(SDL_Renderer &renderer,TTF_Font& font,const char *text,SDL_Color color) {
    if (!text || !*text) {
        // Make sure we return a texture even if there is no text provided.
        text = NO_TEXT;
    }

    return checkAndConvert(renderer,SurfacePtr(TTF_RenderUTF8_Blended(&font, text, color)),text);
}

TexturePtr titleTextureFromText(SDL_Renderer& renderer, const char* text, SDL_Color color, int width) {
	if (!text || !*text) {
		// Make sure we return a texture even if there is no text provided.
		text = NO_TEXT;
	}

	int w;

	TTF_SizeUTF8(fontTitle, text, &w, NULL);
	if (w <= width) {
		return textureFromText(renderer, *fontTitle, text, color);
	}

	TTF_SizeUTF8(fontGUI, text, &w, NULL);
	if (w <= width) {
		return textureFromText(renderer, *fontGUI, text, color);
	}

	return textureFromText(renderer, *fontGUISmall, text, color);
}


TexturePtr textureFromTextShaded(SDL_Renderer &renderer,TTF_Font &font,const char *text,SDL_Color fg,SDL_Color bg) {
    if (!text || !*text) {
        text = NO_TEXT;
    }

    return checkAndConvert(renderer,SurfacePtr(TTF_RenderUTF8_Shaded(&font, text, fg,bg)),text);
}

void applyTexture(int x, int y, SDL_Texture &texture, SDL_Renderer & renderer, const SDL_Rect *clip) {
	if (clip) {
		const SDL_Rect dstRect = { x, y, clip->w, clip->h };
		SDL_RenderCopy(&renderer, &texture, clip, &dstRect);
	} else {
		//Get width/height from the texture.
		const SDL_Rect dstRect = rectFromTexture(x, y, texture);
		SDL_RenderCopy(&renderer, &texture, clip, &dstRect);
	}
}

SurfacePtr createSurface(int width, int height) {
    return SurfacePtr(SDL_CreateRGBSurface(0, width, height, SCREEN_BPP, RMASK, GMASK, BMASK, AMASK));
}

void drawTitleTexture(int screenWidth, SDL_Texture &texture, SDL_Renderer &renderer) {
    SDL_Rect dstRect = rectFromTexture(0,40-TITLE_FONT_RAISE, texture);
    dstRect.x = (screenWidth - dstRect.w) / 2;
    SDL_RenderCopy(&renderer, &texture, NULL, &dstRect);
}
