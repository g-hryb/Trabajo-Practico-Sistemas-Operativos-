#ifndef KERNEL_ESTRUCTURAS_H_
#define KERNEL_ESTRUCTURAS_H_

#include "gestor_kernel.h"

typedef enum
{   
    STOP,
    NEW,
    READY,
    EXEC,
    EXIT,
    BLOCKED,
    SUSP_BLOCKED,
    SUSP_READY

}nombre_estado; //cambiar

// Métricas de Estado (ME)
typedef struct {
    int new_count;          
    int ready_count;        
    int exec_count;         
    int blocked_count;     
    int susp_ready_count;   
    int susp_blocked_count; 
    int exit_count;         //(debería ser 0 o 1)
} MetricasEstado;

// Métricas de Tiempo (MT)
typedef struct {
    int64_t new_time;        
    int64_t ready_time;      
    int64_t exec_time;       
    int64_t blocked_time;    
    int64_t susp_ready_time; 
    int64_t susp_blocked_time; 
    t_temporal* last_state_change; // Momento del último cambio de estado
} MetricasTiempo;

struct pcb
{
    uint32_t pid;
    uint32_t programCounter; //arranca n 0
    //t_registros_cpu* registrosCpu;
    //double estimadoProxRafaga; //HHRN
    //timestamp *tiempoLlegadaReady;
    //t_dictionary *archivosAbiertos;
    //uint32_t tamanioTablaSegmentos; 
    //t_info_segmentos **tablaSegmentos;
    //t_nombre_estado estadoActual;   
    //t_nombre_estado estadoDeFinalizacion;
    //t_nombre_estado estadoAnterior;
    //bool procesoBloqueadoOTerminado;
    //uint32_t socketProceso;
    //pthread_mutex_t *mutex;
    //char* dispositivoIoEnUso;
    char* path;
    int tamanio; //TAMAÑO DEL PROCESO
    MetricasEstado me;
    MetricasTiempo mt;
    //t_list *listaTCB; 
    uint32_t estado_actual;
    // Variables para SJF
    int64_t rafaga_anterior;
    t_temporal* cronometro; 
    double estimado_rafaga_anterior;
    double estimacion_proxima_rafaga;
    };
typedef struct pcb t_pcb;


typedef struct {
    int pid;
    int tid;
   // sem_t * cant_hilos_block; // Semaforo para tratar con los hilos que bloquean a este hilo por THREAD JOIN
    nombre_estado estado;
    //t_list* lista_espera; // Lista de hilos que están esperando a que el hilo corriendo termine
    //  int contador_joins;
    //t_list* instrucciones;
    //RegistroCPU *registro;  // Lista de instrucciones para el hilo
} t_tcb;

typedef struct
{
    nombre_estado nombre_estado;
    t_list *lista_procesos;
} t_cola_proceso;

typedef struct 
{
    //protocolo_socket tipo;
    t_pcb *proceso;
    t_tcb *hilo;
    bool respuesta_recibida;
    bool respuesta_exitosa;
}t_peticion;



#endif