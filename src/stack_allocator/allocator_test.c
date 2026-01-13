#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stack_alloc.h"

// --- Ferramentas Visuais ---

// Limpa a tela (Linux/Mac)
void clear_screen() {
    printf("\033[H\033[J");
}

// Cores ANSI para o terminal
#define COLOR_RESET   "\033[0m"
#define COLOR_HEADER  "\033[31m" // Vermelho para Headers
#define COLOR_DATA    "\033[32m" // Verde para Dados
#define COLOR_EMPTY   "\033[90m" // Cinza para Vazio

void print_visual_stack(Stack *s) {
    printf("\n=== VISUALIZADOR DE MEMÓRIA ===\n");
    printf("Offset Atual: %zu | Capacidade: %zu\n", s->offset, s->capacity);
    printf("Legenda: " COLOR_HEADER "[HEADER] " COLOR_DATA "[DADOS] " COLOR_EMPTY "[VAZIO/LIXO]" COLOR_RESET "\n");
    printf("----------------------------------------------------------------\n");
    printf("Offset  | Conteúdo (Hex)\n");
    printf("----------------------------------------------------------------\n");

    for (size_t i = 0; i < s->capacity; i += 16) {
        // Se já passamos muito do offset e só tem zero, paramos para não poluir
        if (i > s->offset + 32) break; 

        printf("%04zX    | ", i);

        for (size_t j = 0; j < 16; j++) {
            size_t current_addr = i + j;
            unsigned char byte = s->buffer[current_addr];
            
            // Lógica de coloração
            if (current_addr < s->offset) {
                // Aqui é difícil saber exato o que é header ou dado sem rastrear
                // mas vamos pintar tudo de verde se estiver alocado
                // Para ser perfeito, precisariamos guardar metadados extras.
                // Por hora, pintamos alocados de verde brilhante.
                printf(COLOR_DATA "%02X " COLOR_RESET, byte);
            } else {
                // Memória "livre" (ou lixo antigo)
                if (byte == 0) {
                    printf(COLOR_EMPTY "00 " COLOR_RESET);
                } else {
                    // "Ghost Data": Dados liberados mas ainda não sobrescritos
                    printf(COLOR_EMPTY "%02X " COLOR_RESET, byte);
                }
            }
        }
        printf("\n");
    }
    printf("----------------------------------------------------------------\n");
}

void pause_execution(const char* message) {
    printf("\n>>> AÇÃO: %s\n", message);
    printf("[Pressione ENTER para continuar...]");
    while(getchar() != '\n');
    clear_screen();
}

// --- Cenário de Teste ---

int main() {
    // 1. Configuração Inicial
    unsigned char buffer[256]; // Buffer pequeno para facilitar visualização
    // Limpamos o buffer com 0 para ficar bonito, mas na real não precisaria
    memset(buffer, 0, 256); 
    
    Stack my_stack;
    stack_init(&my_stack, buffer, 256);

    clear_screen();
    print_visual_stack(&my_stack);
    pause_execution("Início: Stack Vazia");

    // 2. Alocação 1: Um Inteiro (4 bytes)
    // Isso vai gerar Padding + Header + Int
    int *num = (int*)stack_alloc(&my_stack, sizeof(int));
    if (num) *num = 0xAABBCCDD; // Preenchemos com padrão reconhecível
    
    print_visual_stack(&my_stack);
    printf("Info: Alocado int (0xAABBCCDD) com alinhamento 16\n");
    pause_execution("Alocação 1 Concluída");

    // 3. Alocação 2: Uma String curta
    char *text = (char*)stack_alloc_align(&my_stack, 12, 1); // Alinhamento 1 byte (sem padding extra)
    if (text) strcpy(text, "OlaMundo!");
    
    print_visual_stack(&my_stack);
    printf("Info: Alocado string 'OlaMundo!' logo após o int\n");
    pause_execution("Alocação 2 Concluída");

    // 4. Alocação 3: Um Array de Bytes
    unsigned char *arr = (unsigned char*)stack_alloc_align(&my_stack, 8, 8);
    if (arr) memset(arr, 0xFF, 8); // Preenche tudo com FF
    
    print_visual_stack(&my_stack);
    printf("Info: Alocado bloco de 0xFF com alinhamento 8\n");
    pause_execution("Alocação 3 Concluída");

    // 5. O Teste de Free (LIFO)
    // Vamos liberar o array de 0xFF. O offset deve voltar para o fim da string.
    stack_free(&my_stack, arr);
    
    print_visual_stack(&my_stack);
    printf("Info: Liberado o bloco de 0xFF.\n");
    printf("Note que os bytes 'FF' ainda estão lá (Ghost Data),\nmas a cor mudou (estão após o offset).\n");
    pause_execution("Free (Pop) Concluído");

    // 6. Sobrescrita (Prova Real)
    // Vamos alocar algo novo. Ele DEVE sobrescrever os FFs antigos.
    char *text2 = (char*)stack_alloc_align(&my_stack, 8, 8);
    if (text2) strcpy(text2, "NOVODADO");

    print_visual_stack(&my_stack);
    printf("Info: Nova alocação feita. Note que 'FF' foi substituído por 'NOVODADO'.\n");
    pause_execution("Sobrescrita Concluída");

    return 0;
}