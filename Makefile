# minihttpd - Makefile
CC = gcc
# Flags:
# -Wall -Wextra : warnings
# -Werror       : warnings como errores
# -std=c11      : estándar C11
# -g            : debug
# -O2           : optimización
CFLAGS = -Wall -Wextra -Werror -std=c11 -g -O2 -D_GNU_SOURCE
SRCDIR = src
INCDIR = include
OBJDIR = obj
# Todos los archivos .c
SRCS = $(wildcard $(SRCDIR)/*.c)
# Convertir .c -> .o
OBJS = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SRCS))
TARGET = minihttpd
# Compilación principal
all: $(TARGET)
# Crear ejecutable final
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -I$(INCDIR) -o $@ $^
# Compilar archivos objeto
$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -I$(INCDIR) -c $< -o $@
# Crear carpeta obj si no existe
$(OBJDIR):
	mkdir -p $(OBJDIR)
# Ejecutar servidor
run: all
	./$(TARGET) 8080
# Ejecutar con Valgrind
valgrind: all
	valgrind --leak-check=full ./$(TARGET) 8080
# Limpiar binarios
clean:
	rm -rf $(OBJDIR) $(TARGET)
.PHONY: all run clean valgrind