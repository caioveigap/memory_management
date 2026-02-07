#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <stdio.h>
#include <time.h>
#include <stdint.h>

// Estrutura para guardar os dados de cada função
typedef struct Function_Profile {
    const char *func_name;
    uint64_t total_ns;      // Acumulador de tempo total em nanosegundos
    uint64_t func_calls;    // Contador de chamadas
    struct Function_Profile *next;
} Function_Profile;

// Cabeça da lista global (começa vazia)
static Function_Profile *_global_benchmark = NULL;

// Helper para pegar o tempo atual em nanosegundos (monotônico)
static inline uint64_t benchmark_get_nanos() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

// Registra uma nova função na lista encadeada (Executado apenas 1x por função)
static inline void benchmark_register(Function_Profile *p, const char *func_name) {
    p->func_name = func_name;
    p->total_ns = 0;
    p->func_calls = 0;
    
    // Inserção na cabeça da lista (LIFO)
    // 1. O próximo do novo nó aponta para a cabeça atual
    p->next = _global_benchmark;
    // 2. A cabeça global passa a ser o novo nó
    _global_benchmark = p; 
}

// --- Macros de Controle ---

// Inicia o cronômetro (com nome personalizado)
#define BENCHMARK_START_NAME(NAME_STR) \
    static Function_Profile _profile_data = {0}; \
    static int _profile_initialized = 0; \
    if (!_profile_initialized) { \
        benchmark_register(&_profile_data, NAME_STR); \
        _profile_initialized = 1; \
    } \
    uint64_t _start_ns = benchmark_get_nanos(); // Variável LOCAL na stack

// Atalho para usar o nome da própria função
#define BENCHMARK_START() BENCHMARK_START_NAME(__func__)

// Para o cronômetro e acumula os resultados
#define BENCHMARK_END() \
    uint64_t _end_ns = benchmark_get_nanos(); \
    _profile_data.total_ns += (_end_ns - _start_ns); \
    _profile_data.func_calls++;

// --- Função de Relatório (Output Formatado) ---

static void benchmark_print_stats() {
    printf("\n============================================================================================\n");
    printf("| %-20s | %-10s | %-15s | %-10s | %-15s |\n", 
           "Função", "Chamadas", "Média (ns)", "Total (s)", "Total (ns)");
    printf("============================================================================================\n");
    
    Function_Profile* current = _global_benchmark;
    while (current != NULL) {
        double media = 0;
        if (current->func_calls > 0) {
            media = (double)current->total_ns / current->func_calls;
        }

        // Separa Segundos e Nanossegundos para exibição
        uint64_t total_seconds = current->total_ns / 1000000000ULL;
        uint64_t total_remainder_ns = current->total_ns % 1000000000ULL;

        printf("| %-20s | %-10lu | %-15.2f | %-10lu | %-15lu |\n", 
               current->func_name,     // Nome
               current->func_calls,    // Qtd Chamadas
               media,                  // Média
               total_seconds,          // Segundos inteiros
               total_remainder_ns      // Nanossegundos restantes
        );
               
        current = current->next;
    }
    printf("============================================================================================\n");
}

#endif