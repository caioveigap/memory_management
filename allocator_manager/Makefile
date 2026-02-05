CC = gcc
# Flags de compilação (adicionei -Isrc para headers)
CFLAGS = -Wall -Wextra -g -O0 -Isrc

# Definição dos diretórios
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

# Encontra automaticamente todos os arquivos .c em src/
# Se preferir listar manualmente: SRCS = src/main.c src/backend_manager.c
# SRCS = $(wildcard $(SRC_DIR)/*.c)
SRCS = src/test_suite.c src/pool.c src/backend_manager.c

# Cria a lista de objetos: troca "src/%.c" por "obj/%.o"
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

# Nome do executável final (agora dentro de bin/)
TARGET = $(BIN_DIR)/main

# Regra padrão: garante que as pastas existem antes de compilar
all: directories $(TARGET)

# Cria os diretórios obj e bin se não existirem
directories:
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(BIN_DIR)

# Regra de Linkagem: Linka os objetos dentro de obj/ para criar o binário em bin/
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Regra de Compilação: Compila .c de src/ para .o em obj/
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Limpa as pastas geradas
clean:
	rm -f $(OBJ_DIR)/*.o $(BIN_DIR)/*

# Atalho para rodar
run: all
	./$(TARGET)

.PHONY: all directories clean run