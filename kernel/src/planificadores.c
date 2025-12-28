
#include "../include/planificadores.h"
#include "../include/kernel_manejo_cpus.h"
#include "../include/kernel_memoria.h"
#include  "../include/kernel_pcb.h"

//1 cola, 1 mutex y 1 contador por cada estado

t_cola_proceso* cola_NEW;
pthread_mutex_t * mutex_NEW;
sem_t * contador_NEW;

t_cola_proceso* cola_READY;
pthread_mutex_t * mutex_READY;
sem_t * contador_READY;

t_cola_proceso* cola_EXEC;
pthread_mutex_t * mutex_EXEC;
sem_t * contador_EXEC;

t_cola_proceso* cola_BLOCKED;
pthread_mutex_t * mutex_BLOCKED;
sem_t * contador_BLOCKED;

t_cola_proceso* cola_EXIT;
pthread_mutex_t * mutex_EXIT;
sem_t * contador_EXIT;

t_cola_proceso* cola_SUSP_READY;
pthread_mutex_t * mutex_SUSP_READY;
sem_t * contador_SUSP_READY;

t_cola_proceso* cola_SUSP_BLOCKED;
pthread_mutex_t * mutex_SUSP_BLOCKED;
sem_t * contador_SUSP_BLOCKED;

sem_t* contador_replanificacion_largo; // NEW y EXIT o SUSPEND
sem_t* contador_replanificacion_corto; // READY y EXIT o BLOCKED
sem_t* contador_replanificacion_corto_desalojo; // READY y EXIT o BLOCKED

char* algoritmo;

pthread_mutex_t* mutex_pid;

uint32_t pid = 1;

pthread_mutex_t * mutex_socket_memoria;

void inicializar_cola_READY(){

    cola_READY = malloc(sizeof(t_cola_proceso));
    cola_READY->nombre_estado = READY;
    cola_READY->lista_procesos = list_create();  // Crea una lista vacía
}

void inicializar_cola_NEW(){
    cola_NEW = malloc(sizeof(t_cola_proceso));
    cola_NEW->nombre_estado = NEW;
    cola_NEW->lista_procesos = list_create();  // Crea una lista vacía
}

void inicializar_cola_EXEC(){
    cola_EXEC = malloc(sizeof(t_cola_proceso));
    cola_EXEC->nombre_estado = EXEC;
    cola_EXEC->lista_procesos = list_create();  // Crea una lista vacía
}

void inicializar_cola_BLOCKED(){
    cola_BLOCKED = malloc(sizeof(t_cola_proceso));
    cola_BLOCKED ->nombre_estado = BLOCKED;
    cola_BLOCKED ->lista_procesos = list_create();  // Crea una lista vacía
}

void inicializar_cola_EXIT(){
    cola_EXIT = malloc(sizeof(t_cola_proceso));
    cola_EXIT->nombre_estado = EXIT;
    cola_EXIT->lista_procesos = list_create();  // Crea una lista vacía
}

void inicializar_cola_SUSP_READY(){
    cola_SUSP_READY = malloc(sizeof(t_cola_proceso));
    cola_SUSP_READY->nombre_estado = SUSP_READY;
    cola_SUSP_READY->lista_procesos = list_create();  // Crea una lista vacía
}

void inicializar_cola_SUSP_BLOCKED(){
    cola_SUSP_BLOCKED= malloc(sizeof(t_cola_proceso));
    cola_SUSP_BLOCKED -> nombre_estado = SUSP_BLOCKED;
    cola_SUSP_BLOCKED->lista_procesos = list_create();  // Crea una lista vacía
}

void inicializar_colas(){
    inicializar_cola_NEW();
    inicializar_cola_READY();
    inicializar_cola_EXEC();
    inicializar_cola_BLOCKED();
    inicializar_cola_EXIT();
    inicializar_cola_SUSP_READY();
    inicializar_cola_SUSP_BLOCKED();
}

void inicializar_semaforos(){

    mutex_NEW = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(mutex_NEW, NULL);
    contador_NEW = malloc(sizeof(sem_t));
    sem_init(contador_NEW, 0, 0);

    mutex_READY = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(mutex_READY, NULL);
    contador_READY = malloc(sizeof(sem_t));
    sem_init(contador_READY, 0, 0);

    mutex_EXEC = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(mutex_EXEC, NULL);
    contador_EXEC = malloc(sizeof(sem_t));
    sem_init(contador_EXEC, 0, 0);
    
    mutex_BLOCKED = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(mutex_BLOCKED, NULL);
    contador_BLOCKED = malloc(sizeof(sem_t));
    sem_init(contador_BLOCKED, 0, 0);

    mutex_EXIT = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(mutex_EXIT, NULL);
    contador_EXIT = malloc(sizeof(sem_t));
    sem_init(contador_EXIT, 0, 0);

    mutex_SUSP_READY = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(mutex_SUSP_READY, NULL);
    contador_SUSP_READY = malloc(sizeof(sem_t));
    sem_init(contador_SUSP_READY, 0, 0);

    mutex_SUSP_BLOCKED = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(mutex_SUSP_BLOCKED, NULL);
    contador_SUSP_BLOCKED = malloc(sizeof(sem_t));
    sem_init(contador_SUSP_BLOCKED, 0, 0);

    mutex_pid = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(mutex_pid, NULL);

    contador_replanificacion_largo = malloc(sizeof(sem_t));
    sem_init(contador_replanificacion_largo, 0, 0);

    contador_replanificacion_corto = malloc(sizeof(sem_t));
    sem_init(contador_replanificacion_corto, 0, 0);

    contador_replanificacion_corto_desalojo = malloc(sizeof(sem_t));
    sem_init(contador_replanificacion_corto_desalojo, 0, 0);
}

void inicializar_planificadores(){
    inicializar_colas();
    inicializar_semaforos();
}

void agregar_a_NEW(t_pcb* pcb) {
    pthread_mutex_lock(mutex_NEW); // proteger acceso a la cola
    list_add(cola_NEW->lista_procesos, pcb);
    pthread_mutex_unlock(mutex_NEW);
    sem_post(contador_replanificacion_largo);
}

void agregar_a_READY(t_pcb* pcb) {
    pthread_mutex_lock(mutex_READY); // proteger acceso a la cola
    list_add(cola_READY->lista_procesos, pcb);
    pthread_mutex_unlock(mutex_READY);
    cambiar_estado(pcb, READY);
    sem_post(contador_replanificacion_corto);
    sem_post(contador_replanificacion_corto_desalojo);
}

void agregar_a_EXEC(t_pcb* pcb) {
    pthread_mutex_lock(mutex_EXEC); // proteger acceso a la cola
    list_add(cola_EXEC->lista_procesos, pcb);
    pthread_mutex_unlock(mutex_EXEC);
    cambiar_estado(pcb, EXEC);
}

void agregar_a_BLOCKED(t_pcb* pcb) {
    pthread_mutex_lock(mutex_BLOCKED); // proteger acceso a la cola
    list_add(cola_BLOCKED->lista_procesos, pcb);
    pthread_mutex_unlock(mutex_BLOCKED);
    cambiar_estado(pcb, BLOCKED);
    iniciar_timer_blocked(pcb);
    sem_post(contador_replanificacion_corto);
    sem_post(contador_replanificacion_corto_desalojo);
}

void agregar_a_EXIT(t_pcb* pcb) {
    pthread_mutex_lock(mutex_EXIT); // proteger acceso a la cola
    list_add(cola_EXIT->lista_procesos, pcb);
    pthread_mutex_unlock(mutex_EXIT);
    cambiar_estado(pcb, EXIT);
    log_info(kernel_logger, "## (<%d>) - Finaliza el proceso", pcb->pid);
    mostrar_metricas(pcb);
    sem_post(contador_replanificacion_largo);
    sem_post(contador_replanificacion_corto);
    sem_post(contador_replanificacion_corto_desalojo); //A CHEQUERAR, PUEDE SER QUE DESALOJE CUANDO NO QUEREMOS
}

void agregar_a_SUSP_READY(t_pcb* pcb) {
    pthread_mutex_lock(mutex_SUSP_READY); // proteger acceso a la cola
    list_add(cola_SUSP_READY->lista_procesos, pcb);
    pthread_mutex_unlock(mutex_SUSP_READY);
    cambiar_estado(pcb, SUSP_READY);
    sem_post(contador_replanificacion_largo);
}

void agregar_a_SUSP_BLOCKED(t_pcb* pcb) {
    pthread_mutex_lock(mutex_SUSP_BLOCKED); // proteger acceso a la cola
    list_add(cola_SUSP_BLOCKED->lista_procesos, pcb);
    pthread_mutex_unlock(mutex_SUSP_BLOCKED);
    cambiar_estado(pcb, SUSP_BLOCKED);
}

void ejecutar_algoritmo_planificador_largo(){
    if(ALGORITMO_INGRESO_A_READY){
        if (strcmp(ALGORITMO_INGRESO_A_READY, "FIFO") == 0) {
            log_trace(kernel_logger,"Soy Largo Plazo FIFO\n");
            pthread_t hilo_largo_plazo_fifo;
            pthread_create(&hilo_largo_plazo_fifo, NULL, (void*)largo_plazo_FIFO, NULL);
            pthread_detach(hilo_largo_plazo_fifo);
        } else if (strcmp(ALGORITMO_INGRESO_A_READY, "PMCP") == 0) {
            log_trace(kernel_logger,"Soy Largo Plazo PMCP\n");
            pthread_t hilo_largo_plazo_pcmp;
            pthread_create(&hilo_largo_plazo_pcmp, NULL, (void*)largo_plazo_PMCP, NULL);
            pthread_detach(hilo_largo_plazo_pcmp);
        } else {
            log_error(kernel_logger, "Algoritmo de corto plazo no reconocido: %s", ALGORITMO_INGRESO_A_READY);
        }
    }
}

void ejecutar_algoritmo_planificador_corto(){
    if (strcmp(ALGORITMO_CORTO_PLAZO, "FIFO") == 0) {
        log_trace(kernel_logger, "Soy Corto Plazo FIFO");
        corto_plazo_FIFO();
    } else if (strcmp(ALGORITMO_CORTO_PLAZO, "SJF") == 0) {
        corto_plazo_SJF();
    }  else if (strcmp(ALGORITMO_CORTO_PLAZO, "SRT") == 0) {
        // IMPLEMENTAR SJF
        log_trace(kernel_logger, "SOY SRT"); 
        corto_plazo_SRT();
    }
    else {
        log_error(kernel_logger, "Algoritmo de corto plazo no reconocido: %s", ALGORITMO_CORTO_PLAZO);
    }
}

bool esta_vacia(t_cola_proceso* cola, pthread_mutex_t* mutex) {
    pthread_mutex_lock(mutex); // Proteger acceso a la cola
    bool vacio = list_is_empty(cola->lista_procesos); 
    pthread_mutex_unlock(mutex); // Desbloquear la cola
    return vacio; //sigue ocurriendo condición de carrera
}

t_pcb* INIT_PROC(char* path, int tamanio) {
    log_trace(kernel_logger, "Iniciando proceso con PID: %d, Path: %s, Tamaño: %d", pid, path, tamanio);
    t_pcb* pcb = malloc(sizeof(t_pcb));
    pthread_mutex_lock(mutex_pid);
    pcb->pid = pid; //HABRIA QUE VER SI FUNCIONA 
    pid++; // Incrementar el PID para el siguiente proceso
    pthread_mutex_unlock(mutex_pid);
    pcb->programCounter = 0;
    pcb->path = strdup(path);
    pcb->tamanio = tamanio;
    pcb->estado_actual = NEW;
    //pcb->listaTCB = list_create();
    pcb->rafaga_anterior = -1;
    pcb->estimado_rafaga_anterior = ESTIMACION_INICIAL;
    pcb->estimacion_proxima_rafaga = ESTIMACION_INICIAL;
    inicializar_metricas(pcb);
    log_info(kernel_logger, "## (<%d>) Se crea el proceso - Estado: NEW", pcb->pid);
    if(no_hay_ningun_proceso() && intentar_enviar_proceso(pcb)){}
    else {
        agregar_a_NEW(pcb); // Agrega el PCB a la cola NEW
    }
    return pcb;
}

//ES PARA PARA PRUEBAS!!!
t_pcb* crear_proceso_prueba(int pid, char* path, int tamanio, int64_t rafaga_anterior, double estimado_rafaga_anterior) {
    log_trace(kernel_logger, "Iniciando proceso con PID: %d, Path: %s, Tamaño: %d", pid, path, tamanio);
    t_pcb* pcb = malloc(sizeof(t_pcb));
    pcb->pid = pid;
    pcb->programCounter = 0;
    pcb->path = strdup(path);
    pcb->tamanio = tamanio;
    pcb->estado_actual = NEW;
    //pcb->listaTCB = list_create();
    pcb->rafaga_anterior = rafaga_anterior;
    pcb->estimado_rafaga_anterior = estimado_rafaga_anterior;
    pcb->estimacion_proxima_rafaga = ESTIMACION_INICIAL;
    inicializar_metricas(pcb);
    return pcb;
}


bool no_hay_ningun_proceso(){
    return list_is_empty(cola_NEW->lista_procesos) && list_is_empty(cola_SUSP_READY->lista_procesos);
}

void largo_plazo_FIFO(){ //FALTA EL EXIT?S
        pthread_t hilo_largo_plazo_FIFO_NEW;
        pthread_create(&hilo_largo_plazo_FIFO_NEW, NULL, (void*)largo_plazo_FIFO_NEW, NULL);
        pthread_detach(hilo_largo_plazo_FIFO_NEW);             
        //sem_wait(contador_EXIT);
        // SUSP. READY TIENE MÁS PRIORIDAD        
}

void largo_plazo_FIFO_NEW(){
    while(1){
        sem_wait(contador_replanificacion_largo);
        if(list_is_empty(cola_SUSP_READY->lista_procesos)){
            if(list_is_empty(cola_NEW->lista_procesos)){
                continue; // Si no hay procesos en NEW, reintenta
            }
            pthread_mutex_lock(mutex_NEW); // Bloquea la cola NEW
            t_pcb *proceso = list_get(cola_NEW->lista_procesos, 0); // Obtiene el primer proceso sin eliminarlo
            pthread_mutex_unlock(mutex_NEW); // Desbloquea la cola NEW
            intentar_enviar_proceso(proceso);
        }
        else {
            pthread_mutex_lock(mutex_SUSP_READY); // Bloquea la cola SUSP_READY
            t_pcb *proceso = list_get(cola_SUSP_READY->lista_procesos, 0); // Obtiene el primer proceso sin eliminarlo
            pthread_mutex_unlock(mutex_SUSP_READY); // Desbloquea la cola SUSP_READY
            intentar_desuspender_proceso(proceso);
        }
    }
}

void largo_plazo_PMCP(){
        pthread_t hilo_largo_plazo_PMCP_NEW;
        pthread_create(&hilo_largo_plazo_PMCP_NEW, NULL, (void*)largo_plazo_PMCP_NEW, NULL);
        pthread_detach(hilo_largo_plazo_PMCP_NEW);        
        // SUSP. READY TIENE MÁS PRIORIDAD        
}

void largo_plazo_PMCP_NEW(){
    while(1){
        sem_wait(contador_replanificacion_largo);
        if(list_is_empty(cola_SUSP_READY->lista_procesos)){
            if(list_is_empty(cola_NEW->lista_procesos)){
                continue; // Si no hay procesos en NEW, reintenta
            }
            pthread_mutex_lock(mutex_NEW); // Bloquea la cola NEW
            list_sort(cola_NEW->lista_procesos, _tiene_tamanio_mas_chico);
            t_pcb *proceso = list_get(cola_NEW->lista_procesos, 0); // Obtiene el primer proceso sin eliminarlo
            pthread_mutex_unlock(mutex_NEW); // Desbloquea la cola NEW
            intentar_enviar_proceso(proceso);
        }
        else {
            pthread_mutex_lock(mutex_SUSP_READY); // Bloquea la cola SUSP_READY
            list_sort(cola_SUSP_READY->lista_procesos, _tiene_tamanio_mas_chico);
            t_pcb *proceso = list_get(cola_SUSP_READY->lista_procesos, 0); // Obtiene el primer proceso sin eliminarlo
            pthread_mutex_unlock(mutex_SUSP_READY); // Desbloquea la cola SUSP_READY
            intentar_desuspender_proceso(proceso);
        }
    }
}

bool _tiene_tamanio_mas_chico(void* a, void* b) {
    t_pcb* proceso_a = (t_pcb*) a;
    t_pcb* proceso_b = (t_pcb*) b;
    return proceso_a->tamanio <= proceso_b->tamanio;
}

bool intentar_enviar_proceso(t_pcb* proceso){
    t_buffer* buffer_peticion = crear_buffer();
    cargar_uint32_al_buffer(buffer_peticion, proceso->pid);
    cargar_string_al_buffer(buffer_peticion, proceso->path);
    cargar_int_al_buffer(buffer_peticion, proceso->tamanio);
    t_paquete* paquete_peticion = crear_paquete(CREAR_PROCESO_KM, buffer_peticion);
    enviar_paquete(paquete_peticion, fd_memoria); // Enviamos el paquete a memoria
    log_trace(kernel_logger, "Enviando solicitud de creación a memoria para PID: %d", proceso->pid);
    sem_wait(binario_respuesta_crear_proceso); //ESPERA EL POST DE KERNEL_MEMORIA EN UNA VARIABLE COMPARTIDA
    if (respuesta_crear_proceso) {
        pthread_mutex_lock(mutex_NEW);
        list_remove_element(cola_NEW->lista_procesos, proceso); // PUEDE TRAER PROBLEMA SI HUBIERA MUCHOS LARGOS PLAZOS EJECUTANDO (QUE CAMBIE EL 0)
        pthread_mutex_unlock(mutex_NEW);
        agregar_a_READY(proceso);
        respuesta_crear_proceso = false; // Reinicia la variable compartida
        return true; // Si se pudo crear el proceso, lo agrega a READY
    }
    else {
        return false; // Si no se pudo crear el proceso, no lo agrega a READY
    }
}

bool intentar_desuspender_proceso(t_pcb* proceso){
    t_buffer* buffer_peticion = crear_buffer();
    cargar_uint32_al_buffer(buffer_peticion, proceso->pid);
    t_paquete* paquete_peticion = crear_paquete(DESUSPENDER_PROCESO_KM, buffer_peticion);
    enviar_paquete(paquete_peticion, fd_memoria); // Enviamos el paquete a memoria
    log_trace(kernel_logger, "Enviando solicitud de desuspensión a memoria para PID: %d", proceso->pid);
    sem_wait(binario_respuesta_desuspender_proceso); //ESPERA EL POST DE KERNEL_MEMORIA EN UNA VARIABLE COMPARTIDA
    if (respuesta_desuspender_proceso) {
        pthread_mutex_lock(mutex_SUSP_READY);
        list_remove_element(cola_SUSP_READY->lista_procesos, proceso);
        pthread_mutex_unlock(mutex_SUSP_READY);
        agregar_a_READY(proceso);
        respuesta_desuspender_proceso = false; // Reinicia la variable compartida
        return true; // Si se pudo crear el proceso, lo agrega a READY
    }
    else {
        return false; // Si no se pudo crear el proceso, no lo agrega a READY
    }
}

void corto_plazo_FIFO(){
        // cada sem_wait tiene que ir con un hilo paralelo
        pthread_t hilo_corto_plazo_FIFO_READY;
        pthread_create(&hilo_corto_plazo_FIFO_READY, NULL, (void*)corto_plazo_FIFO_READY, NULL);
        pthread_detach(hilo_corto_plazo_FIFO_READY);   
        
        //sem_wait(contador_BLOCKED);

        //sem_wait(contador_EXEC);
}

void corto_plazo_FIFO_READY(){
    while(1){
        sem_wait(contador_replanificacion_corto); // Espera que haya un proceso en READY
        if(list_is_empty(cola_READY->lista_procesos)){
            continue; // Si no hay procesos en READY, reintenta
        }
        pthread_mutex_lock(mutex_READY); // Bloquea la cola READY
        t_pcb* proceso = list_get(cola_READY->lista_procesos, 0); // Obtiene el primer proceso sin eliminarlo
        pthread_mutex_unlock(mutex_READY); // Desbloquea la cola READY
        
        if(!(enviar_a_cpu(proceso))){ //SI NO HAY CPU DISPONIBLE
            continue; // Reintenta despues
        }
        log_trace(kernel_logger, "Pudo enviar a CPU");
        pthread_mutex_lock(mutex_READY);
        list_remove_element(cola_READY->lista_procesos, proceso); // PUEDE TRAER PROBLEMA SI HUBIERA MUCHOS LARGOS PLAZOS EJECUTANDO (QUE CAMBIE EL 0)
        pthread_mutex_unlock(mutex_READY);
        log_trace(kernel_logger, "Pudo sacar de READY");
        agregar_a_EXEC(proceso); // Mueve el proceso a EXEC
    }
}

/*Se elegirá el proceso que tenga la rafaga más corta. Su funcionamiento será como se explica en las clases. 
Para la primera ráfaga de todos los procesos se utilizará una estimación inicial definida por archivo de configuración.*/ 

void corto_plazo_SJF(){
        pthread_t hilo_corto_plazo_SJF_READY;
        pthread_create(&hilo_corto_plazo_SJF_READY, NULL, (void*)corto_plazo_SJF_READY, NULL);
        pthread_detach(hilo_corto_plazo_SJF_READY);   
}

void corto_plazo_SJF_READY(){
    while(1){
        sem_wait(contador_replanificacion_corto); // Espera que haya un proceso en READY
        if(list_is_empty(cola_READY->lista_procesos)){
            continue; // Si no hay procesos en READY, reintenta
        }
        pthread_mutex_lock(mutex_READY); // Bloquea la cola READY
        aplicar_SFJ(cola_READY->lista_procesos);
        list_sort(cola_READY->lista_procesos, tiene_rafaga_mas_corta); // Ordena la cola READY por estimación de ráfaga
        t_pcb* proceso = list_get(cola_READY->lista_procesos, 0); // Obtiene el primer proceso sin eliminarlo
        pthread_mutex_unlock(mutex_READY); // Desbloquea la cola READY
        
        if(!(enviar_a_cpu(proceso))){ //SI NO HAY CPU DISPONIBLE
            continue; // Reintenta despues
        }
        pthread_mutex_lock(mutex_READY);
        list_remove_element(cola_READY->lista_procesos, proceso); // PUEDE TRAER PROBLEMA SI HUBIERA MUCHOS LARGOS PLAZOS EJECUTANDO (QUE CAMBIE EL 0)
        pthread_mutex_unlock(mutex_READY);
        agregar_a_EXEC(proceso); // Mueve el proceso a EXEC
        }
}

void aplicar_SFJ(t_list* procesos){
    for(int i = 0; i < list_size(procesos); i++){
        t_pcb* proceso = list_get(procesos, i);
        calculo_SJF(proceso);
    }
}

bool tiene_rafaga_mas_corta(void* a, void* b){
    t_pcb* proceso_a = (t_pcb*) a;
    t_pcb* proceso_b = (t_pcb*) b;
    return proceso_a -> estimacion_proxima_rafaga <= proceso_b -> estimacion_proxima_rafaga;    
}

void calculo_SJF(t_pcb* pcb){
    
    if (pcb->rafaga_anterior == -1 )
    {
        pcb->estimacion_proxima_rafaga = ESTIMACION_INICIAL; 
    }
    else {
        double rafaga_anterior_double = (double) pcb->rafaga_anterior;
        pcb->estimacion_proxima_rafaga = (rafaga_anterior_double * ALFA) + (pcb->estimado_rafaga_anterior * (1 - ALFA));
    }
}

void corto_plazo_SRT(){
    pthread_t hilo_corto_plazo_SRT_READY;
    pthread_create(&hilo_corto_plazo_SRT_READY, NULL, (void*)corto_plazo_SRT_READY, NULL);
    pthread_detach(hilo_corto_plazo_SRT_READY);   
}

//Llega un proceso a ready mas corto que el de ejecución. 
//En vez de reorginzar la lista una vez que termina el de exec, lo hace apenas llega y lo desaloja si es mas chico.
void corto_plazo_SRT_READY(){
    while(1){
        sem_wait(contador_replanificacion_corto_desalojo); // Espera que haya un proceso en READY
        if(list_is_empty(cola_READY->lista_procesos)){
            continue; // Si no hay procesos en READY, reintenta
        }
        pthread_mutex_lock(mutex_READY); // Bloquea la cola READY
        aplicar_SFJ(cola_READY->lista_procesos);
        list_sort(cola_READY->lista_procesos, tiene_rafaga_mas_corta); // Ordena la cola READY por estimación de ráfaga
        t_pcb* proceso = list_get(cola_READY->lista_procesos, 0); // Obtiene el primer proceso sin eliminarlo
        pthread_mutex_unlock(mutex_READY); // Desbloquea la cola READY
        
        if(!(enviar_a_cpu_desalojo(proceso))){ //SI NO HAY CPU DISPONIBLE
            continue; // Reintenta despues
        }
        pthread_mutex_lock(mutex_READY);
        list_remove_element(cola_READY->lista_procesos, proceso); // PUEDE TRAER PROBLEMA SI HUBIERA MUCHOS LARGOS PLAZOS EJECUTANDO (QUE CAMBIE EL 0)
        pthread_mutex_unlock(mutex_READY);
        agregar_a_EXEC(proceso); // Mueve el proceso a EXEC
    }
}

bool enviar_a_cpu(t_pcb* proceso){
    //Prepara el paquete con el PID y PC del proceso
    log_trace(kernel_logger, "Enviando a CPU: PID=%d, PC=%d\n", proceso->pid, proceso->programCounter);
    int fd_dispatch = buscar_cpu_disponible(proceso); // Busca una CPU disponible
    if (fd_dispatch == -1) {
        log_debug(kernel_logger, "No hay CPUs disponibles para ejecutar el proceso");
        return false; // No se puede enviar el proceso
    }
    t_buffer* buffer_temp = crear_buffer();
    cargar_uint32_al_buffer(buffer_temp, proceso->pid);
    cargar_uint32_al_buffer(buffer_temp, proceso->programCounter);
    t_paquete* paquete_temp = crear_paquete(CONTEXTO, buffer_temp);
    log_trace(kernel_logger, "Enviando proceso a CPU con fd: %d", fd_dispatch);
    enviar_paquete(paquete_temp, fd_dispatch);
    proceso->cronometro = temporal_create();
    
    return true;


    //destruir_paquete(paquete_temp);
}

bool enviar_a_cpu_desalojo(t_pcb* proceso){
    //Prepara el paquete con el PID y PC del proceso
    
    int fd_dispatch = buscar_cpu_disponible(proceso); // Busca una CPU disponible
    if (fd_dispatch == -1) {
        log_debug(kernel_logger, "No hay CPUs disponibles para ejecutar el proceso");
        pthread_mutex_lock(&mutex_cpus);
        int fd_dispatch = buscar_cpu_disponible_desalojo(proceso); // Busca una CPU disponible para desalojo
        if(fd_dispatch == -1) {
            pthread_mutex_unlock(&mutex_cpus);
            log_debug(kernel_logger, "No hay CPUs disponibles para desalojo del proceso");
            return false; // No se puede enviar el proceso, no hay CPU para desalojar
        }
        log_debug(kernel_logger, "Se encontró una CPU disponible para desalojo: %d", fd_dispatch);
        t_buffer* buffer_temp = crear_buffer();
        cargar_uint32_al_buffer(buffer_temp, proceso->pid);
        cargar_uint32_al_buffer(buffer_temp, proceso->programCounter);
        t_paquete* paquete_temp = crear_paquete(CONTEXTO_INTERRUPCION, buffer_temp);
        enviar_paquete(paquete_temp, fd_dispatch);
        proceso->cronometro = temporal_create();
        pthread_mutex_unlock(&mutex_cpus);
        return true; // Se envió el proceso a la CPU disponible para desalojo
    }
    t_buffer* buffer_temp = crear_buffer();
    cargar_uint32_al_buffer(buffer_temp, proceso->pid);
    cargar_uint32_al_buffer(buffer_temp, proceso->programCounter);
    t_paquete* paquete_temp = crear_paquete(CONTEXTO, buffer_temp);
    enviar_paquete(paquete_temp, fd_dispatch);
    proceso->cronometro = temporal_create();
    pthread_mutex_unlock(&mutex_cpus);
    return true; // Se envió el proceso a la CPU disponible
}

void leer_cola_NEW() {
    pthread_mutex_lock(mutex_NEW);

    if (list_is_empty(cola_NEW->lista_procesos)) {
        log_trace(kernel_logger,"La cola NEW está vacía.\n");
    } else {
        log_trace(kernel_logger,"Contenido de la cola NEW:\n");
        for (int i = 0; i < list_size(cola_NEW->lista_procesos); i++) {
            t_pcb* proceso = list_get(cola_NEW->lista_procesos, i); // Obtener el proceso en la posición i
            log_trace(kernel_logger,"PID: %d, Tamaño: %d, Estado: %d\n", proceso->pid, proceso->tamanio, proceso->estado_actual);
        }
    }

    pthread_mutex_unlock(mutex_NEW);
}

void leer_cola_READY() {
    pthread_mutex_lock(mutex_READY); // Proteger acceso a la cola READY

    if (list_is_empty(cola_READY->lista_procesos)) {
        log_trace(kernel_logger,"La cola READY está vacía.\n");
    } else {
        log_trace(kernel_logger,"Contenido de la cola READY:\n");
        for (int i = 0; i < list_size(cola_READY->lista_procesos); i++) {
            t_pcb* proceso = list_get(cola_READY->lista_procesos, i); // Obtener el proceso en la posición i
            log_trace(kernel_logger,"PID: %d, Tamaño: %d, Estado: %d\n", proceso->pid, proceso->tamanio, proceso->estado_actual);
        }
    }

    pthread_mutex_unlock(mutex_READY); // Desbloquear acceso a la cola READY
}

void exec_a_blocked(uint32_t pid, uint32_t pc) {
    pthread_mutex_lock(mutex_EXEC); // Proteger acceso a la cola EXEC
    for (int i = 0; i < list_size(cola_EXEC->lista_procesos); i++) {
        t_pcb* proceso = list_get(cola_EXEC->lista_procesos, i);
        if (proceso->pid == pid) {
            proceso->programCounter = pc; // Actualizar el PC del proceso
            log_trace(kernel_logger, "Proceso con PID %d movido a BLOCKED desde EXEC. PC actualizado a %d.", pid, proceso->programCounter);
            list_remove(cola_EXEC->lista_procesos, i);
            temporal_stop(proceso->cronometro);
            proceso->rafaga_anterior = temporal_gettime(proceso->cronometro);
            temporal_destroy(proceso->cronometro);
            agregar_a_BLOCKED(proceso);
            log_trace(kernel_logger, "Proceso con PID %d bloqueado y movido a la cola BLOCKED.", pid);
            
            break;
        }
    }
    pthread_mutex_unlock(mutex_EXEC); // Desbloquear acceso a la cola EXEC
}

void blocked_a_ready(uint32_t pid) {
    pthread_mutex_lock(mutex_BLOCKED);
    for (int i = 0; i < list_size(cola_BLOCKED->lista_procesos); i++) {
        t_pcb* proceso = list_get(cola_BLOCKED->lista_procesos, i);
        if (proceso->pid == pid) {
            list_remove(cola_BLOCKED->lista_procesos, i);
            agregar_a_READY(proceso);
            break;
        }
    }
    pthread_mutex_unlock(mutex_BLOCKED);

    pthread_mutex_lock(mutex_SUSP_BLOCKED);
    for (int i = 0; i < list_size(cola_SUSP_BLOCKED->lista_procesos); i++) {
        t_pcb* proceso = list_get(cola_SUSP_BLOCKED->lista_procesos, i);
        if (proceso->pid == pid) {
            list_remove(cola_SUSP_BLOCKED->lista_procesos, i);
            agregar_a_SUSP_READY(proceso);
            break;
        }
    }
    pthread_mutex_unlock(mutex_SUSP_BLOCKED);
}

t_pcb* buscar_proceso_por_pid(uint32_t pid) {
    log_trace(kernel_logger, "Buscando proceso con PID: %d", pid);
    pthread_mutex_lock(mutex_EXEC);
    for (int i = 0; i < list_size(cola_EXEC->lista_procesos); i++) {
        t_pcb* proceso = list_get(cola_EXEC->lista_procesos, i);
        if (proceso->pid == pid) {
            list_remove(cola_EXEC->lista_procesos, i);
            pthread_mutex_unlock(mutex_EXEC);
            return proceso; // Retorna el proceso encontrado y lo elimina de la cola EXEC
            break;
        }
    }
    pthread_mutex_unlock(mutex_EXEC);

    pthread_mutex_lock(mutex_BLOCKED);
    for (int i = 0; i < list_size(cola_BLOCKED->lista_procesos); i++) {
        t_pcb* proceso = list_get(cola_BLOCKED->lista_procesos, i);
        if (proceso->pid == pid) {
            list_remove(cola_BLOCKED->lista_procesos, i);
            pthread_mutex_unlock(mutex_BLOCKED);
            return proceso; // Retorna el proceso encontrado y lo elimina de la cola BLOCKED
            break;
        }
    }
    pthread_mutex_unlock(mutex_BLOCKED);

    pthread_mutex_lock(mutex_SUSP_BLOCKED);
    for (int i = 0; i < list_size(cola_SUSP_BLOCKED->lista_procesos); i++) {
        t_pcb* proceso = list_get(cola_SUSP_BLOCKED->lista_procesos, i);
        if (proceso->pid == pid) {
            list_remove(cola_SUSP_BLOCKED->lista_procesos, i);
            pthread_mutex_unlock(mutex_SUSP_BLOCKED);
            return proceso; // Retorna el proceso encontrado y lo elimina de la cola SUSP_BLOCKED
            break;
        }
    }
    pthread_mutex_unlock(mutex_SUSP_BLOCKED);
    log_trace(kernel_logger, "No se encontró el proceso con PID %d en ninguna cola.", pid);
    return NULL; // Retorna NULL si no se encontró el proceso en ninguna cola
}

void EXIT_PID(uint32_t pid){
    t_buffer* buffer_exit = crear_buffer();
    cargar_uint32_al_buffer(buffer_exit, pid);
    t_paquete* paquete_exit = crear_paquete(FINALIZAR_PROCESO_KM, buffer_exit);
    enviar_paquete(paquete_exit, fd_memoria);
    log_debug(kernel_logger, "Enviando solicitud de finalización a memoria para PID: %d", pid);
    sem_wait(binario_respuesta_eliminar_proceso);
    if(respuesta_eliminar_proceso){
        t_pcb* proceso = buscar_proceso_por_pid(pid);
        if (proceso == NULL) {
            log_error(kernel_logger, "No se pudo encontrar el proceso con PID %d para finalizarlo.", pid);
            return; // Si no se encuentra el proceso, no se puede finalizar
        }
        
        liberar_cpu(pid);
        agregar_a_EXIT(proceso);
    
        // Liberar recursos
        free(proceso->path);  
        free(proceso);
        respuesta_eliminar_proceso = false; // Reiniciar la variable de respuesta
        log_trace(kernel_logger, "Proceso con PID %d finalizado y recursos liberados.", pid);
    }
    else {
        log_error(kernel_logger, "No se pudo finalizar el proceso con PID %d en memoria.", pid);
    }
}

void manejar_tiempo_blocked(t_pcb* proceso) {
    log_trace(kernel_logger, "Iniciando timer para proceso con PID %d en estado BLOCKED.", proceso->pid);
    usleep(TIEMPO_SUSPENSION * 1000); // Espera el tiempo configurado (en milisegundos)

    pthread_mutex_lock(mutex_BLOCKED);
    bool sigue_en_blocked = false;
    for (int i = 0; i < list_size(cola_BLOCKED->lista_procesos); i++) { //Verifica que siga en blocked
        t_pcb* p = list_get(cola_BLOCKED->lista_procesos, i);
        if (p->pid == proceso->pid) {
            sigue_en_blocked = true;
            list_remove(cola_BLOCKED->lista_procesos, i);
            break;
        }
    }
    pthread_mutex_unlock(mutex_BLOCKED);

    if (sigue_en_blocked) { // Si lo esta, lo mueve a SUSP BLOCKED
        log_trace(kernel_logger, "Proceso con PID %d movido a SUSP. BLOCKED.", proceso->pid);
        agregar_a_SUSP_BLOCKED(proceso);
        // Informar a memoria que debe mover el proceso a swap
        t_buffer* buffer = crear_buffer();
        cargar_uint32_al_buffer(buffer, proceso->pid);
        t_paquete* paquete = crear_paquete(SUSPENDER_PROCESO_KM, buffer);
        enviar_paquete(paquete, fd_memoria);

        // Verificar si hay procesos en NEW o SUSP. READY que puedan entrar
        sem_post(contador_replanificacion_largo);
    }
    //Si no lo esta, no hace nada
}

void iniciar_timer_blocked(t_pcb* proceso) {
    pthread_t hilo_timer;
    pthread_create(&hilo_timer, NULL, (void*)manejar_tiempo_blocked, proceso);
    pthread_detach(hilo_timer); // Desvincula el hilo para que se limpie automáticamente
}