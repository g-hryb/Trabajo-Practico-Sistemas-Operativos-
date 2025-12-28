#include "../include/memoria_finalizar.h"
#include "../include/memoria_swap.h"
#include "../include/memoria_proceso.h"

bool eliminar_proceso_en_memoria(uint32_t pid){
    pthread_mutex_lock(mutex_lista_procesos);
    for(int i = 0; i < list_size(lista_procesos); i++) {
        proceso_t* proceso = list_get(lista_procesos, i); // obtiene sin eliminar
        if(proceso->pid == pid) {
            //log_trace(memoria_logger, "Eliminando proceso con PID %d de la lista.", pid);
            list_remove(lista_procesos, i); // elimina el proceso de la lista
            log_trace(memoria_logger, "Eliminando proceso con PID %d de la memoria.", pid);
            destruir_proceso(proceso);  //libera la memoria del proceso
            pthread_mutex_unlock(mutex_lista_procesos);
            return true;
        }
    }
    pthread_mutex_unlock(mutex_lista_procesos);
    log_trace(memoria_logger, "No se encontró el proceso con PID %d en la lista de procesos activos.", pid);
    return false;
}

bool eliminar_proceso_en_swap(uint32_t pid){
    pthread_mutex_lock(mutex_procesos_en_swap);
    for(int i = 0; i < list_size(procesos_en_swap); i++) {
        proceso_t* proceso = list_get(procesos_en_swap, i); // obtiene sin eliminar
        if(proceso->pid == pid) {
            list_remove(procesos_en_swap, i); // elimina el proceso de la lista
            destruir_proceso(proceso); // libera la memoria del proceso
            pthread_mutex_unlock(mutex_procesos_en_swap);
            return true;
        }
    }
    log_trace(memoria_logger, "No se encontró el proceso con PID %d en la lista de procesos suspendidos.", pid);
    pthread_mutex_unlock(mutex_procesos_en_swap);
    return false;
}

bool proceso_esta_en_swap(proceso_t* proceso) {
    pthread_mutex_lock(mutex_procesos_en_swap);
    log_trace(memoria_logger, "Verificando si el proceso con PID %d está en swap.", proceso->pid);
    for(int i = 0; i < list_size(procesos_en_swap); i++) {
        proceso_t* p = list_get(procesos_en_swap, i);
        if(p->pid == proceso->pid) {
            log_trace(memoria_logger, "El proceso con PID %d está en swap.", proceso->pid);
            pthread_mutex_unlock(mutex_procesos_en_swap);
            return true;
        }
    }
    pthread_mutex_unlock(mutex_procesos_en_swap);
    return false;
}

void liberar_espacio_en_memoria(proceso_t * proceso_finalizar) {
    // 1. Liberar los marcos ocupados por el proceso en el bitmap
    pthread_mutex_lock(mutex_bitmap);
    for (int i = 0; i < proceso_finalizar->cantidad_paginas; i++) {
        int marco = obtener_marco_de_pagina(proceso_finalizar->tabla_raiz, i);
        //eliminar contenido de los marcos ocupados por el proceso en memoria
        log_trace(memoria_logger, "Liberando marco %d del proceso con PID %d.", marco, proceso_finalizar->pid);
        memset((char*)espacio_usuario + (marco * TAM_PAGINA), 0, TAM_PAGINA);
        bitmap[marco] = false; // Liberar el marco en el bitmap
        pthread_mutex_lock(mutex_memoria_ocupada);
        MEMORIA_OCUPADA -= TAM_PAGINA; // Actualizar memoria ocupada
        pthread_mutex_unlock(mutex_memoria_ocupada);
    }
    pthread_mutex_unlock(mutex_bitmap);
}

void mostrar_metricas_proceso(proceso_t * proceso) {
    log_info(memoria_logger,  "## PID: %d - Proceso Destruido - Métricas - Acc.T.Pag: %d; Inst.Sol.: %d; SWAP: %d; Mem.Prin.: %d; Lec.Mem.: %d; Esc.Mem.: %d",
            proceso->pid, 
            proceso->accesos_tablas_paginas, 
            proceso->instrucciones_solicitadas, 
            proceso->bajadas_a_swap, 
            proceso->subidas_a_memoria, 
            proceso->lecturas_memoria, 
            proceso->escrituras_memoria);
}