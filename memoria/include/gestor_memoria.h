#ifndef GESTOR_MEMORIA_H_
#define GESTOR_MEMORIA_H_

#include "../../utils/include/utils.h"

extern t_log* memoria_logger;
extern t_config* memoria_config;

extern t_list* lista_procesos; // es el registro de procesos activos en memoria
//es la forma de acceder a la tabla de paginas y los demas datos de un proceso
extern pthread_mutex_t* mutex_lista_procesos; // Mutex para proteger el acceso a lista_procesos
extern pthread_mutex_t* mutex_procesos_en_swap; // Mutex para proteger el acceso a procesos_en_swap
extern pthread_mutex_t* mutex_bitmap; // Mutex para proteger el acceso al bitmap
extern pthread_mutex_t* mutex_memoria_ocupada;

//--------- variables globales -------------

typedef struct tabla_pagina {
    bool es_entrada_final;
    int cantidad_entradas; //AAAAAAAA
    union {
        struct tabla_pagina** subtablas; // Para niveles intermedios
        int* marcos;                     // Para el último nivel: número de marco físico
        //marcos es un array de enteros que representa los marcos físicos asignados a las páginas
    };
} tabla_pagina_t;

//tabla_pagina_t* crear_tabla_pagina(int nivel_actual);

typedef struct {
    char** instrucciones;  // Array de strings, cada string es una instrucción
    int cantidad;         // Cantidad de instrucciones
    int proxima;          // Índice de la próxima instrucción a devolver
} pseudocodigo_t;

typedef struct {    
    uint32_t pid;
    pseudocodigo_t* pseudocodigo;
    tabla_pagina_t* tabla_raiz; //es la tabla de nivel mas alto, tiene las entradas a las siguientes tablas
    int accesos_tablas_paginas;
    int instrucciones_solicitadas;
    int bajadas_a_swap;
    int subidas_a_memoria;
    int lecturas_memoria;
    int escrituras_memoria;
    // Otros campos según necesidad

    int cantidad_paginas; // Cantidad de páginas asignadas al proceso
} proceso_t;


//extern t_list* lista_procesos;

// configuracion_memoria
//extern int CANTIDAD_ENTRADAS_POR_TABLA;
//extern int CANTIDAD_NIVELES;

//Parte de la estructura de la memoria:
extern void* espacio_usuario; //

extern int fd_memoria;
extern int fd_kernel;
extern int fd_cpu;

extern char* archivo_config;

extern char* PUERTO_ESCUCHA;
extern int TAM_MEMORIA;
extern int TAM_PAGINA;
extern int ENTRADAS_POR_TABLA;
extern int CANTIDAD_NIVELES;
extern int RETARDO_MEMORIA;
extern char* PATH_SWAPFILE;
extern int RETARDO_SWAP;
extern char* LOG_LEVEL;
extern char* DUMP_PATH;
extern char* PATH_INSTRUCCIONES;

//Variables globales para el numero maximo de marcos: 17/06
extern int framesMaximos;// 17/06
extern int MEMORIA_OCUPADA; // 17/06
extern bool* bitmap; //marcos ocupados o libres, 17/06

//SWAP
extern t_list* procesos_en_swap;
extern FILE* archivo_swap;

#endif