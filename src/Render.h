#ifndef RENDER_H
#define RENDER_H

#include <memory>
#include <vector>
#include <string>

#include "ImageManager.h"

// The forward declaration of TTF_Font is clunky like this
// as it's forward declared like this in SDL_ttf_fontfallback.h
struct _TTF_Font;
typedef struct _TTF_Font TTF_Font;

// Deleter functor for textures.
struct TextureDeleter {
    void operator()(SDL_Texture* t) {
        SDL_DestroyTexture(t);
    }
};

struct SurfaceDeleter {
    void operator()(SDL_Surface* s) {
        SDL_FreeSurface(s);
    }
};

using TexturePtr = std::unique_ptr<SDL_Texture, TextureDeleter>;
using SurfacePtr = std::unique_ptr<SDL_Surface, SurfaceDeleter>;

//Create a texture from text.
//renderer: The SDL_Renderer instance.
//font: The wanted font.
//text: The text that should be rendered to the texture.
//color: The color the text should be rendered with.
TexturePtr textureFromText(SDL_Renderer& renderer, TTF_Font& font,const char* text,SDL_Color color);
inline SharedTexture textureFromTextShared(SDL_Renderer& renderer,TTF_Font& font,const char* text,SDL_Color color) {
    return SharedTexture(textureFromText(renderer, font, text, color));
}

TexturePtr textureFromMultilineText(SDL_Renderer& renderer, TTF_Font& font, const std::vector<std::string>& text, SDL_Color color, int gravity = 0);
inline SharedTexture textureFromMultilineTextShared(SDL_Renderer& renderer, TTF_Font& font, const std::vector<std::string>& text, SDL_Color color, int gravity = 0) {
	return SharedTexture(textureFromMultilineText(renderer, font, text, color));
}

//Create a texture from text, choosing appropriate font according to width.
TexturePtr titleTextureFromText(SDL_Renderer& renderer, const char* text, SDL_Color color, int width);

TexturePtr textureFromTextShaded(SDL_Renderer& renderer, TTF_Font& font, const char* text, SDL_Color fg,SDL_Color bg);

inline SharedTexture textureFromSurface(SDL_Renderer& renderer, SurfacePtr surface) {
    return SharedTexture(SDL_CreateTextureFromSurface(&renderer,surface.get()), SDL_DestroyTexture);
}

inline TexturePtr textureUniqueFromSurface(SDL_Renderer& renderer, SurfacePtr surface) {
    return TexturePtr(SDL_CreateTextureFromSurface(&renderer,surface.get()));
}

SurfacePtr createSurface(int width, int height);

//Convenience function to render a texture at default size to a renderer.
//x: The x location to draw the source on the desination.
//y: The y location to draw the source on the desination.
//renderer: The SDL_Renderer to render on.
//texture: The SDL_Texture to render.
//clip: Rectangle which part of the source should be drawn. (NULL for the whole source.
void applyTexture(int x,int y,SDL_Texture& texture, SDL_Renderer& renderer, const SDL_Rect* clip = NULL);
inline void applyTexture(int x,int y,TexturePtr& texture, SDL_Renderer& renderer, const SDL_Rect* clip = NULL) {
    applyTexture(x, y, *texture, renderer, clip);
}
inline void applyTexture(int x,int y,SharedTexture& texture, SDL_Renderer& renderer, const SDL_Rect* clip = NULL) {
    applyTexture(x, y, *texture, renderer, clip);
}

//Draw the texture in the top middle, used for titles.
void drawTitleTexture(int screenWidth, SDL_Texture& texture, SDL_Renderer& renderer);

//Convenience function to create a SDL_Rect with w/h equal to that of the provided texture.
inline SDL_Rect rectFromTexture(int x, int y, SDL_Texture& texture) {
    SDL_Rect r{x, y, 0, 0};
    SDL_QueryTexture(&texture, NULL, NULL, &r.w, &r.h);
    return r;
}

inline SDL_Rect rectFromTexture(SDL_Texture& texture) {
    return rectFromTexture(0, 0, texture);
}

template<typename T>
inline SDL_Rect rectFromTexture(int x, int y, T& texture) {
    SDL_Rect r{x, y, 0, 0};
    SDL_QueryTexture(texture.get(), NULL, NULL, &r.w, &r.h);
    return r;
}

template<typename T>
inline SDL_Rect rectFromTexture(T& texture) {
    return rectFromTexture(0, 0, texture);
}

inline int textureWidth(SDL_Texture& texture) {
    return rectFromTexture(texture).w;
}

inline int textureHeight(SDL_Texture& texture) {
    return rectFromTexture(texture).h;
}

template<typename T>
inline int textureHeight(T& texture) {
    if (texture) {
        return rectFromTexture(*texture.get()).h;
    } else {
        return 0;
    }
}

template<typename T>
inline int textureWidth(T& texture) {
    if (texture) {
        return rectFromTexture(*texture.get()).w;
    } else {
        return 0;
    }
}

inline void dimScreen(SDL_Renderer& renderer, Uint8 alpha=127) {
    SDL_SetRenderDrawColor(&renderer, 0,0,0,alpha);
    SDL_RenderFillRect(&renderer, NULL);
}

#endif // RENDER_H
