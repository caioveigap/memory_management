CC = gcc
# -Iinclude: Garante que o compilador ache seus headers em allocator_manager/include/
CFLAGS_COMMON = -Wall -Wextra -Iinclude

# --- Configurações de Modo (Release vs Debug) ---
# Release: Otimização máxima (-O3), remove asserts (-DNDEBUG)
CFLAGS_RELEASE = $(CFLAGS_COMMON) -O3 -DNDEBUG
# Debug: Símbolos de debug (-g), sem otimização (-O0), ativa macros de debug (-DDEBUG)
CFLAGS_DEBUG = $(CFLAGS_COMMON) -g -O0 -DDEBUG

# Lógica de decisão: Se rodar 'make DEBUG=1' ou 'make debug', usa flags de debug
ifdef DEBUG
    CFLAGS = $(CFLAGS_DEBUG)
else
    CFLAGS = $(CFLAGS_RELEASE)
endif

# --- Diretórios ---
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin
TEST_DIR = test

# --- 1. Compilação da Biblioteca (Core) ---
# Encontra automaticamente todos os .c dentro de src/ (backend_manager.c, pool.c, heap.c)
LIB_SRCS = $(wildcard $(SRC_DIR)/*.c)
# Cria a lista de objetos esperados: src/arquivo.c -> obj/arquivo.o
LIB_OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(LIB_SRCS))

# --- 2. Compilação dos Testes (Executáveis) ---
# Encontra automaticamente todos os .c dentro de test/ (test_01.c, test_benchmark.c, etc)
TEST_SRCS = $(wildcard $(TEST_DIR)/*.c)
# Define o nome dos executáveis finais: test/exemplo.c -> bin/exemplo (sem extensão .out)
TEST_BINS = $(patsubst $(TEST_DIR)/%.c, $(BIN_DIR)/%, $(TEST_SRCS))

# --- Regras Principais ---

# O alvo 'all' agora constrói a lib E todos os testes encontrados
all: directories $(LIB_OBJS) $(TEST_BINS)

directories:
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(BIN_DIR)

# Regra Mágica para Executáveis de Teste
# Lê-se: "Para criar um arquivo em bin/, pegue o correspondente em test/%.c 
# E misture com todos os objetos da biblioteca (LIB_OBJS)"
$(BIN_DIR)/%: $(TEST_DIR)/%.c $(LIB_OBJS)
	@echo "Compilando teste: $@"
	$(CC) $(CFLAGS) $< $(LIB_OBJS) -o $@

# Regra para compilar os objetos da biblioteca (.c -> .o)
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# --- Comandos Utilitários ---

# Atalho para compilar em modo debug (limpa tudo antes para garantir recompilação)
debug: clean
	@echo "=== BUILD EM MODO DEBUG ==="
	$(MAKE) all DEBUG=1

# Limpa a sujeira
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

# Ajuda para rodar testes específicos
# Uso: make run t=test_01
run:
ifdef t
	./$(BIN_DIR)/$(t)
else
	@echo "Erro: Especifique o teste com t=..."
	@echo "Exemplo: make run t=test_01"
	@echo "Executáveis disponíveis:"
	@ls $(BIN_DIR) 2>/dev/null || echo "(Nenhum compilado ainda)"
endif

.PHONY: all directories clean debug run