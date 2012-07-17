PROJECT(meandmyshadow)
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

#Option if the game should be compiled with hardware acceleration.
option (HARDWARE_ACCELERATION "Use openGL as rendering backend" ON)
option (DEBUG_MODE "Compile the game with debug mode enabled" OFF)

#Find the required libraries.
Find_Package(SDL REQUIRED)
Find_Package(SDL_ttf REQUIRED)
Find_Package(SDL_image REQUIRED)
Find_Package(SDL_mixer REQUIRED)
Find_Package(SDL_gfx REQUIRED)
Find_Package(CURL REQUIRED)
Find_Package(LibArchive REQUIRED)
Find_Package(OpenSSL REQUIRED)

#Use openGL only when compiling with HARDWARE_ACCELERATION.
#Otherwise we try to find XLib.
if(HARDWARE_ACCELERATION)
	Find_Package(OpenGL REQUIRED)
else(HARDWARE_ACCELERATION)
	INCLUDE(FindX11)
endif(HARDWARE_ACCELERATION)

#Add the include dirs of the libraries.
INCLUDE_DIRECTORIES(${SDL_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${SDLIMAGE_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${SDLMIXER_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${SDLTTF_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${SDLGFX_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${CURL_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${LibArchive_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${OPENSSL_INCLUDE_DIR})
#Use openGL only when compiling with HARDWARE_ACCELERATION.
if(HARDWARE_ACCELERATION)
	INCLUDE_DIRECTORIES(${OPENGL_INCLUDE_DIR})
endif(HARDWARE_ACCELERATION)

if ( NOT SDL_FOUND )
    message(FATAL_ERROR "SDL not found !")
endif ( NOT SDL_FOUND )

if ( NOT SDLTTF_FOUND )
    message(FATAL_ERROR "SDL_ttf not found !")
endif ( NOT SDLTTF_FOUND )

if ( NOT SDLMIXER_FOUND )
    message(FATAL_ERROR "SDL_mixer not found !")
endif ( NOT SDLMIXER_FOUND )

if ( NOT SDLGFX_FOUND )
    message(FATAL_ERROR "SDL_gfx not found !")
endif ( NOT SDLGFX_FOUND )

if ( NOT CURL_FOUND )
    message(FATAL_ERROR "CURL not found !")
endif ( NOT CURL_FOUND )

if ( NOT LibArchive_FOUND )
    message(FATAL_ERROR "LIBARCHIVE not found !")
endif ( NOT LibArchive_FOUND )

if ( NOT OPENSSL_FOUND )
    message(FATAL_ERROR "OpenSSL not found !")
endif ( NOT OPENSSL_FOUND )

if ( HARDWARE_ACCELERATION AND NOT OPENGL_FOUND )
    message(FATAL_ERROR "OpenGL not found !")
endif ( HARDWARE_ACCELERATION AND NOT OPENGL_FOUND )

#Parse the configure file.
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
	${SDLGFX_INCLUDE_DIR}
	${OPENSSL_INCLUDE_DIR}
	${OPENGL_INCLUDE_DIR}
)

file(GLOB tinygettext ${SRC_DIR}/libs/tinygettext/*.cpp)

file(GLOB findlocale ${SRC_DIR}/libs/findlocale/*.c)

# Déclaration de l'exécutable
file(GLOB Sources ${SRC_DIR}/*.cpp)
add_executable(meandmyshadow ${Sources} ${tinygettext} ${findlocale})

target_link_libraries(
    meandmyshadow
    ${SDL_LIBRARY}
	${SDLTTF_LIBRARY}
	${SDLIMAGE_LIBRARY}
	${SDLMIXER_LIBRARY}
	${SDLGFX_LIBRARY}
    ${CURL_LIBRARY}
    ${LibArchive_LIBRARY}
    ${OPENSSL_LIBRARIES}
    ${CRYPTO_LIBRARY}
    ${OPENGL_LIBRARY}
    ${X11_LIBRARIES}
)

# Path options
set(BINDIR "bin" CACHE STRING "Where to install binaries")
set(DATAROOTDIR "${CMAKE_INSTALL_PREFIX}/share" CACHE STRING "Sets the root of data directories to a non-default location")
set(ICONDIR "${DATAROOTDIR}/icons" CACHE STRING "Sets the icon directory for desktop entry to a non-default location.")
set(DESKTOPDIR "${DATAROOTDIR}/applications" CACHE STRING "Sets the desktop file directory for desktop entry to a non-default location.")

# install locations
install(DIRECTORY ${PROJECT_SOURCE_DIR}/data DESTINATION ${DATAROOTDIR}/meandmyshadow/)
install(TARGETS meandmyshadow RUNTIME DESTINATION ${BINDIR})
if("${CMAKE_SYSTEM_NAME}" MATCHES "Linux") 
	install(FILES meandmyshadow.desktop DESTINATION ${DESKTOPDIR})
	install(FILES icons/16x16/meandmyshadow.png DESTINATION ${ICONDIR}/hicolor/16x16/apps/)
	install(FILES icons/32x32/meandmyshadow.png DESTINATION ${ICONDIR}/hicolor/32x32/apps/)
	install(FILES icons/48x48/meandmyshadow.png DESTINATION ${ICONDIR}/hicolor/48x48/apps/)
	install(FILES icons/64x64/meandmyshadow.png DESTINATION ${ICONDIR}/hicolor/64x64/apps/)
endif("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")
