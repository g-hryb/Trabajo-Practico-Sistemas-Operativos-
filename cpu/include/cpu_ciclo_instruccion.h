#ifndef CPU_CICLO_INSTRUCCION
#define CPU_CICLO_INSTRUCCION

#include "gestor_cpu.h"



typedef enum{
    NOOP,
    WRITE,
    READ,
    GOTO,
    IO,
    INIT_PROC,
    DUMP_MEMORY,
    EXIT
}instruccion_cpu;



typedef struct t_tlb {
    int nro_pagina ; //-1 si no hay pagina
    u_int32_t pid; //-1 si no hay pid
    int marco; //-1 si no hay marco
    int tiempo_en_tlb; // Tiempo que la entrada ha estado en TLB ???????????????
} t_tlb;

typedef struct t_cache {
    int nro_pagina; //-1 si no hay pagina
    u_int32_t pid; //-1 si no hay pid
    void* contenido; //-1 si no hay contenido
    size_t tamanio_contenido; //-1 si no hay contenido
    int bit_uso; 
    int modificado; //  0 o 1
    int tiempo_en_cache;
    uint32_t direccionFisica;
} t_cache;

extern t_tlb* tlb;
extern t_cache* cache;


extern bool fin_proceso;
extern bool io_dump; 
extern char* ultimo_contenido_agregado;


extern pthread_mutex_t * mutex_tlb;
extern pthread_mutex_t * mutex_cache;
extern sem_t* sem_interrupt; 

void limpiar_cadena(char* str);
char* fetch();
void decode(char* instruccion);
char* pedir_a_memoria_instruccion(uint32_t pc, uint32_t pid);
void execute(int instruccion, char** argumentos,uint32_t pid);
void separar_en_tres_strings(const char* linea, char** argumentos);
int obtener_instruccion(char* string);
void limpiar_cadena(char* str);
uint32_t mmu(int tamanioPagina,int marco, int direccionLogica );
int tlb_hit(int numero_pagina,u_int32_t pid,t_tlb* tlb);
int cache_hit(int numero_pagina,t_cache* cache);
int nro_pagina(int direccionLogica, int tamanioPagina);
int* entrada_nivel_X(int nro_Pagina, int cant_entradas_tabla, int cant_niveles);
void algoritmo_tlb_fifo(t_tlb* tlb, int nro_pagina, u_int32_t pid, int marco);
int nro_pagina_en_tlb(t_tlb* tlb, int nro_pagina, u_int32_t pid);
int pagina_mas_antigua(t_tlb* tlb);
void actualizar_tlb(t_tlb* tlb, int nro_pagina, u_int32_t pid, int marco);
void actualizar_cache(t_cache* cache,t_tlb* tlb, int nro_pagina, u_int32_t pid, char* contenido,size_t size_contenido, int modificado, int direccionLogica, uint32_t direccionFisica);
int buscar_en_MP(int numero_pagina, int entradasPorTabla, int cantidadDeNiveles,int tamanioPagina, int direccionLogica, u_int32_t pid);
void ejecutar_write(int direccionLogica, char* datos,uint32_t pid,t_cache* cache, t_tlb* tlb);
void ejecutar_read(int direccionLogica, int tamanio,uint32_t pid,t_cache* cache, t_tlb* tlb);
void algoritmo_tlb_lru(t_tlb* tlb, int nro_pagina, u_int32_t pid, int marco);
int pagina_vacia_tlb(t_tlb* tlb);
int pagina_vacia_cache(t_cache* cache);
int nro_pagina_en_cache(t_cache* cache, int nro_pagina, u_int32_t pid);
int pagina_mas_antigua_cache(t_cache* cache);
void algoritmo_cache_clock(t_cache* cache,t_tlb* tlb , int nro_pagina, u_int32_t pid, char* contenido,size_t size_contenido, int modificado, int direccionLogica, uint32_t direccionFisica);
void algoritmo_cache_clock_modificado(t_cache* cache,t_tlb* tlb , int nro_pagina, u_int32_t pid, char* contenido,size_t size_contenido, int modificado, int direccionLogica, uint32_t direccionFisica);
bool todos_bit_uso_en_uno(t_cache* cache);
void desalojar_processo_tlb(t_tlb* tlb);
void desalojar_processo_cache(t_cache* cache);
void inicializar_tlb();
void inicializar_cache();
void inicializar_semaforos_tlb();
void inicializar_semaforos_cache();
int entrada_bit_uso_0_mas_antigua(t_cache* cache);
void check_interrupt();
int victima_clock_m(t_cache* cache, t_tlb* tlb, int nro_pagina, u_int32_t pid, char* contenido, int modificado, int direccionLogica);
int entrada_bit_uso_0_modificado_1_mas_antigua(t_cache* cache);
bool existe_0_1(t_cache* cache);
void cargar_contenido(t_cache* cache,t_tlb*tlb, int indice, char*contenido, size_t size_contenido, uint32_t direccionFisica, int direccionLogica);
void inicializar_semaforo_interrupt();
void desalojar_proceso(t_tlb* tlb, t_cache* cache);
void desalojar_proceso_cache(t_cache* cache);
void desalojar_proceso_tlb(t_tlb* tlb);
int calcular_desplazamiento(int direccionLogica);
void leer_void_desde_memoria(void* data, size_t tamanio_contenido, uint32_t direccionFisica);
void limpiar_salto_de_linea(char* str);
void mostrar_cache(t_cache cache);
void limpiar_y_copiar_a_cache(void* pagina, t_cache *cache, int indice);

#endif