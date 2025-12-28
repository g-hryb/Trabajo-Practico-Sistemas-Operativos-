#ifndef INICIALIZAR_KERNEL_H_
#define INICIALIZAR_KERNEL_H_

#include "gestor_kernel.h"
#include "planificadores.h"

void inicializar_kernel();
void inicializar_config_kernel();
void inicializar_logger_kernel();
void loggear_config_kernel();
void inicializar_cola_ready(); // Para inicializar la cola de procesos en READY
t_cola_proceso* inicializar_cola_procesos_ready();


#endif