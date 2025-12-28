#ifndef MEMORIA_SWAP_H
#define MEMORIA_SWAP_H

#include "gestor_memoria.h"

typedef struct {
    int pid;
    int pagina_inicial;
    int cantidad_paginas;
} entrada_swap_proceso_t;

void inicializar_swap();

//int tamaño_actual_swap_en_bytes();
int tamanio_actual_swap_en_paginas();

t_list* obtener_marcos_asignados(proceso_t* proceso);
entrada_swap_proceso_t* buscar_entrada_swap_por_pid(int pid);
bool asignar_marcos_a_tabla(tabla_pagina_t* tabla, int nivel_actual, int cantidad_marcos);
int obtener_marco_de_pagina(tabla_pagina_t* tabla, int nro_pagina_logica);
void borrar_espacio_swap(entrada_swap_proceso_t* entrada);
bool es_entrada_buscada(void* elem, void* pid_ptr);
void desuspender_proceso_de_swap(proceso_t* proceso);

void suspender_proceso_a_swap(entrada_swap_proceso_t* entrada, proceso_t* proceso);
void recolectar_marcos(tabla_pagina_t* tabla, int nivel_actual, t_list* lista_marcos, int cantidad_paginas);

t_list* obtener_marcos_asignados_suspender(proceso_t* proceso);
void recolectar_marcos_suspender(tabla_pagina_t* tabla, int nivel_actual, t_list* lista_marcos, int cantidad_paginas);

#endif
