#include "../include/memoria.h"
#include "../include/manejo_memoria.h"
#include "../include/memoria_swap.h"


int main(int argc, char* argv[]) {
    // Verificar que se pasen los argumentos necesarios
    if (argc < 2) {
        printf("[ERROR] Debes proporcionar el nombre del archivo de config.\n");
        printf("Uso: %s <archivo_config>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Leer los argumentos
    archivo_config = argv[1];
    
    saludar("memoria");
    // Inicializar estructura de la Memoria
    inicializar_memoria();
    
    //Inicializar memoria como servidor
    fd_memoria = iniciar_servidor(PUERTO_ESCUCHA, memoria_logger, "MEMORIA");
    
    //Esperar conexion de Kernel
    log_trace(memoria_logger, "Esperando conexion de KERNEL");
    fd_kernel = esperar_cliente(fd_memoria, memoria_logger, HANDSHAKE_KERNEL);
    log_info(memoria_logger, "## Kernel Conectado - FD de %d", fd_kernel);
    //Atender los mensajes de Kernel
    pthread_t hilo_kernel;
    pthread_create(&hilo_kernel, NULL, (void*)atender_memoria_kernel, NULL);
    pthread_detach(hilo_kernel);
    
    //Atender conexiones de CPU
    //Esto en un hilo permanente, y cada vez que venga uno, lo ejecuta y crea otro hilo para el atender_kernel_io con su repectivo fd_io
    pthread_t hilo_conexiones_cpu;
    pthread_create(&hilo_conexiones_cpu, NULL, (void*)manejar_conexiones_cpus, NULL);
    pthread_join(hilo_conexiones_cpu, NULL);
    
 
    return 0;

}


