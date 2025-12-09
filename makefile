# Guys, this file is mostly for me, as I am on Linux 
# (The objectively superior C development environment)
# but feel free to use it with wsl if you want a better
# experience.
#
# 	~ Shraga
#

SOURCE_SIM = $(wildcard sim/*.h sim/*.c)
TARGET_SIM = simulator
FLAGS = -Wall -Wextra
DEBUG_FLAGS = -Wall -Wextra -g -O0 -DDEBUG

simulator: clean $(SOURCES_SIM)
	@echo Compiling files: $(SOURCE_SIM)
	@gcc $(FLAGS) -o $(TARGET_SIM) $(SOURCE_SIM)

simulator-debug: clean $(SOURCES_SIM)
	@echo Compiling files with debug: $(SOURCE_SIM)
	@gcc $(DEBUG_FLAGS) -o $(TARGET_SIM) $(SOURCE_SIM)

run: 
	@./$(TARGET_SIM)
clean: 
	rm $(TARGET_SIM)