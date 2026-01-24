# Guys, this file is mostly for me, as I am on Linux 
# (The objectively superior C development environment)
# but feel free to use it with wsl if you want a better
# experience.
#
# 	~ Shraga
#

SOURCE_SIM = $(wildcard sim/*.h sim/*.c)
TARGET_SIM = test/simulator
FLAGS = -Wall -Wextra
DEBUG_FLAGS = -Wall -Wextra -g -O0 -DDEBUG

all: $(TARGET_SIM)

$(TARGET_SIM): clean $(SOURCES_SIM)
	@echo Compiling files: $(SOURCE_SIM)
	@gcc $(FLAGS) -o $(TARGET_SIM) $(SOURCE_SIM)


clean: 
	@rm -f $(TARGET_SIM)

clean-test:
	@rm -f $(TARGET_SIM)
	@rm -f test/*trace.txt test/stats* test/*out* test/*ram*

	
.PHONY: clean clean-test