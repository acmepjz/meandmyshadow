PROJECT(meandmyshadow)
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)


Find_Package(SDL REQUIRED)
Find_Package(SDL_ttf REQUIRED)
Find_Package(SDL_image REQUIRED)
Find_Package(SDL_mixer REQUIRED)

INCLUDE_DIRECTORIES(${SDL_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${SDLIMAGE_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${SDLMIXER_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${SDLTTF_INCLUDE_DIR})

if ( NOT SDL_FOUND )
    message(FATAL_ERROR "SDL not found !")
endif ( NOT SDL_FOUND )

if ( NOT SDLTTF_FOUND )
    message(FATAL_ERROR "SDL_ttf not found !")
endif ( NOT SDLTTF_FOUND )

if ( NOT SDLMIXER_FOUND )
    message(FATAL_ERROR "SDL_mixer not found !")
endif ( NOT SDLMIXER_FOUND )

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR})
set(SRC_DIR ${PROJECT_SOURCE_DIR}/src)

include_directories(
	${SDL_INCLUDE_DIR}
	${SDLTTF_INCLUDE_DIR}
	${SDLMIXER_INCLUDE_DIR}
	${SDLIMAGE_INCLUDE_DIR}
)

SET(CMAKE_CXX_FLAGS "-Wall -g")

# Déclaration de l'exécutable
add_executable(
    meandmyshadow
    ${SRC_DIR}/LevelEditor.h 
    ${SRC_DIR}/Levels.h 
    ${SRC_DIR}/Shadow.h 
    ${SRC_DIR}/Globals.h 
    ${SRC_DIR}/Functions.h 
    ${SRC_DIR}/GameObjects.h 
    ${SRC_DIR}/Block.h 
    ${SRC_DIR}/Game.h 
    ${SRC_DIR}/Player.h 
    ${SRC_DIR}/StartObjects.h 
    ${SRC_DIR}/Title_Menu.h 
    ${SRC_DIR}/Objects.h 
    ${SRC_DIR}/Timer.h 
    ${SRC_DIR}/LevelSelect.h
    ${SRC_DIR}/LevelEditor.cpp 
    ${SRC_DIR}/Levels.cpp 
    ${SRC_DIR}/Shadow.cpp 
    ${SRC_DIR}/Globals.cpp 
    ${SRC_DIR}/Functions.cpp 
    ${SRC_DIR}/GameObjects.cpp 
    ${SRC_DIR}/Block.cpp 
    ${SRC_DIR}/Game.cpp 
    ${SRC_DIR}/Main.cpp 
    ${SRC_DIR}/Player.cpp 
    ${SRC_DIR}/StartObjects.cpp 
    ${SRC_DIR}/Title_Menu.cpp 
    ${SRC_DIR}/Objects.cpp 
    ${SRC_DIR}/Timer.cpp 
    ${SRC_DIR}/LevelSelect.cpp
)

target_link_libraries(
    meandmyshadow
    ${SDL_LIBRARY}
	${SDLTTF_LIBRARY}
	${SDLIMAGE_LIBRARY}
	${SDLMIXER_LIBRARY}
)
