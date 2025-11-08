CC := gcc
CFLAGS := -std=c99 -Wall -Wextra -Wpedantic -g
SRCDIR := src
INCDIR := include
OBJDIR := obj
SOURCES := $(wildcard $(SRCDIR)/*.c)

# transforms each source path to a corresponding object file path
# e.g. src/main.c -> obj/main.o
OBJECTS = $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

TARGET = a1 # the name of the final executable

# special targets not associated with files
# https://stackoverflow.com/questions/2145590/what-is-the-purpose-of-phony-in-a-makefile
.PHONY: all clean install

all: $(TARGET)

# linking
# $@ = target
$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@

# compilation (compile each .c file to .o)
# | $(OBJDIR) ensures obj dir exists before compilation
$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -I$(INCDIR) -c $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	rm -rf $(OBJDIR) $(TARGET)

# requires sudo
install: $(TARGET)
	cp $(TARGET) /usr/local/bin/
