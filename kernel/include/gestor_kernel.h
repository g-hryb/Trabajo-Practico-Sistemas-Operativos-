#ifndef GESTOR_KERNEL_H_
#define GESTOR_KERNEL_H_

#include "../../utils/include/utils.h"


extern t_log* kernel_logger;
extern t_config* kernel_config;
extern pthread_mutex_t mutex_cpus;
extern pthread_mutex_t mutex_ios;

extern char* archivo_pseudocodigo;
extern int tamano_proceso;
extern char* archivo_config;

extern int fd_memoria;
extern int fd_kernel_dispatch;
extern int fd_kernel_interrupt;
extern int fd_kernel_io;
//extern int fd_cpu_dispatch;
//extern int fd_cpu_interrupt;
//extern int fd_io;

extern char* IP_MEMORIA; 
extern char* PUERTO_MEMORIA; 
extern char* PUERTO_ESCUCHA_DISPATCH; 
extern char* PUERTO_ESCUCHA_INTERRUPT;
extern char* PUERTO_ESCUCHA_IO; 
extern char* ALGORITMO_CORTO_PLAZO; 
extern char* ALGORITMO_INGRESO_A_READY; 
extern double ALFA; 
extern double ESTIMACION_INICIAL;
extern int TIEMPO_SUSPENSION; 
extern char* LOG_LEVEL; 

#endif