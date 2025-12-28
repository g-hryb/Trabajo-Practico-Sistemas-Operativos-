#ifndef CPU_KERNEL_DISPATCH
#define CPU_KERNEL_DISPATCH

#include "gestor_cpu.h"
#include "cpu_ciclo_instruccion.h"

extern pthread_mutex_t * mutex_tlb;
extern pthread_mutex_t * mutex_cache;

extern pthread_mutex_t * mutex_pid;
extern pthread_mutex_t * mutex_pc;
extern sem_t* sem_pid_pc;
extern sem_t* sem_contexto;

extern bool esperar_contexto;

void inicializar_semaforos_pid_pc();
void atender_cpu_kernel_dispatch();
void atender_contexto(t_log* cpu_logger); //cambiar o sacar

#endif // CPU_KERNEL_DISPATCH