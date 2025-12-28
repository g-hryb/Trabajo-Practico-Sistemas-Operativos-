#include "../include/kernel_manejo_cpus.h"

t_list* lista_CPU;
sem_t* binario_desalojado;

void inicializar_lista_cpu(){
    lista_CPU = list_create();
    binario_desalojado = malloc(sizeof(sem_t));
    sem_init(binario_desalojado, 0, 0);
};

void manejar_conexiones_cpus() {
    log_trace(kernel_logger, "Esperando conexiones de CPUs");
    while (1) {
        fds_cpu_t fds;
        fds = conectar_cpu();
        int* fd_cpu_dispatch_ptr = malloc(sizeof(int));
        *fd_cpu_dispatch_ptr = fds.dispatch;
        int* fd_cpu_interrupt_ptr = malloc(sizeof(int));
        *fd_cpu_interrupt_ptr = fds.interrupt;
        //Atender los mensajes de CPU - Dispatch
        pthread_t hilo_cpu_dispatch;
        pthread_create(&hilo_cpu_dispatch, NULL, (void*)atender_kernel_cpu_dispatch, (void*)fd_cpu_dispatch_ptr);
        pthread_detach(hilo_cpu_dispatch);
        //Atender los mensajes de CPU - Interrupt
        pthread_t hilo_cpu_interrupt;
        pthread_create(&hilo_cpu_interrupt, NULL, (void*)atender_kernel_cpu_interrupt, (void*)fd_cpu_interrupt_ptr);
        pthread_detach(hilo_cpu_interrupt);
    }
}

fds_cpu_t conectar_cpu() {
    int id_cpu_dispatch;
    int id_cpu_interrupt;
    // Esperar conexión de CPU - Dispatch

    log_trace(kernel_logger, "Esperando conexion de CPU_DISPATCH");
    int fd_cpu_dispatch = esperar_cliente(fd_kernel_dispatch, kernel_logger, HANDSHAKE_CPU_DISPATCH);
    recv(fd_cpu_dispatch, &id_cpu_dispatch, sizeof(int), MSG_WAITALL);

    // Esperar conexión de CPU - Interrupt

    log_trace(kernel_logger, "Esperando conexion de CPU_INTERRUPT");
    int fd_cpu_interrupt = esperar_cliente(fd_kernel_interrupt, kernel_logger, HANDSHAKE_CPU_INTERRUPT);
    recv(fd_cpu_interrupt, &id_cpu_interrupt, sizeof(int), MSG_WAITALL);

    if(id_cpu_dispatch != id_cpu_interrupt){
        log_error(kernel_logger, "Los ID de CPU son diferentes");
        exit(EXIT_FAILURE); // Termina el programa
    }
    log_trace(kernel_logger, "Vinculación exitosa con el CPU ID: %d", id_cpu_dispatch);
    nodoCPU* nuevoCPU = crear_nodo_cpu(id_cpu_dispatch, fd_cpu_dispatch, fd_cpu_interrupt);
    list_add(lista_CPU, nuevoCPU);
    imprimir_lista_cpus();
   
    fds_cpu_t fds;
    fds.dispatch = fd_cpu_dispatch;
    fds.interrupt = fd_cpu_interrupt;
    return fds;
}

nodoCPU* crear_nodo_cpu(int id, int fd_disp, int fd_int) {
    nodoCPU* nuevo = (nodoCPU*)malloc(sizeof(nodoCPU));
    if (nuevo == NULL) {
        perror("Error al asignar memoria para el nodo CPU");
        exit(EXIT_FAILURE);
    }
    
    nuevo->id_cpu = id;
    nuevo->fd_dispatch = fd_disp;
    nuevo->fd_interrupt = fd_int;
    nuevo->estado = DISPONIBLE;
    nuevo->proceso_running = NULL;
    
    return nuevo;
}

/*
void insertar_cpu(nodoCPU* nuevoCPU){
    pthread_mutex_lock(&mutex_cpus);
    if (cabezaCPU == NULL) {
        cabezaCPU = nuevoCPU;
    } else {
        nodoCPU* temp = cabezaCPU;
        while (temp->siguiente != NULL) {
            temp = temp->siguiente;
        }
        temp->siguiente = nuevoCPU;
    }
    pthread_mutex_unlock(&mutex_cpus); 
}


void eliminar_nodo_cpu(int id_eliminar) {
    pthread_mutex_lock(&mutex_cpus);
    if (cabezaCPU == NULL) return;
    
    nodoCPU* actual = cabezaCPU;
    nodoCPU* anterior = NULL;
    
    // Caso especial: eliminar el primer nodo
    if (actual != NULL && actual->id_cpu == id_eliminar) {
        cabezaCPU = actual->siguiente;
        free(actual);
        return;
    }
    
    // Buscar el nodo a eliminar
    while (actual != NULL && actual->id_cpu != id_eliminar) {
        anterior = actual;
        actual = actual->siguiente;
    }
    
    // Si no se encontró
    if (actual == NULL) return;
    
    // Desenlazar el nodo de la lista
    anterior->siguiente = actual->siguiente;
    free(actual);
    pthread_mutex_unlock(&mutex_cpus);
}*/

void imprimir_lista_cpus() {
    pthread_mutex_lock(&mutex_cpus);
    if (list_is_empty(lista_CPU)) {
        printf("La lista está vacía.\n");
        return;
    }
    printf("Lista de CPUs:\n");
    for(int i = 0; i < list_size(lista_CPU); i++) {
        nodoCPU* actual = list_get(lista_CPU, i);
        printf("CPU ID: %d | Estado: %s | FD Dispatch: %d | FD Interrupt: %d\n",
               actual->id_cpu,
               actual->estado == DISPONIBLE ? "DISPONIBLE" : "OCUPADO",
               actual->fd_dispatch,
               actual->fd_interrupt);
    }
    pthread_mutex_unlock(&mutex_cpus); 
}

int buscar_cpu_disponible(t_pcb* proceso) {
    log_trace(kernel_logger, "Antes del mutex de buscar_cpu_disponible");
    pthread_mutex_lock(&mutex_cpus);
    //nodoCPU* actual = cabezaCPU;
    for(int i = 0; i < list_size(lista_CPU); i++) {
        nodoCPU* actual = list_get(lista_CPU, i);
        if (actual->estado == DISPONIBLE) {
            actual->estado = OCUPADO; // Marca la CPU como ocupada
            actual->proceso_running = proceso; // Asigna el proceso a la CPU
            pthread_mutex_unlock(&mutex_cpus);
            log_trace(kernel_logger, "CPU ID: %d asignada al proceso PID: %d", actual->id_cpu, proceso->pid);
            return actual->fd_dispatch; // Retorna el File Descriptor Dispatch de la CPU disponible
        }
    }
    pthread_mutex_unlock(&mutex_cpus);
    log_trace(kernel_logger, "No hay CPU disponible para el proceso PID: %d", proceso->pid);
    return -1; // No hay CPU disponible
}

int buscar_cpu_disponible_desalojo(t_pcb* proceso) {
    list_sort(lista_CPU, _tiene_estimacion_mas_grande); // Ordena la lista de CPUs por estimación de ráfaga
    nodoCPU* actual = list_get(lista_CPU, 0);
    if (actual->proceso_running->estimacion_proxima_rafaga > proceso->estimacion_proxima_rafaga) {
        desalojar_cpu(actual); // Desalojar la CPU si la estimación del proceso en ejecución es mayor
        log_debug(kernel_logger, "Esperando confirmacion de cpu de desalojo");
        sem_wait(binario_desalojado);
        actual->estado = OCUPADO; // Marca la CPU como ocupada
        actual->proceso_running = proceso; // Asigna el proceso a la CPU
        log_trace(kernel_logger, "CPU ID: %d asignada al proceso PID: %d", actual->id_cpu, proceso->pid);
        return actual->fd_dispatch; // Retorna el File Descriptor Dispatch de la CPU a desalojar
    }
    return -1; // No hay CPU disponible
}

bool _tiene_estimacion_mas_grande(void* a, void* b) {
    t_pcb* proceso_a = (t_pcb*) a;
    t_pcb* proceso_b = (t_pcb*) b;
    return proceso_a->estimacion_proxima_rafaga > proceso_b->estimacion_proxima_rafaga;
}

void desalojar_cpu(nodoCPU* cpu) {
    t_buffer* buffer_desalojo = crear_buffer();
    cargar_uint32_al_buffer(buffer_desalojo, cpu->proceso_running->pid);
    t_paquete* paquete_desalojo = crear_paquete(INTERRUPT, buffer_desalojo);
    enviar_paquete(paquete_desalojo, cpu->fd_interrupt); // Enviar el paquete de desalojo a la CPU
    log_debug(kernel_logger, "Le mando el paquete de interrupcion a cpu");
}

void liberar_cpu(uint32_t pid_buscar){
    log_debug(kernel_logger, "Entro a liberar cpu");
    pthread_mutex_lock(&mutex_cpus);
    log_debug(kernel_logger, "pase el mutex de liberar cpu");
    nodoCPU* actual;
    for(int i = 0; i < list_size(lista_CPU); i++) {
        actual = list_get(lista_CPU, i);
        if (actual->proceso_running != NULL && actual->proceso_running->pid == pid_buscar) {
            actual->estado = DISPONIBLE; // Marca la CPU como disponible
            actual->proceso_running = NULL; // Limpia el proceso en ejecución
            log_trace(kernel_logger, "CPU ID: %d liberada del proceso PID: %d", actual->id_cpu, pid_buscar);
            break;
        }
    }
    pthread_mutex_unlock(&mutex_cpus);
}