CC=g++

all: meandmyshadow

meandmyshadow: 
	$(CC) -o meandmyshadow src/Levels.cpp src/Shadow.cpp src/Globals.cpp src/Functions.cpp src/GameObjects.cpp src/Block.cpp src/Game.cpp src/Main.cpp src/Player.cpp src/StartObjects.cpp src/Title_Menu.cpp src/Objects.cpp src/Timer.cpp src/LevelSelect.cpp -lSDL_image -lSDL_mixer -lSDL_ttf -lSDL

clean:
	rm -rf *o
