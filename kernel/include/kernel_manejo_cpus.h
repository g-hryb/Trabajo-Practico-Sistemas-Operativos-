#ifndef KERNEL_MANEJO_CPUS_H
#define KERNEL_MANEJO_CPUS_H

#include "gestor_kernel.h"
#include "kernel_estructuras.h"
#include "kernel_cpu_dispatch.h"
#include "kernel_cpu_interrupt.h"


typedef struct {
    int dispatch;
    int interrupt;
} fds_cpu_t;

typedef enum
{   
    DISPONIBLE,
    OCUPADO

}estado_cpu;

typedef struct nodoCPU
{
    int id_cpu;
    int fd_dispatch;
    int fd_interrupt;
    estado_cpu estado;
    t_pcb* proceso_running;
} nodoCPU;

extern t_list* lista_CPU;
extern sem_t* binario_desalojado;

void inicializar_lista_cpu();
void manejar_conexiones_cpus();
fds_cpu_t conectar_cpu();
nodoCPU* crear_nodo_cpu(int id, int fd_disp, int fd_int);
void imprimir_lista_cpus();
int buscar_cpu_disponible(t_pcb* proceso);
void liberar_cpu(uint32_t pid_buscar);


bool _tiene_estimacion_mas_grande(void* a, void* b);
int buscar_cpu_disponible_desalojo(t_pcb* proceso);
void desalojar_cpu(nodoCPU* cpu);
#endif