#ifndef MEMORIA_FINALIZAR_H_
#define MEMORIA_FINALIZAR_H_
#include "gestor_memoria.h"

bool proceso_esta_en_swap(proceso_t* proceso);
bool eliminar_proceso_en_memoria(uint32_t pid);
bool eliminar_proceso_en_swap(uint32_t pid);


void liberar_espacio_en_memoria(proceso_t * proceso_finalizar);
void mostrar_metricas_proceso(proceso_t * proceso);

#endif