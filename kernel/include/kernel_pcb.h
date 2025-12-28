#ifndef KERNEL_PCB_H
#define KERNEL_PCB_HSS

#include "gestor_kernel.h"
#include "kernel_estructuras.h"

// Funciones para manejar procesos
void inicializar_metricas(t_pcb* proceso);
const char* estado_to_string(int estado);
void finalizar_metricas(t_pcb* proceso);
void mostrar_metricas(const t_pcb* proceso);
void inicializar_semaforos_pid();
void cambiar_estado(t_pcb* proceso, int nuevo_estado);
void cronometro();
#endif