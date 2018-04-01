Project (meandmyshadow)
CMake_Minimum_Required (VERSION 2.6)
Set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

#Option if the game should be compiled with hardware acceleration.
Option (HARDWARE_ACCELERATION "Use openGL as rendering backend" ON)
Option (DEBUG_MODE "Compile the game with debug mode enabled" OFF)

#Use openGL only when compiling with HARDWARE_ACCELERATION.
#Otherwise we try to find XLib.
if (HARDWARE_ACCELERATION)
	Find_Package (OpenGL REQUIRED)
	if ("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")
		Find_Package (X11 REQUIRED)
	endif()
else (HARDWARE_ACCELERATION)
	Include (FindX11)
endif (HARDWARE_ACCELERATION)

#Find the required libraries.
Find_Package (SDL REQUIRED)
Find_Package (SDL_gfx REQUIRED)
Find_Package (SDL_image REQUIRED)
Find_Package (SDL_ttf REQUIRED)
Find_Package (SDL_mixer REQUIRED)
Find_Package (CURL REQUIRED)
Find_Package (LibArchive REQUIRED)

# maybe Lua >= 5.3 is also OK
Find_Package (Lua 5.2 EXACT REQUIRED)

if (HARDWARE_ACCELERATION AND NOT OPENGL_FOUND)
	message (FATAL_ERROR "OpenGL library could not be found!")
elseif (NOT HARDWARE_ACCELERATION AND NOT X11_FOUND)
	message (FATAL_ERROR "X11 library could not be found!")
endif (HARDWARE_ACCELERATION AND NOT OPENGL_FOUND)

#Parse the configure file.
Configure_File (
	"${PROJECT_SOURCE_DIR}/src/config.h.in"
	"${PROJECT_BINARY_DIR}/config.h"
)

#Add the include directories of the (found) libraries.
Include_Directories(
	${PROJECT_BINARY_DIR}
	${X11_X11_INCLUDE_PATH}
	${OPENGL_gl_INCLUDE_DIR}
	${SDL_INCLUDE_DIR}
	${SDLGFX_INCLUDE_DIR}
	${SDLIMAGE_INCLUDE_DIR}
	${SDLTTF_INCLUDE_DIR}
	${SDLMIXER_INCLUDE_DIR}
	${CURL_INCLUDE_DIR}
	${LibArchive_INCLUDE_DIR}
	${LUA_INCLUDE_DIR}
)

#Set the output path and the source path.
Set (EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR})
Set (SRC_DIR ${PROJECT_SOURCE_DIR}/src)

#List the source files.
File (GLOB SOURCES ${SRC_DIR}/*.cpp)
File (GLOB TINYGETTEXT ${SRC_DIR}/libs/tinygettext/*.cpp)
File (GLOB FINDLOCALE ${SRC_DIR}/libs/findlocale/*.cpp)

Set(OSX_SOURCES )

#if(APPLE)
#	Set(OSX_SOURCES
#		${SRC_DIR}/mac/meandmyshadow/osx_helpers.mm
#		${SRC_DIR}/mac/meandmyshadow/SDLMain.m
#		)
#endif()

Add_Executable (meandmyshadow ${SOURCES} ${TINYGETTEXT} ${FINDLOCALE} ${OSX_SOURCES})

Target_Link_Libraries (
	meandmyshadow
	${OPENGL_gl_LIBRARY}
	${X11_X11_LIB}
	${SDL_LIBRARY}
	${SDLGFX_LIBRARY}
	${SDLIMAGE_LIBRARY}
	${SDLTTF_LIBRARY}
	${SDLMIXER_LIBRARY}
	${CURL_LIBRARY}
	${LibArchive_LIBRARY}
	${LUA_LIBRARIES}
)

#Path options
Set (BINDIR "bin" CACHE STRING "Where to install binaries")
Set (DATAROOTDIR "${CMAKE_INSTALL_PREFIX}/share" CACHE STRING "Sets the root of data directories to a non-default location")
Set (ICONDIR "${DATAROOTDIR}/icons" CACHE STRING "Sets the icon directory for desktop entry to a non-default location.")
Set (DESKTOPDIR "${DATAROOTDIR}/applications" CACHE STRING "Sets the desktop file directory for desktop entry to a non-default location.")

#Install locations
Install (DIRECTORY ${PROJECT_SOURCE_DIR}/data DESTINATION ${DATAROOTDIR}/meandmyshadow/)
Install (FILES AUTHORS DESTINATION ${DATAROOTDIR}/meandmyshadow/)
Install (TARGETS meandmyshadow RUNTIME DESTINATION ${BINDIR})

if ("${CMAKE_SYSTEM_NAME}" MATCHES "Linux") 
	Install (FILES meandmyshadow.desktop DESTINATION ${DESKTOPDIR})
	Install (FILES icons/16x16/meandmyshadow.png DESTINATION ${ICONDIR}/hicolor/16x16/apps/)
	Install (FILES icons/32x32/meandmyshadow.png DESTINATION ${ICONDIR}/hicolor/32x32/apps/)
	Install (FILES icons/48x48/meandmyshadow.png DESTINATION ${ICONDIR}/hicolor/48x48/apps/)
	Install (FILES icons/64x64/meandmyshadow.png DESTINATION ${ICONDIR}/hicolor/64x64/apps/)
endif ("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")
