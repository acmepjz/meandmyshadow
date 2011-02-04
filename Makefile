CC=g++

all: meandmyshadow

meandmyshadow: 
	$(CC) -o meandmyshadow Levels.cpp Shadow.cpp Globals.cpp Functions.cpp GameObjects.cpp Block.cpp Game.cpp Main.cpp Player.cpp StartObjects.cpp Title_Menu.cpp Objects.cpp Timer.cpp LevelSelect.cpp -lSDL_image -lSDL_mixer -lSDL_ttf -lSDL

clean:
	rm -rf *o
