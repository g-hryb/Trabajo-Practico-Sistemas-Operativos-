#include "../include/inicializar_cpu.h"
#include "../include/cpu_kernel_dispatch.h"
#include "../include/cpu_ciclo_instruccion.h"

void inicializar_cpu(int argc, char* argv[]){
    validarIDCPU(argc, argv);
    inicializar_config_cpu(argv);  
    inicializar_logger_cpu();
    loggear_config_cpu();
    inicializar_semaforos_pid_pc();
    inicializar_tlb();
    inicializar_cache();
    inicializar_semaforos_tlb();
    inicializar_semaforos_cache();
    inicializar_semaforo_interrupt();

}


void inicializar_config_cpu(char* argv[]) {

    char* path_base = "/home/utnso/tp-2025-1c-TP-Aprobado/cpu/";
    // Crear un string dinámico y copiar el path base
    char* path = string_new();
    string_append(&path, path_base);
    printf("Path del archivo de configuración: %s\n", path);
    // Concatenar el nombre del archivo de configuración
    char* archivo_config = argv[2];
    string_append(&path, archivo_config);
    
    cpu_config = config_create(path);
    if(cpu_config == NULL){
        perror("No se pudo crear el config");
        exit(EXIT_FAILURE);
    }
    
    // Cargar configuracion
    IP_MEMORIA = config_get_string_value(cpu_config, "IP_MEMORIA");
    PUERTO_MEMORIA = config_get_string_value(cpu_config, "PUERTO_MEMORIA");
    IP_KERNEL = config_get_string_value(cpu_config, "IP_KERNEL");
    PUERTO_KERNEL_DISPATCH = config_get_string_value(cpu_config, "PUERTO_KERNEL_DISPATCH");
    PUERTO_KERNEL_INTERRUPT = config_get_string_value(cpu_config, "PUERTO_KERNEL_INTERRUPT");
    ENTRADAS_TLB = config_get_int_value(cpu_config, "ENTRADAS_TLB");
    REEMPLAZO_TLB = config_get_string_value(cpu_config, "REEMPLAZO_TLB");
    ENTRADAS_CACHE = config_get_int_value(cpu_config, "ENTRADAS_CACHE");
    REEMPLAZO_CACHE = config_get_string_value(cpu_config, "REEMPLAZO_CACHE");
    RETARDO_CACHE = config_get_int_value(cpu_config, "RETARDO_CACHE");
    LOG_LEVEL = config_get_string_value(cpu_config, "LOG_LEVEL"); 
    free(path); // Liberar la memoria del path dinámico
}
    
void loggear_config_cpu(){
    log_trace(cpu_logger, "El ID del CPU es: %d\n", ID_CPU);
    log_trace(cpu_logger, "IP_MEMORIA: %s", IP_MEMORIA);
    log_trace(cpu_logger, "PUERTO_MEMORIA: %s", PUERTO_MEMORIA);
    log_trace(cpu_logger, "IP_KERNEL: %s", IP_KERNEL);
    log_trace(cpu_logger, "PUERTO_KERNEL_DISPATCH: %s", PUERTO_KERNEL_DISPATCH);
    log_trace(cpu_logger, "PUERTO_KERNEL_INTERRUPT: %s", PUERTO_KERNEL_INTERRUPT);
    log_trace(cpu_logger, "ENTRADAS_TLB: %d", ENTRADAS_TLB);
    log_trace(cpu_logger, "REEMPLAZO_TLB: %s", REEMPLAZO_TLB);
    log_trace(cpu_logger, "ENTRADAS_CACHE: %d", ENTRADAS_CACHE);
    log_trace(cpu_logger, "REEMPLAZO_CACHE: %s", REEMPLAZO_CACHE);
    log_trace(cpu_logger, "RETARDO_CACHE: %d", RETARDO_CACHE);
    log_trace(cpu_logger, "LOG_LEVEL: %s", LOG_LEVEL);
}

void inicializar_logger_cpu(){
    t_log_level log_level = log_level_from_string(LOG_LEVEL);
    cpu_logger = log_create("cpu.log", "CPU_LOG", 1, log_level);
    if(cpu_logger == NULL){
        perror("Algo raro paso con el log. No se pudo crear o encontrar el archivo");
        exit(EXIT_FAILURE);
    }
}

void validarIDCPU(int argc, char* argv[]){
    // Verificar que se haya pasado un argumento
    if (argc < 3) {
        printf("Tenes que proporcionar un número entero como argumento y el .config");
        exit(EXIT_FAILURE);
    }

    // Convertir el argumento a un entero
    ID_CPU = atoi(argv[1]);

    // Validar el número (opcional)
    if (ID_CPU <= 0) {
        printf("El número debe ser mayor que 0.");
        exit(EXIT_FAILURE);
    }
}