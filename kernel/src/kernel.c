#include "../include/kernel.h"

#define CANT_PROCESOS 5

int main(int argc, char* argv[]) {
    // Verificar que se pasen los argumentos necesarios
    if (argc < 4) {
        printf("[ERROR] Debes proporcionar el nombre del archivo de pseudocódigo y el tamaño del proceso.\n");
        printf("Uso: %s <archivo_pseudocodigo> <tamaño_proceso> <archivo_config>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Leer los argumentos
    archivo_pseudocodigo = argv[1];
    tamano_proceso = atoi(argv[2]);
    archivo_config = argv[3];

    // Validar el tamaño del proceso
    if (tamano_proceso < 0) {
        printf("[ERROR] El tamaño del proceso debe ser un número mayor o igual a 0.\n");
        exit(EXIT_FAILURE);
    }

    saludar("kernel");
    // Inicializar estructura del Kernel
    inicializar_kernel(); 
    inicializar_planificadores();
    inicializar_binario_respuestas();
    inicializar_listas_io(); // Inicializa las listas de IO y procesos en cola
    inicializar_lista_cpu();

    // Mostrar los argumentos leídos (opcional, para depuración)
    log_trace(kernel_logger,"Archivo de pseudocódigo: %s", archivo_pseudocodigo);
    log_trace(kernel_logger,"Tamaño del proceso: %d", tamano_proceso);

    // Conectarse como cliente con la Memoria
    fd_memoria = crear_conexion(IP_MEMORIA, PUERTO_MEMORIA, HANDSHAKE_KERNEL);
    if (fd_memoria == -1) {
        log_error(kernel_logger, "No se pudo conectar con la Memoria");
        return EXIT_FAILURE; // Termina el programa
    }
    log_trace(kernel_logger,"Conexion exitosa con MEMORIA");
    
    // Iniciar el Kernel como servidor CPU - Dispatch
    
    fd_kernel_dispatch = iniciar_servidor(PUERTO_ESCUCHA_DISPATCH, kernel_logger, "KERNEL_DISPATCH");

    // Iniciar el Kernel como servidor CPU - Interrupt
    
    fd_kernel_interrupt = iniciar_servidor(PUERTO_ESCUCHA_INTERRUPT, kernel_logger, "KERNEL_INTERRUPT");

    // Iniciar el Kernel como servidor  -IO
    fd_kernel_io = iniciar_servidor(PUERTO_ESCUCHA_IO, kernel_logger, "KERNEL_IO");


    // Conexion de CPU
    //Esto en un hilo permanente, y cada vez que venga uno, lo ejecuta y crea otro hilo para el atender_kernel_cpu con su repectivo fd_cpu dispatch e interrupt
    pthread_t hilo_conexiones_cpu;
    pthread_create(&hilo_conexiones_cpu, NULL, (void*)manejar_conexiones_cpus, NULL);
    pthread_detach(hilo_conexiones_cpu);

    // Conexion de IO
    //Esto en un hilo permanente, y cada vez que venga uno, lo ejecuta y crea otro hilo para el atender_kernel_io con su repectivo fd_io
    pthread_t hilo_conexiones_io;
    pthread_create(&hilo_conexiones_io, NULL, (void*)manejar_conexiones_ios, NULL);
    pthread_detach(hilo_conexiones_io);

    // Se encarga de gestionar la cola de procesos en espera de IO
    pthread_t hilo_manejo_io;
    pthread_create(&hilo_manejo_io, NULL, (void*)atender_cola_espera, NULL);
    pthread_detach(hilo_manejo_io);


    //Atender los mensajes de Memoria
    pthread_t hilo_memoria;
    pthread_create(&hilo_memoria, NULL, (void*)atender_kernel_memoria, NULL);
    
    
    
    printf("Ingresa enter para empezar\n");
    
    char* leido = readline("> ");

	while(strcmp(leido,"") != 0){
		leido = readline("> ");
	}
	
    free(leido);

    printf("Arrancando planificadores\n");
    
    t_pcb* pcb_chico1 = INIT_PROC(archivo_pseudocodigo, tamano_proceso);

    
    ejecutar_algoritmo_planificador_largo();
    ejecutar_algoritmo_planificador_corto();


    //Finalizar Kernel.
    //mostrar_metricas(pcb_chico1);
    pthread_join(hilo_memoria,NULL);
    return 0;
    }

