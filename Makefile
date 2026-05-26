# ============================================================
# Makefile – minihttpd
# ============================================================
# Uso:
#   make          → compilar el servidor
#   make clean    → borrar binarios y objetos
#   make run      → compilar y ejecutar en puerto 8080
#   make valgrind → ejecutar con Valgrind (detección de memory leaks)
# ============================================================

# ── Compilador y flags ───────────────────────────────────────
CC      = gcc

# Flags de compilación:
#   -Wall -Wextra  → activar todos los warnings
#   -Werror        → tratar warnings como errores (código limpio)
#   -std=c11       → estándar C11
#   -g             → símbolos de debug (para gdb/valgrind)
#   -O2            → optimización nivel 2
#   -D_GNU_SOURCE  → habilitar extensiones GNU (epoll, etc.)
CFLAGS  = -Wall -Wextra -Werror -std=c11 -g -O2 -D_GNU_SOURCE

# Directorios
SRCDIR  = src
INCDIR  = include
OBJDIR  = obj

# Archivos fuente y objetos
SRCS    = $(SRCDIR)/main.c   \
          $(SRCDIR)/server.c \
          $(SRCDIR)/http.c   \
          $(SRCDIR)/mime.c   \
          $(SRCDIR)/files.c

OBJS    = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SRCS))

# Binario de salida
TARGET  = minihttpd

# ── Regla principal ──────────────────────────────────────────
all: $(OBJDIR) $(TARGET)

# Crear directorio obj si no existe
$(OBJDIR):
	mkdir -p $(OBJDIR)

# Enlazar todos los objetos en el ejecutable final
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -I$(INCDIR) -o $@ $^
	@echo ""
	@echo "  ✓ Compilación exitosa → ./$(TARGET)"
	@echo "  Ejecuta con:  ./$(TARGET)"
	@echo "  O con puerto: ./$(TARGET) 9090"
	@echo ""

# Compilar cada .c a su .o correspondiente
$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -I$(INCDIR) -c $< -o $@

# ── Limpiar artefactos de compilación ────────────────────────
clean:
	rm -rf $(OBJDIR) $(TARGET)
	@echo "  ✓ Limpieza completada"

# ── Ejecutar el servidor directamente ────────────────────────
run: all
	./$(TARGET) 8080

# ── Ejecutar con Valgrind para detectar memory leaks ─────────
valgrind: all
	valgrind --leak-check=full --show-leak-kinds=all ./$(TARGET) 8080

# ── Evitar conflictos con archivos que se llamen igual ───────
.PHONY: all clean run valgrind
