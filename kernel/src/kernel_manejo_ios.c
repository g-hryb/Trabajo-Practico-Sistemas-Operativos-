#include "../include/kernel_manejo_ios.h"
#include "../include/planificadores.h"

t_list* lista_procesos_en_cola;
pthread_mutex_t* mutex_procesos_en_cola;

t_list* lista_IO;
sem_t* binario_IO;


void inicializar_list_procesos_en_cola() {
    lista_procesos_en_cola = list_create();
    mutex_procesos_en_cola = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(mutex_procesos_en_cola, NULL);
    binario_IO = malloc(sizeof(sem_t));
    sem_init(binario_IO, 0, 0);
}

void inicialiar_list_io()
{
    lista_IO = list_create();
}

void inicializar_listas_io(){
    inicializar_list_procesos_en_cola();
    inicialiar_list_io();
}

void manejar_conexiones_ios() {
    log_trace(kernel_logger, "Esperando conexiones de IOs");
    while (1) {
        int fd_io = conectar_io();
        if (fd_io < 0) {
            log_error(kernel_logger, "Error al conectar con IO");
            continue; // Intentar nuevamente
        }
        int* fd_io_ptr = malloc(sizeof(int));
        *fd_io_ptr = fd_io; // Copia el valor actual de fd_io
        pthread_t hilo_io;
        pthread_create(&hilo_io, NULL, (void*)atender_kernel_io, (void*)fd_io_ptr);
        pthread_detach(hilo_io);
        sem_post(binario_IO); // Notifica que hay un IO conectado
    }
}


int conectar_io() {
    int size_id_io;
    log_trace(kernel_logger, "Esperando conexion de IO");
    int fd_io = esperar_cliente(fd_kernel_io, kernel_logger,HANDSHAKE_IO);
    if (recv(fd_io, &size_id_io, sizeof(int), MSG_WAITALL) <= 0) { //RECIBIR EL TAMAÑO DEL STRING
        log_error(kernel_logger, "Error al recibir el size ID del IO");
        close(fd_io);
        exit(EXIT_FAILURE);
    }
    log_trace(kernel_logger, "Tamaño del ID del IO recibido: %d", size_id_io);
    char id_io[size_id_io];
    if (recv(fd_io, id_io, size_id_io, MSG_WAITALL) <= 0) { //RECIBIR EL STRING
        log_error(kernel_logger, "Error al recibir el ID del IO");
        close(fd_io);
        exit(EXIT_FAILURE);
    }
    nodoIO* nuevo_io = crear_nodo_io(id_io, fd_io);
    list_add(lista_IO,nuevo_io);
    log_trace(kernel_logger, "Conexion exitosa con IO ID: %s", id_io);
    imprimir_lista_ios();
    return fd_io;
}

nodoIO* crear_nodo_io(char* id, int fd_io) {
    nodoIO* nuevo = (nodoIO*)malloc(sizeof(nodoIO));
    if (nuevo == NULL) {
        perror("Error al asignar memoria para el nodo IO");
        exit(EXIT_FAILURE);
    }
    nuevo->id_io = malloc(strlen(id) + 1);
    strcpy(nuevo->id_io, id);
    nuevo->fd_io = fd_io;
    nuevo->esta_ocupado = false;
    nuevo->proceso_en_ejecucion = 0;
    return nuevo;
}

void eliminar_nodo_io(int fd_io){
    pthread_mutex_lock(&mutex_ios);
    for(int i = 0; i < list_size(lista_IO); i++) {
        nodoIO* io = list_get(lista_IO, i);
        if (io->fd_io == fd_io) {
            if(!coincide_nombre_io(lista_IO, io->id_io)){ //Si no hay mas dispositivos con ese nombre, manda a los procesos en cola a EXIT
                EXIT_PID(io->proceso_en_ejecucion); //Manda al que esta ejecutando a EXIT
                for(int i = 0; i < list_size(lista_procesos_en_cola); i++) {
                    nodoProcesoEnCola* proceso = list_get(lista_procesos_en_cola, i);
                    if(strcmp(proceso->dispositivo_io,io->id_io) == 0){
                        log_trace(kernel_logger, "Eliminando el proceso %d de la cola de espera por IO %s", proceso->pid, io->id_io);
                        list_remove(lista_procesos_en_cola, i); // Elimina el nodo de procesos en cola
                        EXIT_PID(proceso->pid);
                        free(proceso->dispositivo_io);
                        free(proceso);
                        break; // Sale del bucle una vez que encuentra y elimina el proceso
                    }
                }
            }
            else{ //Si hay mas dispositivos con ese nombre, solo libera el proceso en ejecucion
                EXIT_PID(io->proceso_en_ejecucion);
            }
            list_remove_and_destroy_element(lista_IO, i, free); // Elimina el nodo y libera la memoria en la lista de IOs
            pthread_mutex_unlock(&mutex_ios);
            sem_post(binario_IO);
            return; // Nodo eliminado, salir de la función
        }
    }
    pthread_mutex_unlock(&mutex_ios);
}

void imprimir_lista_ios() {
    pthread_mutex_lock(&mutex_ios);
    if (list_is_empty(lista_IO)) {
        printf("La lista está vacía.\n");
        return;
    }
    printf("Lista de IOs:\n");
    for (int i = 0; i < list_size(lista_IO);i++)
    {
    nodoIO* nodoActual = list_get(lista_IO, i);
    printf("IO ID: %s | FD IO: %d |\n", nodoActual->id_io, nodoActual-> fd_io);
    }
    pthread_mutex_unlock(&mutex_ios);
}

bool existe_io(char* nombre) {
    log_trace(kernel_logger, "Verificando si el IO %s existe", nombre);
    pthread_mutex_lock(&mutex_ios);
    for(int i = 0; i < list_size(lista_IO); i++) {
        nodoIO* io = list_get(lista_IO, i);
        if (strcmp(io->id_io, nombre) == 0) {
            pthread_mutex_unlock(&mutex_ios);
            return true; // El IO existe
        }
    }
    pthread_mutex_unlock(&mutex_ios);
    return false; // El IO no existe
}

void agregar_a_cola_por_IO(uint32_t pid, char* dispositivo_io, int tiempo_milisegundos) {
    pthread_mutex_lock(mutex_procesos_en_cola);
    nodoProcesoEnCola* nuevo_nodo = malloc(sizeof(nodoProcesoEnCola));
    nuevo_nodo->pid = pid;
    nuevo_nodo->dispositivo_io = strdup(dispositivo_io); //chequear porque podria romper
    nuevo_nodo->tiempo_milisegundos = tiempo_milisegundos;
    
    list_add(lista_procesos_en_cola, nuevo_nodo);
    sem_post(binario_IO); // Notifica que hay un proceso en la cola de IO
    pthread_mutex_unlock(mutex_procesos_en_cola);
}

void atender_cola_espera(){
        while(1){
        sem_wait(binario_IO); // Espera a que haya un proceso en la cola de IO, un nuevo IO, o que libera un IO, o se desconecta un IO. 
        pthread_mutex_lock(mutex_procesos_en_cola);
        log_trace(kernel_logger, "Cantidad de procesos en cola: %d", list_size(lista_procesos_en_cola));
        for(int i = 0; i < list_size(lista_procesos_en_cola); i++) {
            log_trace(kernel_logger, "Entra al for");
            nodoProcesoEnCola* nodo_actual = list_get(lista_procesos_en_cola, i); // Obtiene el primer nodo sin eliminarlo
            log_trace(kernel_logger, "PID: %d, Dispositivo IO: %s, Tiempo: %d", nodo_actual->pid, nodo_actual->dispositivo_io, nodo_actual->tiempo_milisegundos);
            if (!existe_io(nodo_actual->dispositivo_io)) {
                EXIT_PID(nodo_actual->pid);
                log_trace(kernel_logger, "Eliminado el proceso de la cola");
                list_remove(lista_procesos_en_cola, i); // Elimina el nodo de la lista
                break;
            } else if (hay_alguno_libre_io(nodo_actual->dispositivo_io)) {
                log_trace(kernel_logger, "Hay IOs libres para el proceso %d en la cola de espera", nodo_actual->pid);
                t_buffer* buffer = crear_buffer();
                cargar_uint32_al_buffer(buffer, nodo_actual->pid);
                cargar_int_al_buffer(buffer, nodo_actual->tiempo_milisegundos);
                t_paquete* paquete = crear_paquete(INICIO_IO, buffer);
                int fd_io = ocupar_io(nodo_actual->dispositivo_io, nodo_actual->pid); //Busca un IO libre, lo marca ocupado y devuelve el fd
                enviar_paquete(paquete, fd_io);
                list_remove(lista_procesos_en_cola, i); // Elimina el nodo
                log_trace(kernel_logger, "Enviando solicitud de IO a %s para el proceso %d por %d milisegundos", nodo_actual->dispositivo_io, nodo_actual->pid, nodo_actual->tiempo_milisegundos);
                break;
            } else {
                log_trace(kernel_logger, "No hay IOs libres para el proceso %d en la cola de espera", nodo_actual->pid);
            }
        }
        log_trace(kernel_logger, "Funciona antes del mutex!!!");
        pthread_mutex_unlock(mutex_procesos_en_cola);
        log_trace(kernel_logger, "Cola de espera de IO atendida");
    }
}

bool hay_alguno_libre_io(char* nombreIO){
    log_trace(kernel_logger, "Verificando si algun IO %s está libre", nombreIO);
    bool respuesta = false;

    pthread_mutex_lock(&mutex_ios);
    t_list* lista_libres = list_filter(lista_IO, (void*)_esta_libre);
    respuesta = coincide_nombre_io(lista_libres, nombreIO);
    pthread_mutex_unlock(&mutex_ios);
    return respuesta;
}

bool _esta_libre(void* ptr) {
	nodoIO* modulo = (nodoIO*) ptr;
	return !(modulo->esta_ocupado);
}

bool coincide_nombre_io(t_list* lista_libres, char* nombreIO)
{
    bool coincide = false;
    if(list_is_empty(lista_libres)){
        log_trace(kernel_logger, "No hay IOs con ese nombre");
        return coincide; // No hay IOs libres
    }

    for(int i = 0 ; i< list_size(lista_libres);i++)
    {
        nodoIO* io = list_get(lista_libres, i);
        if( strcmp(io->id_io, nombreIO) == 0){
            coincide = !(io->esta_ocupado);
            break;
        }
    }
    return coincide;
}

int ocupar_io(char* nombre, uint32_t pid){
    pthread_mutex_lock(&mutex_ios);
    for(int i = 0; i < list_size(lista_IO); i++) {
        nodoIO* io = list_get(lista_IO, i);
        if (strcmp(io->id_io, nombre) == 0 && io->proceso_en_ejecucion == 0) {
            io->esta_ocupado = true; // Marca el IO como ocupado
            io->proceso_en_ejecucion = pid;
            pthread_mutex_unlock(&mutex_ios);
            return io->fd_io; // Devuelve el fd del IO
        }
    }
    pthread_mutex_unlock(&mutex_ios);
    return -1; // Si no se encontró un IO libre
}

void liberar_io(int fd_io){
    pthread_mutex_lock(&mutex_ios);
    for(int i = 0; i < list_size(lista_IO); i++) {
        nodoIO* io = list_get(lista_IO, i);
        if (io->fd_io == fd_io) {
            io->esta_ocupado = false; // Marca el IO como libre
            io->proceso_en_ejecucion = 0; // Limpia el proceso en ejecución
            pthread_mutex_unlock(&mutex_ios);
            sem_post(binario_IO); // Notifica que hay un IO libre
            return; // Nodo eliminado, salir de la función
        }
    }
    pthread_mutex_unlock(&mutex_ios);
}