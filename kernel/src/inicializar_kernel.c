#include "../include/inicializar_kernel.h"


void inicializar_kernel(){
    inicializar_config_kernel();
    inicializar_logger_kernel();
    loggear_config_kernel();
}


void inicializar_config_kernel(){
    char* path_base = "/home/utnso/tp-2025-1c-TP-Aprobado/kernel/";
    // Crear un string dinámico y copiar el path base
    char* path = string_new();
    string_append(&path, path_base);
    printf("Path del archivo de configuración: %s\n", path);
    // Concatenar el nombre del archivo de configuración
    string_append(&path, archivo_config);
    printf("Path del archivo de configuración: %s\n", path);
    kernel_config = config_create(path);
    if(kernel_config == NULL){
        perror("No se pudo crear el config");
        exit(EXIT_FAILURE);
    }
    
    IP_MEMORIA = config_get_string_value(kernel_config, "IP_MEMORIA");
    PUERTO_MEMORIA = config_get_string_value(kernel_config, "PUERTO_MEMORIA");
    PUERTO_ESCUCHA_DISPATCH = config_get_string_value(kernel_config, "PUERTO_ESCUCHA_DISPATCH");
    PUERTO_ESCUCHA_INTERRUPT = config_get_string_value(kernel_config, "PUERTO_ESCUCHA_INTERRUPT");
    PUERTO_ESCUCHA_IO = config_get_string_value(kernel_config, "PUERTO_ESCUCHA_IO");
    ALGORITMO_CORTO_PLAZO = config_get_string_value(kernel_config, "ALGORITMO_CORTO_PLAZO");
    ALGORITMO_INGRESO_A_READY = config_get_string_value(kernel_config, "ALGORITMO_INGRESO_A_READY");
    ALFA = config_get_double_value(kernel_config, "ALFA");
    ESTIMACION_INICIAL = config_get_double_value(kernel_config, "ESTIMACION_INICIAL");
    TIEMPO_SUSPENSION = config_get_int_value(kernel_config, "TIEMPO_SUSPENSION");
    LOG_LEVEL = config_get_string_value(kernel_config, "LOG_LEVEL");
    free(path);
}

void inicializar_logger_kernel(){
    t_log_level log_level = log_level_from_string(LOG_LEVEL);
    kernel_logger = log_create("kernel.log", "KERNEL_LOG", 1, log_level);
    if(kernel_logger == NULL){
        perror("Algo raro paso con el log. No se pudo crear o encontrar el archivo");
        exit(EXIT_FAILURE);
    }
}
void loggear_config_kernel(){
    log_trace(kernel_logger, "IP_MEMORIA: %s", IP_MEMORIA);
    log_trace(kernel_logger, "PUERTO_MEMORIA: %s", PUERTO_MEMORIA);
    log_trace(kernel_logger, "PUERTO_ESCUCHA_DISPATCH: %s", PUERTO_ESCUCHA_DISPATCH);
    log_trace(kernel_logger, "PUERTO_ESCUCHA_INTERRUPT: %s", PUERTO_ESCUCHA_INTERRUPT);
    log_trace(kernel_logger, "PUERTO_ESCUCHA_IO: %s", PUERTO_ESCUCHA_IO);
    log_trace(kernel_logger, "ALGORITMO_CORTO_PLAZO: %s", ALGORITMO_CORTO_PLAZO);
    log_trace(kernel_logger, "ALGORITMO_INGRESO_A_READY: %s", ALGORITMO_INGRESO_A_READY);
    log_trace(kernel_logger, "ALFA: %f", ALFA);
    log_trace(kernel_logger, "ESTIMACION_INICIAL: %f", ESTIMACION_INICIAL);
    log_trace(kernel_logger, "TIEMPO_SUSPENSION: %d", TIEMPO_SUSPENSION);
    log_trace(kernel_logger, "LOG_LEVEL: %s", LOG_LEVEL);

}