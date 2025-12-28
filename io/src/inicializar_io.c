
#include "../include/inicializar_io.h"


void inicializar_io(int argc, char* argv[]){
    inicializar_configs_io();
    inicializar_logs_io();
    imprimir_configs_io();
    inicializar_id(argc,argv);

}

void inicializar_configs_io(){

    io_config = config_create("/home/utnso/tp-2025-1c-TP-Aprobado/io/io.config");
        if(io_config == NULL){
            perror("No se pudo crear el config");
            exit(EXIT_FAILURE);
        }

    // Cargar configuracion
    IP_KERNEL = config_get_string_value(io_config, "IP_KERNEL");
    PUERTO_KERNEL = config_get_string_value(io_config, "PUERTO_KERNEL");
    LOG_LEVEL = config_get_string_value(io_config, "LOG_LEVEL");
}

void imprimir_configs_io(){

    // Imprimir configuracion

    log_trace(io_logger, "IP_KERNEL: %s", IP_KERNEL);
    log_trace(io_logger, "PUERTO_KERNEL: %s", PUERTO_KERNEL);
    log_trace(io_logger, "LOG_LEVEL: %s", LOG_LEVEL);

}

void inicializar_logs_io(){
    t_log_level log_level = log_level_from_string(LOG_LEVEL);
    io_logger = log_create("io.log", "IO_LOG", 1, log_level);
    if(io_logger == NULL){
        perror("Algo raro paso con el log. No se pudo crear o encontrar el archivo");
        exit(EXIT_FAILURE);
    }
}

void inicializar_id(int argc, char* argv[]){
    // Verificar que se haya pasado un argumento
    if (argc < 2) {
        printf("[ERROR] Tenes que proporcionar un string como argumento.\n");
        exit(EXIT_FAILURE);
    }

    // Obtener el string del argumento
    ID_IO = argv[1];
    SIZE_ID_IO = strlen(ID_IO)+1;
    log_trace(io_logger, "El ID proporcionado es: %s", ID_IO);
}