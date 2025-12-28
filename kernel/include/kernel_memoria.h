#ifndef KERNEL_MEMORIA_H
#define KERNEL_MEMORIA_H

#include "gestor_kernel.h"

extern sem_t* binario_respuesta_crear_proceso;
extern sem_t* binario_respuesta_eliminar_proceso;
extern sem_t* binario_respuesta_desuspender_proceso;
extern bool respuesta_crear_proceso;
extern bool respuesta_eliminar_proceso;
extern bool respuesta_desuspender_proceso;

void atender_kernel_memoria();
void inicializar_binario_respuestas();

#endif // KERNEL_MEMORIA_H