#include "../include/cpu.h"

t_tlb* tlb = NULL;
t_cache* cache = NULL;
bool interrupcion_pendiente = false;

int entradasPorTabla=0;
int cantidadDeNiveles=0;
int tamanioPagina=0; 
int marco_proceso=0;
void* datos_memoria= NULL; 
char*ultimo_contenido_agregado=NULL;
size_t tamanio_contenido=0;

sem_t* sem_instruccion;
sem_t* sem_pagina;
sem_t* sem_marco;

extern pthread_mutex_t * mutex_tlb;
extern pthread_mutex_t * mutex_cache;


int main(int argc, char* argv[]) {
    
    // Inicializar estructura de la CPU
    inicializar_cpu(argc, argv);
    sem_instruccion = malloc(sizeof(sem_t));
    log_trace(cpu_logger, "Inicializando semáforo de instrucción");
    sem_instruccion = malloc(sizeof(sem_t));
    sem_init(sem_instruccion, 0, 0);
    sem_pagina = malloc(sizeof(sem_t));
    sem_init(sem_pagina, 0, 0);
    sem_marco = malloc(sizeof(sem_t));
    sem_init(sem_marco, 0, 0);

 

    // Conectarse como cliente con la Memoria
    fd_memoria = crear_conexion(IP_MEMORIA, PUERTO_MEMORIA, HANDSHAKE_CPU);
    if (fd_memoria == -1) {
        log_error(cpu_logger, "No se pudo conectar con la Memoria");
        return EXIT_FAILURE; // Termina el programa
    }
    send(fd_memoria, &ID_CPU, sizeof(int), 0);
    log_trace(cpu_logger, "Conexion exitosa con MEMORIA");


    fd_kernel_dispatch = crear_conexion(IP_KERNEL, PUERTO_KERNEL_DISPATCH, HANDSHAKE_CPU_DISPATCH);
    if (fd_kernel_dispatch == -1) {
        log_error(cpu_logger, "No se pudo conectar con el Kernel - Dispatch");
        return EXIT_FAILURE; // Termina el programa
    }
    send(fd_kernel_dispatch, &ID_CPU, sizeof(int), 0);
    log_trace(cpu_logger, "Conexion exitosa con KERNEL_DISPATCH");

    // Conectarse como cliente con el Kernel - Interrupt
    fd_kernel_interrupt = crear_conexion(IP_KERNEL, PUERTO_KERNEL_INTERRUPT, HANDSHAKE_CPU_INTERRUPT);
    if (fd_kernel_interrupt == -1) {
        log_error(cpu_logger, "No se pudo conectar con el Kernel - Interrupt");
        return EXIT_FAILURE; // Termina el programa
    }
    send(fd_kernel_interrupt, &ID_CPU, sizeof(int), 0);
    log_trace(cpu_logger, "Conexion exitosa con KERNEL_INTERRUPT");
    
    // Atender los mensajes de memoria
    pthread_t hilo_memoria;
    pthread_create(&hilo_memoria, NULL, (void*)atender_cpu_memoria, NULL);
    pthread_detach(hilo_memoria);
    
    // Atender los mensajes del Kernel -Dispatch
    pthread_t hilo_kernel_dispatch;
    pthread_create(&hilo_kernel_dispatch, NULL, (void*)atender_cpu_kernel_dispatch, NULL);
    pthread_detach(hilo_kernel_dispatch);
    
    
    // Atender los mensajes del Kernel -Interrupt 
    pthread_t hilo_kernel_interrupt;
    pthread_create(&hilo_kernel_interrupt, NULL, (void*)atender_cpu_kernel_interrupt, NULL);
    
    
    while(!fin_proceso){
        log_trace(cpu_logger, "Empezando ciclo de instrucción");
        if(esperar_contexto){
            log_trace(cpu_logger, "Esperando contexto...");
            sem_wait(sem_contexto);
            esperar_contexto = false;
        }
        char* instruccion;
        char* instruccion2 = fetch(instruccion);
        decode(instruccion2);
        check_interrupt();
        //sleep(1);
    }


    // Finalizar memoria
    pthread_join(hilo_kernel_interrupt, NULL);
    return 0;
}
