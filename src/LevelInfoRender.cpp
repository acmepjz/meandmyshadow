#include "Functions.h"

#include "Globals.h"
#include "LevelInfoRender.h"
#include "WordWrapper.h"

LevelInfoRender::LevelInfoRender(ImageManager &imageManager, SDL_Renderer &renderer, const std::string& dataPath, SDL_Color textColor)
	: cachedWidth(-1)
{
    playButton=imageManager.loadTexture(dataPath+"gfx/playbutton.png",renderer);
    timeIcon=imageManager.loadTexture(dataPath+"gfx/time.png",renderer);
    recordingsIcon=imageManager.loadTexture(dataPath+"gfx/recordings.png",renderer);
	objThemes.getBlock(TYPE_COLLECTABLE)->createInstance(&collectable);
    //Skip doing this here as it will be called LevelPlaySelect::refresh which is called by it's constructor anyhow.
    //resetText(renderer, textColor);
}

void LevelInfoRender::resetText(SDL_Renderer &renderer, SDL_Color textColor) {
    auto tex = [&](const char* text){
        return textureFromText(renderer,*fontText,text,textColor);
    };

	levelDescription = nullptr;
	cachedDescription = _("Choose a level");
	cachedWidth = -1;

    timeText=tex(_("Time:"));
    recordingsText=tex(_("Recordings:"));
    collectablesText=tex(_("Collectibles:"));
    levelTime=tex("- / -");
    levelRecs=tex("- / -");
}

void LevelInfoRender::update(SDL_Renderer &renderer, SDL_Color textColor,
                             const std::string &description, const std::string& time, const std::string& recordings) {
    auto tex = [&](const std::string& text){
        return textureFromText(renderer,*fontText,text.c_str(),textColor);
    };

	levelDescription = nullptr;
	cachedDescription = description;
	cachedWidth = -1;

	levelTime=tex(time);
    levelRecs=tex(recordings);
}

void LevelInfoRender::render(SDL_Renderer &renderer, bool arcade) {

    //Avoid crashing if this is somehow not initialized.
    if(!timeText) {
        return;
    }
    int w=0,h=0;
    SDL_GetRendererOutputSize(&renderer,&w,&h);

	if (!cachedDescription.empty()) {
		if (!levelDescription || cachedWidth != w) {
			cachedWidth = w;

			WordWrapper wrapper;

			wrapper.font = fontText;
			wrapper.maxWidth = w - 490;
			wrapper.wordWrap = true;
			wrapper.hyphen = "-";

			std::vector<std::string> lines;
			wrapper.addString(lines, cachedDescription);

			levelDescription = textureFromMultilineText(renderer, *fontText, lines, objThemes.getTextColor(false));
		}

		applyTexture(100, h - 130 + (50 - textureHeight(*levelDescription)) / 2, levelDescription, renderer);
	}

    //Draw time the icon.
    applyTexture(w-405,h-130+6,timeIcon,renderer);

    //Now draw the text (title).
    applyTexture(w-380,h-130+6,timeText,renderer);

    //Now draw the second text (value).
    applyTexture(w-textureWidth(*levelTime)-80,h-130+6,levelTime,renderer);

    //Draw the icon.
	if (arcade) {
		collectable.draw(renderer, w - 405 - 16, h - 98 + 6 - 16);
	} else {
		applyTexture(w - 405, h - 98 + 6, recordingsIcon, renderer);
	}

    //Now draw the text (title).
	applyTexture(w - 380, h - 98 + 6, arcade ? collectablesText : recordingsText, renderer);

	//Now draw the second text (value).
    applyTexture(w-textureWidth(*levelRecs)-80,h-98+6,levelRecs,renderer);
}
