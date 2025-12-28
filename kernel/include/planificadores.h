#ifndef PLANIFICADORES_H
#define PLANIFICADORES_H

#include "gestor_kernel.h"
#include "kernel_estructuras.h"


extern t_cola_proceso* cola_NEW;
extern pthread_mutex_t * mutex_NEW;
extern sem_t * contador_NEW;

extern t_cola_proceso* cola_READY;
extern pthread_mutex_t * mutex_READY;
extern sem_t * contador_READY;

extern t_cola_proceso* cola_RETRY;
extern pthread_mutex_t * mutex_RETRY;
extern sem_t * contador_RETRY;

extern t_cola_proceso* cola_EXEC;
extern pthread_mutex_t * mutex_EXEC;
extern sem_t * contador_EXEC;

extern t_cola_proceso* cola_BLOCKED;
extern pthread_mutex_t * mutex_BLOCKED;
extern sem_t * contador_BLOCKED;

extern t_cola_proceso* cola_EXIT;
extern pthread_mutex_t * mutex_EXIT;
extern sem_t * contador_EXIT;

extern char* algoritmo;

extern pthread_mutex_t* mutex_pid;

extern uint32_t pid;

extern pthread_mutex_t * mutex_socket_memoria;

void inicializar_cola_READY();
void inicializar_cola_NEW();
void inicializar_cola_EXEC();
void inicializar_cola_BLOCKED();
void inicializar_cola_EXIT();
void inicializar_cola_SUSP_READY();
void inicializar_cola_SUSP_BLOCKED();
void inicializar_colas();
void inicializar_semaforos();
void inicializar_planificadores();

void agregar_a_NEW(t_pcb* pcb);
void agregar_a_READY(t_pcb* pcb);
void agregar_a_EXEC(t_pcb* pcb);
void agregar_a_BLOCKED(t_pcb* pcb);
void agregar_a_EXIT(t_pcb* pcb);
void agregar_a_SUSP_BLOCKED(t_pcb* pcb);
void agregar_a_SUSP_READY(t_pcb* pcb);

t_pcb* crear_proceso_prueba(int pid, char* path, int tamanio, int64_t rafaga_anterior, double estimado_rafaga_anterior);

void ejecutar_algoritmo_planificador_largo();
void ejecutar_algoritmo_planificador_corto();
bool esta_vacia(t_cola_proceso* cola, pthread_mutex_t* mutex);
t_pcb* INIT_PROC(char* path, int tamanio);
bool no_hay_ningun_proceso();
bool intentar_enviar_proceso(t_pcb* proceso);
void leer_cola_NEW();
void leer_cola_READY();
void corto_plazo_FIFO();
void corto_plazo_FIFO_READY();
bool enviar_a_cpu(t_pcb* proceso);
void largo_plazo_FIFO();
void largo_plazo_FIFO_NEW();
void largo_plazo_PMCP();
void largo_plazo_PMCP_NEW();
bool _tiene_tamanio_mas_chico(void* a, void* b); 

void corto_plazo_SJF();
void corto_plazo_SJF_READY();
void aplicar_SFJ(t_list* procesos);
bool tiene_rafaga_mas_corta(void* a, void* b);
void calculo_SJF();
void exec_a_blocked(uint32_t pid, uint32_t pc);
void blocked_a_ready(uint32_t pid);

t_pcb* buscar_proceso_por_pid(uint32_t pid);
void EXIT_PID(uint32_t pid);

void manejar_tiempo_blocked(t_pcb* proceso);
void iniciar_timer_blocked(t_pcb* proceso);
bool intentar_desuspender_proceso(t_pcb* proceso);

void corto_plazo_SRT();
void corto_plazo_SRT_READY();
bool enviar_a_cpu_desalojo(t_pcb* proceso);
#endif // PLANIFICADORES_H



