#ifndef UTILS_H_
#define UTILS_H_

#include<stdlib.h>
#include<stdio.h>
#include<commons/log.h>
#include<string.h>
#include<commons/string.h>
#include<commons/config.h>
#include<commons/collections/list.h>
#include<readline/readline.h>
#include<signal.h>
#include<unistd.h>
#include<sys/time.h>
#include<sys/socket.h>
#include<netdb.h>
#include<pthread.h>
#include<semaphore.h>
#include<stdbool.h>
#include<commons/temporal.h>
#include<math.h>

// Definición de tipos de datos

typedef enum
{
    //GLOBALES
	MENSAJE,
	PAQUETE,
    HANDSHAKE_KERNEL,
    HANDSHAKE_KERNEL_DISPATCH,
    HANDSHAKE_KERNEL_INTERRUPT,
    HANDSHAKE_CPU,
    HANDSHAKE_CPU_DISPATCH,
    HANDSHAKE_CPU_INTERRUPT,
    HANDSHAKE_IO,
    HANDSHAKE_KERNEL_IO,
    HANDSHAKE_MEMORIA,
    //CPU-KERNEL
    CONTEXTO,
    CONTEXTO_INTERRUPCION,
    INTERRUPT,   
    //CPU-MEMORIA
    REQUEST_INSTRUCTION,
    INSTRUCTION_REQUESTED,
    ESPACIO_VACIO,
    //KERNEL-MEMORIA
    CREAR_PROCESO_KM,
    RESPUESTA_CREAR_PROCESO_KM,
    SUSPENDER_PROCESO_KM,
    RESPUESTA_SUSPENDER_PROCESO_KM,
    DESUSPENDER_PROCESO_KM,
    RESPUESTA_DESUSPENDER_PROCESO_KM, 
    FINALIZAR_PROCESO_KM,
    RESPUESTA_FINALIZAR_PROCESO_KM,
    MEMORY_DUMP,
    RESPUESTA_MEMORY_DUMP,
    //I/O-KERNEL
    //AVISO COMIENZO DE I/O CPU->KERNEL
    SYS_COMIENZO_IO,
    FIN_IO,
    INICIO_IO,
    //AVISO INIT_PROC CPU->KERNEL
    SYS_INIT_PROC,
    //AVISO DUMP MEMORY CPU->KERNEL
    SYS_DUMP_MEMORY,
    //AVISO EXIT CPU->KERNEL
    SYS_EXIT,
    SYS_INTERRUPT,

    //CPU-MEMORIA NUEVO
    REQUEST_FRAME,
    RESPUESTA_REQUEST_FRAME,
    REQUEST_READ,
    RESPUESTA_REQUEST_READ,
    REQUEST_WRITE,
    RESPUESTA_REQUEST_WRITE,
    REQUEST_READ_FULL_PAGE,
    RESPUESTA_REQUEST_READ_FULL_PAGE,
    REQUEST_WRITE_FULL_PAGE,  
    RESPUESTA_REQUEST_WRITE_FULL_PAGE
}op_code;

typedef struct
{
	int size;
	void* stream;
} t_buffer;

typedef struct
{
	op_code cod_op;
	t_buffer* buffer;
} t_paquete;


// Prototipos de funciones

t_log* iniciar_logger(void);
int crear_conexion(char *ip, char* puerto, op_code handshake);
int iniciar_servidor(char* unPuerto, t_log* unLog, char* msjServer);
int esperar_cliente(int socket_servidor, t_log* unLog, op_code handshake);
int recibir_operacion(int socket_cliente);
void saludar(char* quien);

int recibir_operacion(int conexion);
t_buffer* recibir_todo_el_buffer(int conexion);

//********************** GESTIONAR BUFFER *****************************

void* extraer_contenido_del_buffer(t_buffer* buffer);
int extraer_int_del_buffer(t_buffer* buffer);
uint32_t extraer_uint32_del_buffer(t_buffer* buffer);
char* extraer_string_del_buffer(t_buffer* buffer);

//********************** PREPARAR BUFFER *****************************

t_buffer* crear_buffer();
void destruir_buffer(t_buffer* buffer);
void cargar_contenido_al_buffer(t_buffer* buffer, void* contenido, int size_contenido);
void cargar_int_al_buffer(t_buffer* buffer, int int_value);
void cargar_uint32_al_buffer(t_buffer* buffer, uint32_t valor_t);
void cargar_string_al_buffer(t_buffer* buffer, char* string_value);

//************************ MANEJAR PAQUETE ***************************

t_paquete* crear_paquete(op_code cod_op, t_buffer* buffer);
void destruir_paquete(t_paquete* paquete);
void* serializar_paquete(t_paquete* paquete);
void enviar_paquete(t_paquete* paquete, int conexion);


#endif
