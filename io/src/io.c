#include "../include/io.h"

int main(int argc, char* argv[]) {
    saludar("io");

    //Inicializar IO
    inicializar_io(argc, argv);

    // Conectarse como cliente con el Kernel
    fd_kernel = crear_conexion(IP_KERNEL, PUERTO_KERNEL, HANDSHAKE_IO);
    if (fd_kernel == -1) {
        log_error(io_logger, "No se pudo conectar con el Kernel");
        return EXIT_FAILURE; // Termina el programa
    }
    send(fd_kernel, &SIZE_ID_IO, sizeof(int), 0);
    send(fd_kernel, ID_IO, SIZE_ID_IO, 0);
    log_trace(io_logger,"Conexion exitosa con KERNEL");

    // Atender mensajes del kernel 
    pthread_t hilo_io;
    pthread_create(&hilo_io, NULL, (void*)atender_io_kernel, NULL);
    pthread_join(hilo_io, NULL);

    

    return 0;
}





