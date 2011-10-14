PROJECT(meandmyshadow)
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)


Find_Package(SDL REQUIRED)
Find_Package(SDL_ttf REQUIRED)
Find_Package(SDL_image REQUIRED)
Find_Package(SDL_mixer REQUIRED)
Find_Package(CURL REQUIRED)
Find_Package(LibArchive REQUIRED)

INCLUDE_DIRECTORIES(${SDL_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${SDLIMAGE_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${SDLMIXER_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${SDLTTF_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${CURL_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${LibArchive_INCLUDE_DIR})

if ( NOT SDL_FOUND )
    message(FATAL_ERROR "SDL not found !")
endif ( NOT SDL_FOUND )

if ( NOT SDLTTF_FOUND )
    message(FATAL_ERROR "SDL_ttf not found !")
endif ( NOT SDLTTF_FOUND )

if ( NOT SDLMIXER_FOUND )
    message(FATAL_ERROR "SDL_mixer not found !")
endif ( NOT SDLMIXER_FOUND )

if ( NOT CURL_FOUND )
    message(FATAL_ERROR "CURL not found !")
endif ( NOT CURL_FOUND )

if ( NOT LibArchive_FOUND )
    message(FATAL_ERROR "LIBARCHIVE not found !")
endif ( NOT LibArchive_FOUND )


configure_file (
  "${PROJECT_SOURCE_DIR}/src/config.h.in"
  "${PROJECT_BINARY_DIR}/config.h"
  )

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR})
set(SRC_DIR ${PROJECT_SOURCE_DIR}/src)

include_directories(
	${PROJECT_BINARY_DIR}
	${SDL_INCLUDE_DIR}
	${SDLTTF_INCLUDE_DIR}
	${SDLMIXER_INCLUDE_DIR}
	${SDLIMAGE_INCLUDE_DIR}
)

SET(CMAKE_CXX_FLAGS "-Wall -g")

# Déclaration de l'exécutable
file(GLOB Sources ${SRC_DIR}/*.cpp)
add_executable(meandmyshadow ${Sources})

target_link_libraries(
    meandmyshadow
    ${SDL_LIBRARY}
	${SDLTTF_LIBRARY}
	${SDLIMAGE_LIBRARY}
	${SDLMIXER_LIBRARY}
    ${CURL_LIBRARY}
    ${LibArchive_LIBRARY}
)

install(DIRECTORY ${PROJECT_BINARY_DIR}/data DESTINATION share/meandmyshadow/)
install(TARGETS meandmyshadow RUNTIME DESTINATION bin)
if("${CMAKE_SYSTEM_NAME}" MATCHES "Linux") 
	install(FILES meandmyshadow.desktop DESTINATION share/applications/)
	install(FILES meandmyshadow.xpm DESTINATION share/pixmaps/)
endif("${CMAKE_SYSTEM_NAME}" MATCHES "Linux") 
 