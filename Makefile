CC = gcc
CFLAGS = -Wextra -Werror -Wall -Wno-gnu-folding-constant
TARGET = orsh

PREF_SRC = ./src/
PREF_OBJ = ./obj/

SRC = $(shell find . -name "*.c")
OBJ = $(patsubst $(PREF_SRC)%.c, $(PREF_OBJ)%.o, $(SRC))

all: $(TARGET)

$(TARGET) : $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(TARGET)

$(PREF_OBJ)%.o : $(PREF_SRC)%.c
	mkdir -p $(dir $@)
	$(CC) -c $< -o $@

clean :
	rm $(TARGET) $(OBJ)
