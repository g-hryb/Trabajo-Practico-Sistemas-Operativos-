#ifndef CPU_H_
#define CPU_H_

#include "inicializar_cpu.h"
#include "gestor_cpu.h"
#include "cpu_kernel_dispatch.h"
#include "cpu_kernel_interrupt.h"
#include "cpu_memoria.h"
#include "cpu_ciclo_instruccion.h"

//--------- variables globales -------------

t_log* cpu_logger;
t_config* cpu_config;

uint32_t pid;
uint32_t pc = 0;



int ID_CPU;
int fd_memoria;
int fd_kernel_dispatch;
int fd_kernel_interrupt;

char* IP_MEMORIA;
char* PUERTO_MEMORIA;
char* IP_KERNEL;
char* PUERTO_KERNEL_DISPATCH;
char* PUERTO_KERNEL_INTERRUPT;
int ENTRADAS_TLB;
char* REEMPLAZO_TLB;
int ENTRADAS_CACHE;
char* REEMPLAZO_CACHE;
int RETARDO_CACHE;
char* LOG_LEVEL;

extern pthread_mutex_t * mutex_tlb;
extern pthread_mutex_t * mutex_cache;

/* IP_MEMORIA=127.0.0.1  String
PUERTO_MEMORIA=8002 NUm
IP_KERNEL=127.0.0.1 String
PUERTO_KERNEL_DISPATCH=8001 Num
PUERTO_KERNEL_INTERRUPT=8004 NUm
ENTRADAS_TLB=4 Num
REEMPLAZO_TLB=LRU String
ENTRADAS_CACHE=2 Num
REEMPLAZO_CACHE=CLOCK String
RETARDO_CACHE=250 NUm
LOG_LEVEL=TRACE  String
*/ 

#endif
