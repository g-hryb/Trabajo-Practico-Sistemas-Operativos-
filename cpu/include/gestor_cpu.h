#ifndef GESTOR_CPU_H_
#define GESTOR_CPU_H_

#include "../../utils/include/utils.h"

extern t_log* cpu_logger;
extern t_config* cpu_config;

extern uint32_t pid;
extern uint32_t pc;


extern int ID_CPU;
extern int fd_memoria;
extern int fd_kernel_dispatch;
extern int fd_kernel_interrupt;

extern char* IP_MEMORIA;
extern char* PUERTO_MEMORIA;
extern char* IP_KERNEL;
extern char* PUERTO_KERNEL_DISPATCH;
extern char* PUERTO_KERNEL_INTERRUPT;
extern int ENTRADAS_TLB;
extern char* REEMPLAZO_TLB;
extern int ENTRADAS_CACHE;
extern char* REEMPLAZO_CACHE;
extern int RETARDO_CACHE;
extern char* LOG_LEVEL;

#endif