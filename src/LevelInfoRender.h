#ifndef LEVELINFORENDER_H
#define LEVELINFORENDER_H

#include "Render.h"
#include "ThemeManager.h"

class LevelPlaySelect;

class LevelInfoRender
{
    friend class LevelPlaySelect;
private:
    //Icons
    SharedTexture playButton;
    SharedTexture timeIcon;
    SharedTexture recordingsIcon;
	ThemeBlockInstance collectable;
	//Texture displaying the level description.
    TexturePtr levelDescription;
    //Texture displaying "Time" (or other language equivalent).
    TexturePtr timeText;
    //Texture displaying "Recordings" (or other language equivalent).
    TexturePtr recordingsText;
	//Texture displaying "Collectibles" (or other language equivalent).
	TexturePtr collectablesText;
	//Texture displaying the actual level time.
    TexturePtr levelTime;
    TexturePtr levelRecs;
public:
    LevelInfoRender(ImageManager& imageManager, SDL_Renderer& renderer,
                    const std::string &dataPath, TTF_Font& font, SDL_Color textColor);
    void resetText(SDL_Renderer& renderer, TTF_Font& font, SDL_Color textColor);
    void update(SDL_Renderer& renderer,TTF_Font& font, SDL_Color textColor,
                const std::string& description, const std::string& time, const std::string& recordings);
    void render(SDL_Renderer& renderer,bool arcade);
};

#endif // LEVELINFORENDER_H
