#include "Functions.h"

#include "LevelInfoRender.h"

LevelInfoRender::LevelInfoRender(ImageManager &imageManager, SDL_Renderer &renderer, const std::string& dataPath, TTF_Font &font, SDL_Color textColor)
{
    playButton=imageManager.loadTexture(dataPath+"gfx/playbutton.png",renderer);
    timeIcon=imageManager.loadTexture(dataPath+"gfx/time.png",renderer);
    recordingsIcon=imageManager.loadTexture(dataPath+"gfx/recordings.png",renderer);
    //Skip doing this here as it will be called LevelPlaySelect::refresh which is called by it's constructor anyhow.
    //resetText(renderer, font, textColor);
}

void LevelInfoRender::resetText(SDL_Renderer &renderer, TTF_Font &font, SDL_Color textColor) {
    auto tex = [&](const char* text){
        return textureFromText(renderer,font,text,textColor);
    };

    levelDescription=tex(_("Choose a level"));
    timeText=tex(_("Time:"));
    recordingsText=tex(_("Recordings:"));
    levelTime=tex("- / -");
    levelRecs=tex("- / -");
}

void LevelInfoRender::update(SDL_Renderer &renderer, TTF_Font &font, SDL_Color textColor,
                             const std::string &description, const std::string& time, const std::string& recordings) {
    auto tex = [&](const std::string& text){
        return textureFromText(renderer,font,text.c_str(),textColor);
    };
    if(description.empty()) {
        levelDescription=nullptr;
    } else {
        levelDescription=tex(description);
    }
    levelTime=tex(time);
    levelRecs=tex(recordings);
}

void LevelInfoRender::render(SDL_Renderer &renderer) {

    //Avoid crashing if this is somehow not initialized.
    if(!timeText) {
        return;
    }
    int w=0,h=0;
    SDL_GetRendererOutputSize(&renderer,&w,&h);
    if(levelDescription) {
        applyTexture(100,h-130+(50-textureHeight(*levelDescription))/2,levelDescription, renderer);
    }

    //Draw time the icon.
    applyTexture(w-405,h-130+3,timeIcon,renderer);

    //Now draw the text (title).
    applyTexture(w-380,h-130+3,timeText,renderer);

    //Now draw the second text (value).
    applyTexture(w-textureWidth(*levelTime)-80,h-130+3,levelTime,renderer);

    //Draw the icon.
    applyTexture(w-405,h-98+(6)/2,recordingsIcon,renderer);

    //Now draw the text (title).
    applyTexture(w-380,h-98+(32-textureHeight(*recordingsText))/2,
                                                    recordingsText,renderer);
    //Now draw the second text (value).
    applyTexture(w-textureWidth(*levelRecs)-80,h-98+(30-textureHeight(*levelRecs))/2,levelRecs,renderer);
}
