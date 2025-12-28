#include "../include/inicializar_memoria.h"
#include <sys/stat.h> // Para usar stat y mkdir
#include <unistd.h>   // Para funciones relacionadas con el sistema de archivos

void* espacio_usuario = NULL; // No se si va aca igual. Preguntar
int framesMaximos = 0;
bool* bitmap = NULL; // Mapa de bits para los marcos físicos

//int CANTIDAD_ENTRADAS_POR_TABLA;
int CANTIDAD_NIVELES;
int ENTRADAS_POR_TABLA;
int TAM_PAGINA;
int MEMORIA_OCUPADA = 0; // Inicializar la memoria ocupada a 0 17/6

//SWAP
FILE* archivo_swap = NULL;
t_list* procesos_en_swap = NULL;

//MUTEX
pthread_mutex_t* mutex_lista_procesos;
pthread_mutex_t* mutex_procesos_en_swap;
pthread_mutex_t* mutex_bitmap;
pthread_mutex_t* mutex_memoria_ocupada;

void inicializar_memoria(){
    inicializar_config_memoria();
    inicializar_logger_memoria();
    inicializar_lista_procesos(); // Agregado por agus y vicky
    inicializar_espacio_de_usuario(); //3/06/25
    inicializar_semaforos();
    loggear_config_memoria();
    inicializar_frames_maximos();
    inicializar_swap(); // Inicializar swap
    inicializar_dump_path(); // Inicializar el path del dump
    //
}

void inicializar_semaforos(){
    
    mutex_lista_procesos = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(mutex_lista_procesos, NULL);

    mutex_procesos_en_swap = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(mutex_procesos_en_swap, NULL);
    
    mutex_bitmap = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(mutex_bitmap, NULL);

    mutex_memoria_ocupada = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(mutex_memoria_ocupada, NULL);
}

void inicializar_logger_memoria(){
    t_log_level log_level = log_level_from_string(LOG_LEVEL);
    memoria_logger = log_create("memoria.log", "MEMORIA_LOG", 1, log_level);
        if(memoria_logger == NULL){
            perror("Algo raro paso con el log. No se pudo crear o encontrar el archivo");
            exit(EXIT_FAILURE);
        }
}

void inicializar_config_memoria(){
    char* path_base = "/home/utnso/tp-2025-1c-TP-Aprobado/memoria/";
    // Crear un string dinámico y copiar el path base
    char* path = string_new();
    string_append(&path, path_base);
    printf("Path del archivo de configuración: %s\n", path);
    // Concatenar el nombre del archivo de configuración
    string_append(&path, archivo_config);

    memoria_config = config_create(path);
        if(memoria_config == NULL){
            perror("No se pudo crear el config");
            exit(EXIT_FAILURE);
        }
    // Cargar configuracion
    PUERTO_ESCUCHA = config_get_string_value(memoria_config, "PUERTO_ESCUCHA");
    TAM_MEMORIA = config_get_int_value(memoria_config, "TAM_MEMORIA");
    TAM_PAGINA = config_get_int_value(memoria_config, "TAM_PAGINA");
    ENTRADAS_POR_TABLA = config_get_int_value(memoria_config, "ENTRADAS_POR_TABLA");
    CANTIDAD_NIVELES = config_get_int_value(memoria_config, "CANTIDAD_NIVELES");
    RETARDO_MEMORIA = config_get_int_value(memoria_config, "RETARDO_MEMORIA");
    PATH_SWAPFILE = config_get_string_value(memoria_config, "PATH_SWAPFILE");
    RETARDO_SWAP = config_get_int_value(memoria_config, "RETARDO_SWAP");
    DUMP_PATH = config_get_string_value(memoria_config, "DUMP_PATH");
    PATH_INSTRUCCIONES = config_get_string_value(memoria_config, "PATH_INSTRUCCIONES");
    LOG_LEVEL = config_get_string_value(memoria_config, "LOG_LEVEL");    

    free(path); // Liberar la memoria del path dinámico
    }
    
void loggear_config_memoria(){

    // Imprimir configuracion
    log_trace(memoria_logger, "PUERTO_ESCUCHA: %s", PUERTO_ESCUCHA);
    log_trace(memoria_logger, "TAM_MEMORIA: %d", TAM_MEMORIA);
    log_trace(memoria_logger, "TAM_PAGINA: %d", TAM_PAGINA);
    log_trace(memoria_logger, "ENTRADAS_POR_TABLA: %d", ENTRADAS_POR_TABLA);
    log_trace(memoria_logger, "CANTIDAD_NIVELES: %d", CANTIDAD_NIVELES);
    log_trace(memoria_logger, "RETARDO_MEMORIA: %d", RETARDO_MEMORIA);
    log_trace(memoria_logger, "PATH_SWAPFILE: %s", PATH_SWAPFILE);
    log_trace(memoria_logger, "RETARDO_SWAP: %d", RETARDO_SWAP);
    log_trace(memoria_logger, "DUMP_PATH: %s", DUMP_PATH);
    log_trace(memoria_logger, "PATH_INSTRUCCIONES: %s", PATH_INSTRUCCIONES);
    
}

void inicializar_dump_path(){
    if (DUMP_PATH == NULL || strlen(DUMP_PATH) == 0) {
        log_error(memoria_logger, "El DUMP_PATH no está configurado correctamente.");
        exit(EXIT_FAILURE);
    }
    // Verificar que el directorio exista o crearlo
    struct stat st = {0};
    if (stat(DUMP_PATH, &st) == -1) {
        mkdir(DUMP_PATH, 0700);
        log_trace(memoria_logger, "Directorio de dump creado: %s", DUMP_PATH);
    } else {
        log_trace(memoria_logger, "Directorio de dump ya existe: %s", DUMP_PATH);
    }
}