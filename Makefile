CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c11 -g -O2 -D_GNU_SOURCE
SRCDIR = src
INCDIR = include
OBJDIR = obj
SRCS = $(wildcard $(SRCDIR)/*.c)
OBJS = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SRCS))
TARGET = minihttpd
all: $(TARGET)
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -I$(INCDIR) -o $@ $^
$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -I$(INCDIR) -c $< -o $@
$(OBJDIR):
	mkdir -p $(OBJDIR)
run: all
	./$(TARGET) 8080
clean:
	rm -rf $(OBJDIR) $(TARGET)
.PHONY: all run clean