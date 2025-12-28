#include  "../include/kernel_pcb.h"

void inicializar_metricas(t_pcb* proceso) {
    // Inicializar contadores de estado a 0
    memset(&proceso->me, 0, sizeof(MetricasEstado));
    
    // El proceso comienza en NEW, así que contamos esta entrada
    proceso->me.new_count = 1;
    
    // Inicializar tiempos a 0
    memset(&proceso->mt, 0, sizeof(MetricasTiempo));

    proceso->mt.last_state_change = temporal_create(); // Crear un nuevo temporal para el último cambio de estado
}

// PUEDE LLEGAR A ESTAR EN LA SHARED
const char* estado_to_string(int estado) {
    static const char* strings[] = {
        "STOP","NEW", "READY", "EXEC", 
        "EXIT", "BLOCKED", "SUSP_BLOCKED", "SUSP_READY"
    };
    return strings[estado];
}

void cronometro() {
    t_temporal* inicio = temporal_create();
    if (inicio == NULL) {
        log_error(kernel_logger, "Error al crear el temporal de inicio");
        return;
    }

    sleep(2); // Simula tiempo de espera

    t_temporal* fin = temporal_create();
    if (fin == NULL) {
        log_error(kernel_logger, "Error al crear el temporal de fin");
        temporal_destroy(inicio); // Liberar el primero si el segundo falla
        return;
    }

    sleep(3); // Simula más tiempo de espera

    // Obtener tiempos con la función correcta (no acceder a ->elapsed_ms directamente)
    int64_t tiempo_inicio = temporal_gettime(inicio);
    int64_t tiempo_fin = temporal_gettime(fin);
    int64_t diferencia = temporal_diff(inicio, fin);

    log_trace(kernel_logger, "Tiempo inicio: %ld ms", tiempo_inicio);
    log_trace(kernel_logger, "Tiempo fin: %ld ms", tiempo_fin);
    log_trace(kernel_logger, "Diferencia: %ld ms", diferencia);

    // Liberar recursos
    temporal_destroy(inicio);
    temporal_destroy(fin);
}

void cambiar_estado(t_pcb* proceso, int nuevo_estado) {

    // 1. Calcular tiempo transcurrido desde el último cambio de estado
    int64_t tiempo_transcurrido = 0;

    t_temporal* fin_ahora = temporal_create();
    if (fin_ahora == NULL) {
        log_error(kernel_logger, "Error al crear el temporal de fin");
        return;
    }

    int64_t tiempo_inicio = temporal_gettime(proceso->mt.last_state_change);
    int64_t tiempo_fin = temporal_gettime(fin_ahora);
    tiempo_transcurrido = temporal_diff(proceso->mt.last_state_change, fin_ahora);
    temporal_destroy(proceso->mt.last_state_change); //????
    proceso->mt.last_state_change = fin_ahora; 
    //temporal_destroy(fin_ahora);    
    log_trace(kernel_logger, "## (%d) Cambio de estado:", proceso->pid);
    log_trace(kernel_logger, "Tiempo del ultimo cambio: %ld ms", tiempo_inicio);
    log_trace(kernel_logger, "Tiempo fin: %ld ms", tiempo_fin);
    log_trace(kernel_logger, "Diferencia: %ld ms", tiempo_transcurrido);

    // 2. Actualizar tiempo acumulado del estado actual
    switch(proceso->estado_actual) {
        case STOP:
        break;
        case NEW: 
            proceso->mt.new_time += tiempo_transcurrido;
            break;
        case READY: 
            proceso->mt.ready_time += tiempo_transcurrido;
            break;
        case EXEC: 
            proceso->mt.exec_time += tiempo_transcurrido;
            break;
        case EXIT: 
            break;
        case BLOCKED: 
            proceso->mt.blocked_time += tiempo_transcurrido;
            break;
        case SUSP_BLOCKED: 
            proceso->mt.susp_blocked_time += tiempo_transcurrido;
            break;
        case SUSP_READY: 
            proceso->mt.susp_ready_time += tiempo_transcurrido;
            break;
    
        default: break;
    }
    
    // 3. Actualizar contador del nuevo estado
    switch(nuevo_estado) {
        case NEW: proceso->me.new_count++; break;
        case READY: proceso->me.ready_count++; break;
        case EXEC: proceso->me.exec_count++; break;
        case BLOCKED: proceso->me.blocked_count++; break;
        case SUSP_READY: proceso->me.susp_ready_count++; break;
        case SUSP_BLOCKED: proceso->me.susp_blocked_count++; break;
        case EXIT: proceso->me.exit_count++; break;
    }
    
    // 4. Registrar el cambio de estado
    const char* estado_anterior = estado_to_string(proceso->estado_actual);
    proceso->estado_actual = nuevo_estado;
    
    // 5. Log obligatorio
    log_info(kernel_logger, "## (%d) Pasa del estado %s al estado %s", 
             proceso->pid, estado_anterior, estado_to_string(nuevo_estado));
}

void mostrar_metricas(const t_pcb* proceso) {
    log_info(kernel_logger, "## (%d) - Métricas de estado:", proceso->pid);
    log_info(kernel_logger, "NEW: %d veces, %ld ms", 
             proceso->me.new_count, proceso->mt.new_time);
    log_info(kernel_logger, "READY: %d veces, %ld ms", 
             proceso->me.ready_count, proceso->mt.ready_time);
    log_info(kernel_logger, "EXEC: %d veces, %ld ms", 
             proceso->me.exec_count, proceso->mt.exec_time);
    log_info(kernel_logger, "BLOCKED: %d veces, %ld ms", 
             proceso->me.blocked_count, proceso->mt.blocked_time);
    log_info(kernel_logger, "SUSP_READY: %d veces, %ld ms", 
             proceso->me.susp_ready_count, proceso->mt.susp_ready_time);
    log_info(kernel_logger, "SUSP_BLOCKED: %d veces, %ld ms", 
             proceso->me.susp_blocked_count, proceso->mt.susp_blocked_time);
    log_info(kernel_logger, "Estimación proxima rafaga: %lf", proceso->estimacion_proxima_rafaga);
    

    log_info(kernel_logger, "## (%d) - Métricas compactas: NEW (%d) (%ld), READY (%d) (%ld), EXEC (%d) (%ld), BLOCKED (%d) (%ld), SUSP_READY (%d) (%ld), SUSP_BLOCKED (%d) (%ld)",
             proceso->pid,
             proceso->me.new_count, proceso->mt.new_time,
             proceso->me.ready_count, proceso->mt.ready_time,
             proceso->me.exec_count, proceso->mt.exec_time,
             proceso->me.blocked_count, proceso->mt.blocked_time,
             proceso->me.susp_ready_count, proceso->mt.susp_ready_time,
             proceso->me.susp_blocked_count, proceso->mt.susp_blocked_time);
}