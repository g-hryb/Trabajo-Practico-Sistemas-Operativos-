#ifndef CPU_MEMORIA_H_
#define CPU_MEMORIA_H_

#include "gestor_cpu.h"
void atender_cpu_memoria();

extern char* instruccion;
extern sem_t* sem_instruccion;
extern sem_t* sem_pagina;
extern sem_t* sem_marco;

extern int entradasPorTabla;
extern int cantidadDeNiveles;
extern int tamanioPagina; 
extern int marco_proceso;
extern void* datos_memoria;
extern size_t tamanio_contenido;

void* limpiar_header_si_existe(void* contenido);

#endif // CPU_MEMORIA_H_