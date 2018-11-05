Project (meandmyshadow)
CMake_Minimum_Required (VERSION 3.1)
Set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

#Path options
Set (BINDIR "bin" CACHE STRING "Where to install binaries")
Set (DATAROOTDIR "${CMAKE_INSTALL_PREFIX}/share" CACHE STRING "Sets the root of data directories to a non-default location")
Set (ICONDIR "${DATAROOTDIR}/icons" CACHE STRING "Sets the icon directory for desktop entry to a non-default location.")
Set (DESKTOPDIR "${DATAROOTDIR}/applications" CACHE STRING "Sets the desktop file directory for desktop entry to a non-default location.")

#Options
Option (DEBUG_MODE "Compile the game with debug mode enabled" OFF)
Option (DISABLED_DEBUG_STUFF "Enable this you'll see a lot of annoying script debug messages which will lag the game." OFF)

#Find the required libraries.
Find_Package (SDL2 REQUIRED)
Find_Package (SDL2_image REQUIRED)
Find_Package (Freetype REQUIRED)
Find_Package (SDL2_mixer REQUIRED)
Find_Package (CURL REQUIRED)
Find_Package (LibArchive REQUIRED)
Find_Package (Lua 5.3 REQUIRED)

if (NOT SDL2_FOUND)
	message (FATAL_ERROR "SDL2 library could not be found!")
endif (NOT SDL2_FOUND)

if (NOT SDL2_IMAGE_FOUND)
	message (FATAL_ERROR "SDL2_image library could not be found!")
endif (NOT SDL2_IMAGE_FOUND)

if (NOT FREETYPE_FOUND)
	message (FATAL_ERROR "Freetype library could not be found!")
endif (NOT FREETYPE_FOUND)

if (NOT SDL2_MIXER_FOUND)
	message (FATAL_ERROR "SDL2_mixer library could not be found!")
endif (NOT SDL2_MIXER_FOUND)

if (NOT CURL_FOUND)
	message(FATAL_ERROR "CURL library could not be found!")
endif (NOT CURL_FOUND)

if (NOT LibArchive_FOUND)
	message (FATAL_ERROR "LibArchive library could not be found!")
endif (NOT LibArchive_FOUND)

if (NOT LUA_FOUND)
	message (FATAL_ERROR "Lua library could not be found!")
endif (NOT LUA_FOUND)

if (LUA_VERSION_STRING VERSION_LESS "5.3")
	message (FATAL_ERROR "Lua version too old ${LUA_VERSION_STRING}, expected at least 5.3!")
endif ()

# check version from Globals.h
file(READ "${PROJECT_SOURCE_DIR}/src/Globals.h" GLOBALS_H)
string(REGEX MATCH "version[ ]*=[ ]*\"[^\"]*\"" MNMS_VERSION_STR ${GLOBALS_H})
string(REGEX REPLACE "^[^\"]*\"([^\"]*)\".*$" "\\1" MNMS_VERSION_STR ${MNMS_VERSION_STR})
message(STATUS "The version read from Globals.h is: ${MNMS_VERSION_STR}")

string(REGEX REPLACE "^V([0-9.]+).*$" "\\1" MNMS_VERSION_NUM ${MNMS_VERSION_STR})
set(MNMS_VERSION_NUM "${MNMS_VERSION_NUM}.0.0.0.0")
string(REGEX REPLACE "^([0-9]+)[.]([0-9]+)[.]([0-9]+)[.]([0-9]+).*$" "\\1,\\2,\\3,\\4" MNMS_VERSION_NUM ${MNMS_VERSION_NUM})
message(STATUS "which is: ${MNMS_VERSION_NUM}")

# check version from git
find_package(Git)
if(GIT_FOUND)
	exec_program(${GIT_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR} ARGS "describe"
		OUTPUT_VARIABLE MNMS_GIT_VERSION RETURN_VALUE GIT_RETURN_VALUE)
	if(GIT_RETURN_VALUE STREQUAL "0")
		set(MNMS_VERSION_STR "${MNMS_VERSION_STR} (${MNMS_GIT_VERSION})")
		message(STATUS "The version read from git is: ${MNMS_GIT_VERSION}")
	else()
		# possibly there are no any tags
		exec_program(${GIT_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR} ARGS "rev-parse --short HEAD"
			OUTPUT_VARIABLE MNMS_GIT_VERSION RETURN_VALUE GIT_RETURN_VALUE)
		if(GIT_RETURN_VALUE STREQUAL "0")
			set(MNMS_VERSION_STR "${MNMS_VERSION_STR} (git ${MNMS_GIT_VERSION})")
			message(STATUS "The version read from git is: ${MNMS_GIT_VERSION}")
		endif()
	endif()
endif()

# show version information on Windows
Set(WIN32_RESOURCES )
if(WIN32)
	Configure_File (
		"${PROJECT_SOURCE_DIR}/icons/windows-icon/res.rc.in"
		"${PROJECT_BINARY_DIR}/res.rc"
	)
	Set(WIN32_RESOURCES ${PROJECT_BINARY_DIR}/res.rc)
	Include_Directories(${PROJECT_SOURCE_DIR}/icons/windows-icon/)
	SOURCE_GROUP("Source Files\\Resources" FILES ${WIN32_RESOURCES})
endif()

#Parse the configure file.
Configure_File (
	"${PROJECT_SOURCE_DIR}/src/config.h.in"
	"${PROJECT_BINARY_DIR}/config.h"
)

#Add some missing libraries to Windows.
if(WIN32)
	include_directories(${PROJECT_SOURCE_DIR}/src/libs/dirent)
endif(WIN32)

#Disable some annoying warnings.
if(MSVC)
	# warning C4996: '***': This function or variable may be unsafe
	add_definitions(/wd4996)

	# force the source code encoding to UTF-8 (which is available since VC2015)
	if (NOT (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 19.0))
		add_definitions(-utf-8)
	endif()
else()
	# Assume it's gcc or clang
	# warning: '***' overrides a member function but is not marked 'override'
	add_definitions(-Wno-inconsistent-missing-override)
endif()

#Define some debug stuff.
if(DEBUG_MODE)
	add_definitions(-DDEBUG)
	add_definitions(-D_DEBUG)
endif(DEBUG_MODE)
if(DISABLED_DEBUG_STUFF)
	add_definitions(-DDISABLED_DEBUG_STUFF)
endif(DISABLED_DEBUG_STUFF)

#Add the include directories of the (found) libraries.
Include_Directories(
	${PROJECT_BINARY_DIR}
	${SDL2_INCLUDE_DIR}
	${SDL2_IMAGE_INCLUDE_DIR}
	${FREETYPE_INCLUDE_DIRS}
	${SDL2_MIXER_INCLUDE_DIR}
	${CURL_INCLUDE_DIR}
	${LibArchive_INCLUDE_DIR}
	${LUA_INCLUDE_DIR}
	${PROJECT_SOURCE_DIR}/src/libs
	${PROJECT_SOURCE_DIR}/src/libs/SDL2_ttf
)

#Set the output path and the source path.
Set (EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR})
Set (SRC_DIR ${PROJECT_SOURCE_DIR}/src)

#List the source files.
File (GLOB SOURCES ${SRC_DIR}/*.cpp)
File (GLOB TINYFORMAT ${SRC_DIR}/libs/tinyformat/*.cpp)
File (GLOB TINYGETTEXT ${SRC_DIR}/libs/tinygettext/*.cpp)
File (GLOB FINDLOCALE ${SRC_DIR}/libs/findlocale/*.cpp)
File (GLOB SDL2TTF ${SRC_DIR}/libs/SDL2_ttf/*.c)

#Always use SDL_iconv in tinygettext
add_definitions(-DHAVE_SDL)

SOURCE_GROUP("Source Files\\tinyformat" FILES ${TINYFORMAT})
SOURCE_GROUP("Source Files\\tinygettext" FILES ${TINYGETTEXT})
SOURCE_GROUP("Source Files\\findlocale" FILES ${FINDLOCALE})
SOURCE_GROUP("Source Files\\SDL2_ttf" FILES ${SDL2TTF})

Add_Executable (meandmyshadow ${SOURCES} ${TINYFORMAT} ${TINYGETTEXT} ${FINDLOCALE} ${WIN32_RESOURCES} ${SDL2TTF})

set_property(TARGET meandmyshadow PROPERTY CXX_STANDARD 11)

Target_Link_Libraries (
	meandmyshadow
	${SDL2_LIBRARY}
	${SDL2_IMAGE_LIBRARY}
	${FREETYPE_LIBRARIES}
	${SDL2_MIXER_LIBRARY}
	${SDL2MAIN_LIBRARY}
	${CURL_LIBRARY}
	${LibArchive_LIBRARY}
	${LUA_LIBRARIES}
)

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
