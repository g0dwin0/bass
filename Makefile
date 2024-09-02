CPPFLAGS = -g -Ilib/imgui -Ilib/imgui/backends -Ilib/ -Ilib/cli11  -Ilib/tinyfiledialogs -Wall -Wextra -Werror -Iinclude/core -Iinclude/frontend -Iinclude/ -std=c++20 -lpthread -lSDL2 -lGL -Wno-deprecated-enum-enum-conversion
OBJ_DIR = build/obj
OBJ_IMGUI_PATH = $(OBJ_DIR)/imgui
OBJS_IMGUI = $(OBJ_IMGUI_PATH)/imgui_demo.o $(OBJ_IMGUI_PATH)/imgui_draw.o $(OBJ_IMGUI_PATH)/imgui_impl_sdlrenderer2.o $(OBJ_IMGUI_PATH)/imgui_impl_sdl2.o $(OBJ_IMGUI_PATH)/imgui_tables.o $(OBJ_IMGUI_PATH)/imgui_widgets.o $(OBJ_IMGUI_PATH)/imgui.o
OBJS = $(OBJ_DIR)/main.o $(OBJ_DIR)/frontend.o $(OBJ_DIR)/file_dialog.o $(OBJ_DIR)/pak.o $(OBJ_DIR)/bass.o $(OBJ_DIR)/cpu.o $(OBJ_DIR)/arm.o $(OBJ_DIR)/bus.o $(OBJ_DIR)/ppu.o $(OBJ_DIR)/registers.o 
CC=g++
# CPPFLAGS += -fsanitize=undefined,address -D_GLIBCXX_DEBUG
# CPPFLAGS += -O2


PROJ_NAME = bass
all: final

final: $(OBJS) 
	@echo "linking...."
	$(CC) $(OBJS) $(OBJS_IMGUI) -o build/bin/$(PROJ_NAME) $(CPPFLAGS)
	chmod +x build/bin/$(PROJ_NAME)

$(OBJ_DIR)/main.o: src/main.cpp include/common.hpp
	$(CC) $(CPPFLAGS) -c src/main.cpp -o $(OBJ_DIR)/main.o

$(OBJ_DIR)/bass.o: src/core/bass.cpp include/core/bass.hpp
	$(CC) $(CPPFLAGS) -c src/core/bass.cpp -o $(OBJ_DIR)/bass.o

$(OBJ_DIR)/cpu.o: src/core/cpu.cpp include/core/cpu.hpp include/core/instructions/instruction.hpp src/core/shifter.cpp src/core/decode/arm.cpp src/core/decode/thumb.cpp 
	$(CC) $(CPPFLAGS) -c src/core/cpu.cpp -o $(OBJ_DIR)/cpu.o

$(OBJ_DIR)/registers.o: include/core/registers.hpp src/core/registers.cpp
	$(CC) $(CPPFLAGS) -c src/core/registers.cpp -o $(OBJ_DIR)/registers.o

$(OBJ_DIR)/arm.o: src/core/instructions/arm.cpp  include/core/instructions/arm.hpp src/core/registers.cpp 
	$(CC) $(CPPFLAGS) -c src/core/instructions/arm.cpp -o $(OBJ_DIR)/arm.o

$(OBJ_DIR)/bus.o: src/core/bus.cpp include/core/bus.hpp
	$(CC) $(CPPFLAGS) -c src/core/bus.cpp -o $(OBJ_DIR)/bus.o

$(OBJ_DIR)/pak.o: src/core/pak.cpp include/core/pak.hpp
	$(CC) $(CPPFLAGS) -c src/core/pak.cpp -o $(OBJ_DIR)/pak.o

$(OBJ_DIR)/ppu.o: src/core/ppu.cpp include/core/ppu.hpp
	$(CC) $(CPPFLAGS) -c src/core/ppu.cpp -o $(OBJ_DIR)/ppu.o

$(OBJ_DIR)/frontend.o: src/frontend/window.cpp include/frontend/window.h include/common.hpp
	$(CC) $(CPPFLAGS) -c src/frontend/window.cpp -o $(OBJ_DIR)/frontend.o

$(OBJ_DIR)/file_dialog.o: lib/tinyfiledialogs/tinyfiledialogs.c lib/tinyfiledialogs/tinyfiledialogs.h
	$(CC) $(CPPFLAGS) -c lib/tinyfiledialogs/tinyfiledialogs.c -o $(OBJ_DIR)/file_dialog.o -Wno-unused-variable



clean:
	rm $(OBJS)

$(PROJ_NAME):
	cat /dev/null > MyLog.txt
	./build/bin/$(PROJ_NAME) >> MyLog.txt