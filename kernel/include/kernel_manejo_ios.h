#ifndef KERNEL_MANEJO_IOS_H
#define KERNEL_MANEJO_IOS_H

#include "gestor_kernel.h"
#include "kernel_estructuras.h"
#include "kernel_io.h"

typedef struct nodoIO
{
    char* id_io;
    int fd_io;
    bool esta_ocupado;
    uint32_t proceso_en_ejecucion;
} nodoIO;

typedef struct nodoProcesoEnCola
{
    uint32_t pid;
    char* dispositivo_io;
    int tiempo_milisegundos;
} nodoProcesoEnCola;

extern t_list* lista_IO;
extern t_list* lista_procesos_en_cola;
extern sem_t* binario_IO;
extern pthread_mutex_t* mutex_procesos_en_cola;

void inicializar_list_procesos_en_cola();
void inicialiar_list_io();
void inicializar_listas_io();
void manejar_conexiones_ios();
int conectar_io();
nodoIO* crear_nodo_io(char* id, int fd_io);
void insertar_io(nodoIO* nuevo_io);
void imprimir_lista_ios();
void eliminar_nodo_io(int fd_io);
bool existe_io(char* nombre);
void agregar_a_cola_por_IO(uint32_t pid, char* dispositivo_io, int tiempo_milisegundos);
void atender_cola_espera();
int ocupar_io(char* nombre, uint32_t pid);
void liberar_io(int fd_io);

bool hay_alguno_libre_io(char* nombreIO);
bool _esta_libre(void* ptr);
bool coincide_nombre_io(t_list* lista_libres, char* nombreIO);


#endif