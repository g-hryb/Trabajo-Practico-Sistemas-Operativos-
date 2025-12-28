#include "../include/memoria_manejo_cpus.h"

void manejar_conexiones_cpus() {
    log_trace(memoria_logger, "Esperando conexiones de CPUs");
    while (1) {
        int fd_cpu = conectar_cpu();
        if (fd_cpu < 0) {
            log_error(memoria_logger, "Error al conectar con CPU");
            continue; // Intentar nuevamente
        }
        int* fd_cpu_ptr = malloc(sizeof(int));
        *fd_cpu_ptr = fd_cpu; // Copia el valor actual de fd_cpu
        pthread_t hilo_io;
        pthread_create(&hilo_io, NULL, (void*)atender_memoria_cpu, (void*)fd_cpu_ptr);
        pthread_detach(hilo_io);
    }
}

int conectar_cpu() {
    int id_cpu;
    log_trace(memoria_logger, "Esperando conexion de CPU");
    int fd_cpu = esperar_cliente(fd_memoria, memoria_logger,HANDSHAKE_CPU);
    if (recv(fd_cpu, &id_cpu, sizeof(int), MSG_WAITALL) <= 0) { //RECIBIR EL ID DE CPU
        log_error(memoria_logger, "Error al recibir el ID del CPU");
        close(fd_cpu);
        exit(EXIT_FAILURE);
    }
    log_trace(memoria_logger, "Conexion exitosa con CPU ID: %d", id_cpu);
    return fd_cpu;
}