#ifndef MEMORIA_H_
#define MEMORIA_H_

#include "gestor_memoria.h"
#include "inicializar_memoria.h"
#include "memoria_cpu.h"
#include "memoria_kernel.h"
#include "memoria_proceso.h"
#include "memoria_manejo_cpus.h"

//--------- variables globales -------------
t_log* memoria_logger;
t_config* memoria_config;

int fd_memoria;
int fd_kernel;
//int fd_cpu;

char* archivo_config;

char* PUERTO_ESCUCHA;
int TAM_MEMORIA;
//int TAM_PAGINA;
//int ENTRADAS_POR_TABLA;
//int CANTIDAD_NIVELES;
int RETARDO_MEMORIA;
char* PATH_SWAPFILE;
int RETARDO_SWAP;
char* LOG_LEVEL;
char* DUMP_PATH;
char* PATH_INSTRUCCIONES;


/* 
PUERTO_ESCUCHA=8002 
TAM_MEMORIA=4096
TAM_PAGINA=64
ENTRADAS_POR_TABLA=4
CANTIDAD_NIVELES=3
RETARDO_MEMORIA=1500
PATH_SWAPFILE=/home/utnso/swapfile.bin
RETARDO_SWAP=15000
LOG_LEVEL=TRACE
DUMP_PATH=/home/utnso/dump_files/

 */

#endif