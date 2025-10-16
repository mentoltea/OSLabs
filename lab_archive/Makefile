CC = gcc
CFLAGS = -Wall -Wextra -ggdb
BUILD_FOLDER = build

.PHONY: clean archiver

all: archiver

archiver: $(BUILD_FOLDER)/cli.o $(BUILD_FOLDER)/loadsave.o $(BUILD_FOLDER)/archive.o
	$(CC) $(CFLAGS) -o archiver $(BUILD_FOLDER)/cli.o $(BUILD_FOLDER)/loadsave.o $(BUILD_FOLDER)/archive.o

$(BUILD_FOLDER):
	mkdir -p $(BUILD_FOLDER)

$(BUILD_FOLDER)/cli.o: cli.c $(BUILD_FOLDER)
	$(CC) $(CFLAGS) -c cli.c -o $(BUILD_FOLDER)/cli.o

$(BUILD_FOLDER)/archive.o: archive.c $(BUILD_FOLDER)
	$(CC) $(CFLAGS) -c archive.c -o $(BUILD_FOLDER)/archive.o

$(BUILD_FOLDER)/loadsave.o: loadsave.c $(BUILD_FOLDER)
	$(CC) $(CFLAGS) -c loadsave.c -o $(BUILD_FOLDER)/loadsave.o


clean:
	rm -rf $(BUILD_FOLDER) archiver