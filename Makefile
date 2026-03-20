###############################
# 声明一些Makefile变量
###############################
CC = g++
LANG_STD = -std=c++17
COMPILER_FLAGS = -Wall -Wfatal-errors # -Wfatal-errors: 看到第一个错误就停止
INCLUDE_PATH = -I"./libs/"
SRC_FILES = ./src/*.cpp \
			./src/Game/*.cpp \
			./src/Logger/*.cpp \
			./src/ECS/*.cpp \
			./src/AssetStore/*.cpp
LINKER_FLAGS = -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_mixer -llua5.3
OBJ_NAME = gameengine

###############################
# 声明一些Makefile规则
###############################
build:
	$(CC) $(COMPILER_FLAGS) $(LAND_STD) $(INCLUDE_PATH) $(SRC_FILES) $(LINKER_FLAGS) -o $(OBJ_NAME);

run:
	./$(OBJ_NAME);

clean:
	rm $(OBJ_NAME);