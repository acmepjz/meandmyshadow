#ifndef CACHEDTEXTURE_H
#define CACHEDTEXTURE_H

#include "Render.h"

//A class to cache textures that display stuff that is updated regularly
template <typename T>
class CachedTexture
{
private:
    T id;
    TexturePtr tex;
public:
    CachedTexture() {}
    TexturePtr& getTexture(){
        return tex;
    }

    SDL_Texture* get(){
        return tex.get();
    }

	const T& getId() const {
		return id;
	}

    // Check if the texture exists.
    bool exists() {
        return tex.get() != nullptr;
    }

    // Check if the contained texture has to be updated.
    bool needsUpdate(const T& id){
        if (this->id!=id || !tex) {
            return true;
        } else {
            return false;
        }
    }

    //Update the contained texture with a new id and texture.
    void update(T id, TexturePtr&& texture) {
        this->id = id;
        this->tex = std::move(texture);
    }
};

#endif // CACHEDTEXTURE_H
