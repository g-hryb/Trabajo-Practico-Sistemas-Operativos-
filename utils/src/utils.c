#include "../include/utils.h"

void saludar(char* quien) {
    printf("Hola desde %s!!\n", quien);
}

int crear_conexion(char *ip, char* puerto, op_code handshake) //COMO CLIENTE
{
	struct addrinfo hints;
	struct addrinfo *server_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(ip, puerto, &hints, &server_info);

	// Ahora vamos a crear el socket.
	int socket_cliente = socket(server_info->ai_family,
                         server_info->ai_socktype,
                         server_info->ai_protocol);;

	// Ahora que tenemos el socket, vamos a conectarlo
	if (connect(socket_cliente, server_info->ai_addr, server_info->ai_addrlen) == -1) {
	    perror("[ERROR] No se pudo conectar al servidor");
	    freeaddrinfo(server_info);
	    close(socket_cliente);
	    return -1; // Devuelve un valor de error
	}

	// Enviamos el handshake
	send(socket_cliente, &handshake, sizeof(int), 0);
	// Recibimos la respuesta del handshake
	recv(socket_cliente, &handshake, sizeof(int), MSG_WAITALL);
	if(handshake == -1){
		perror("[ERROR] El handshake falló");
		freeaddrinfo(server_info);
	    close(socket_cliente);
	    return -1; // Devuelve un valor de error
	}

	freeaddrinfo(server_info);

	return socket_cliente;
}

int iniciar_servidor(char* puerto, t_log* log, char* msj_server) //COMO SERVIDOR
{
	int socket_servidor;

	struct addrinfo hints, *servinfo;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(NULL, puerto, &hints, &servinfo);

	// Creamos el socket de escucha del servidor
	socket_servidor = socket(servinfo->ai_family,
    	                    servinfo->ai_socktype,
        	                servinfo->ai_protocol);

	// --- AGREGADO: Permitir reutilizar el puerto ---
    int optval = 1;
    setsockopt(socket_servidor, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    // -----------------------------------------------

	// Asociamos el socket a un puerto
	bind(socket_servidor, servinfo->ai_addr, servinfo->ai_addrlen);
	// Escuchamos las conexiones entrantes
	listen(socket_servidor, SOMAXCONN);

	freeaddrinfo(servinfo);
	log_trace(log, "Server %s escuchando en el puerto %s", msj_server, puerto);

	return socket_servidor;
}

// MUCHISIMO POTENCIAL PARA HILOS, ESPERA A LA CONEXIÓN HASTA QUE SE HACE
int esperar_cliente(int socket_servidor, t_log* unLog, op_code handshake) { //COMO SERVIDOR
    while (1) {
        // Aceptamos un nuevo cliente
        int socket_cliente = accept(socket_servidor, NULL, NULL); // BLOQUEANTE
        if (socket_cliente == -1) {
            perror("[ERROR] Error al aceptar conexión");
            continue; // Volver a intentar aceptar otra conexión
        }
		int respuesta;
        // Recibimos el código de operación (handshake)
        int cod_op = recibir_operacion(socket_cliente);
        if (cod_op != handshake) {
            log_warning(unLog, "[WARNING] El handshake no es el esperado. Cerrando conexión.");
			respuesta = -1;
			send(socket_cliente, &respuesta, sizeof(int), 0); // Enviamos el handshake incorrecto
            close(socket_cliente); // Cerramos la conexión con el cliente no esperado
            continue; // Volver a intentar aceptar otra conexión
        }

        // Si el handshake es correcto, registramos la conexión
		respuesta = 1;
		send(socket_cliente, &respuesta, sizeof(int), 0);
        log_trace(unLog, "Se conectó un cliente con el handshake esperado.");
        return socket_cliente; // Devolvemos el socket del cliente conectado
    }
}

//CONEXIONES - PAQUETE - PROTOCOLO ROLANK

//                     PAQUETE -> [cod_op|buffer]
// 						BUFFER -> [size|stream (contenido)]
// Crear buffer:
//         -> agregar int del buffer
//         -> agregar uint32_t del buffer
//         -> agregar string del buffer
//         -> agregar void* del buffer

// Recibir todo el buffer del mensaje:
//         -> extraer int del buffer
//         -> extraer uint32_t del buffer
//         -> extraer string del buffer
//         -> extraer void* del buffer


int recibir_operacion(int conexion)
{
	int cod_op; //Codigo de operacion: MENSAJE O PAQUETE, ETC.
	if(recv(conexion, &cod_op, sizeof(int), MSG_WAITALL) > 0)
	return cod_op;
	else
	{
		printf("Error al recibir el codigo de operacion\n");
		close(conexion);
		return -1;
	}
}

t_buffer* recibir_todo_el_buffer(int conexion){
	t_buffer* buffer = malloc(sizeof(t_buffer));

	if(recv(conexion, &(buffer->size), sizeof(int), MSG_WAITALL) > 0){
		buffer->stream = malloc(buffer->size);
		if(recv(conexion, buffer->stream, buffer->size, MSG_WAITALL) > 0){
			return buffer;
		}else{
			printf("Error al recibir el contenido del buffer\n");
			free(buffer);
			exit(EXIT_FAILURE);
		}
	}else{
		printf("Error al recibir el size del buffer\n");
		free(buffer);
		exit(EXIT_FAILURE);
	}
	return buffer;
}

//***************************************************

void* extraer_contenido_del_buffer(t_buffer* buffer){
	if(buffer->size == 0){
		perror("[ERROR] El buffer no tiene contenido\n");
		exit(EXIT_FAILURE);
	}
	if(buffer->size < 0){
		perror("[ERROR] El buffer tiene un tamaño negativo\n");
		exit(EXIT_FAILURE);
	}

	int size_contenido;
	memcpy(&size_contenido, buffer->stream, sizeof(int));
	void* contenido = malloc(size_contenido);
	memcpy(contenido, buffer->stream + sizeof(int), size_contenido);

	int nuevo_size = buffer->size - sizeof(int) - size_contenido;
	if(nuevo_size == 0){
		buffer->size = 0;
		free(buffer->stream);
		buffer->stream = NULL;
		free(buffer); // AGREGADO, VER SI FUNCA
		return contenido;
	}
	if(nuevo_size < 0){
		perror("[ERROR] El buffer no se ha vaciado\n");
		exit(EXIT_FAILURE);
	}
	void* nuevo_stream = malloc(nuevo_size);
	memcpy(nuevo_stream, buffer->stream + sizeof(int) + size_contenido, nuevo_size);
	free(buffer->stream);
	buffer->size = nuevo_size;
	buffer->stream = nuevo_stream;

	return contenido;
}

int extraer_int_del_buffer(t_buffer* buffer){
	int* int_value = extraer_contenido_del_buffer(buffer);
	int return_value = *int_value;
	free(int_value);
	return return_value;
}

uint32_t extraer_uint32_del_buffer(t_buffer* buffer){
	uint32_t* valor_t = extraer_contenido_del_buffer(buffer);
	uint32_t retorno_t = *valor_t;
	free(valor_t);
	return retorno_t;
}

char* extraer_string_del_buffer(t_buffer* buffer){
	char* string = extraer_contenido_del_buffer(buffer);
	return string;
}

//**********************PREPARAR BUFFER*****************************

t_buffer* crear_buffer(){
	t_buffer* buffer = malloc(sizeof(t_buffer));
	buffer->size = 0;
	buffer->stream = NULL;
	return buffer;
}

void destruir_buffer(t_buffer* buffer){
	if(buffer->stream != NULL){
		free(buffer->stream);}
	free(buffer);
}

void cargar_contenido_al_buffer(t_buffer* buffer, void* contenido, int size_contenido){
	if(buffer -> size == 0){
		buffer -> stream = malloc(sizeof(int) + size_contenido);
		memcpy(buffer->stream, &size_contenido, sizeof(int));
		memcpy(buffer->stream + sizeof(int), contenido, size_contenido);
	}else{
		buffer -> stream = realloc(buffer->stream, buffer->size + sizeof(int) + size_contenido);
		memcpy(buffer->stream + buffer->size, &size_contenido, sizeof(int));
		memcpy(buffer->stream + buffer->size + sizeof(int), contenido, size_contenido);
	}

	buffer->size += sizeof(int) + size_contenido;
}

void cargar_int_al_buffer(t_buffer* buffer, int int_value){
	cargar_contenido_al_buffer(buffer, &int_value, sizeof(int));
}

void cargar_uint32_al_buffer(t_buffer* buffer, uint32_t valor_t){
	cargar_contenido_al_buffer(buffer, &valor_t, sizeof(uint32_t));
}

void cargar_string_al_buffer(t_buffer* buffer, char* string_value){
	cargar_contenido_al_buffer(buffer, string_value, strlen(string_value)+1);
}

//***********************MANEJAR PAQUETE****************************

t_paquete* crear_paquete(op_code cod_op, t_buffer* buffer){
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->cod_op = cod_op;
	paquete->buffer = buffer;
	return paquete;
}

void destruir_paquete(t_paquete* paquete){
	destruir_buffer(paquete->buffer);
	free(paquete);
}

void* serializar_paquete(t_paquete* paquete){
	int size_envio = paquete->buffer->size + 2*sizeof(int); //+ sizeof(op_code);
	void* envio = malloc(size_envio);
	int desplazamiento = 0;

	memcpy(envio + desplazamiento, &(paquete->cod_op), sizeof(int));
	desplazamiento += sizeof(int);
	memcpy(envio + desplazamiento, &(paquete->buffer->size), sizeof(int));
	desplazamiento += sizeof(int);
	memcpy(envio + desplazamiento, paquete->buffer->stream, paquete->buffer->size);
	desplazamiento += paquete->buffer->size;

	return envio;
}

void enviar_paquete(t_paquete* paquete, int conexion)
{
	void* a_enviar = serializar_paquete(paquete);

	int bytes = paquete->buffer->size + 2*sizeof(int);
	send(conexion, a_enviar, bytes, 0);

	free(a_enviar);
	destruir_paquete(paquete);
}