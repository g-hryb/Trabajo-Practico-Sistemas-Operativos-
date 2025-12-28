#ifndef KERNEL_H_
#define KERNEL_H_

#include "gestor_kernel.h"
#include "inicializar_kernel.h"
#include "kernel_memoria.h"
#include "kernel_cpu_dispatch.h"
#include "kernel_cpu_interrupt.h"
#include "kernel_io.h"
#include "kernel_estructuras.h"
#include "kernel_pcb.h"
#include "kernel_manejo_cpus.h"
#include "kernel_manejo_ios.h"
#include "planificadores.h"


//--------- variables globales -------------
t_log* kernel_logger;
t_config* kernel_config;
pthread_mutex_t mutex_cpus;
pthread_mutex_t mutex_ios;

char* archivo_pseudocodigo;
int tamano_proceso;
char* archivo_config;

int fd_memoria;
int fd_kernel_dispatch;
int fd_kernel_interrupt;
int fd_kernel_io;
//int fd_cpu_dispatch;
//int fd_cpu_interrupt;
//int fd_io;

char* IP_MEMORIA; 
char* PUERTO_MEMORIA; 
char* PUERTO_ESCUCHA_DISPATCH; 
char* PUERTO_ESCUCHA_INTERRUPT;
char* PUERTO_ESCUCHA_IO; 
char* ALGORITMO_CORTO_PLAZO; 
char* ALGORITMO_INGRESO_A_READY; 
double ALFA; 
double ESTIMACION_INICIAL;
int TIEMPO_SUSPENSION; 
char* LOG_LEVEL; 

/*IP_MEMORIA=127.0.0.1
PUERTO_MEMORIA=8002
PUERTO_ESCUCHA_DISPATCH=8001
PUERTO_ESCUCHA_INTERRUPT=8004
PUERTO_ESCUCHA_IO=8003
ALGORITMO_CORTO_PLAZO=FIFO
ALGORITMO_INGRESO_A_READY=PMCP
ALFA=0.5
TIEMPO_SUSPENSION=4500
LOG_LEVEL=TRACE
*/





#endif
