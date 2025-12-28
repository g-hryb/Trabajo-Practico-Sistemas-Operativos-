#ifndef IO_H_
#define IO_H_

#include "gestor_io.h"
#include "inicializar_io.h"
#include "io_kernel.h"


//--------- variables globales -------------
t_log* io_logger;
t_config* io_config;

int fd_kernel;
int SIZE_ID_IO;

char* ID_IO;
char* IP_KERNEL; 
char* PUERTO_KERNEL; 
char* LOG_LEVEL; 

/*
IP_KERNEL=127.0.0.1
PUERTO_KERNEL=8003
LOG_LEVEL=TRACE*/


#endif
