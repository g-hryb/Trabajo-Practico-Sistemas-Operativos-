
#include "../include/cpu_ciclo_instruccion.h"
#include "../include/cpu_memoria.h"
#include <math.h>
#include "../include/cpu_kernel_dispatch.h"
#include "../include/cpu_kernel_interrupt.h"

bool fin_proceso = false;
bool io_dump = false;
pthread_mutex_t *mutex_cache;
pthread_mutex_t *mutex_tlb;
sem_t *sem_interrupt;

char *fetch(char *instruccion)
{
    //sem_wait(sem_pid_pc);
    log_info(cpu_logger, "## PID: <%d> - FETCH - Program Counter: <%d>", pid, pc);
    instruccion = pedir_a_memoria_instruccion(pc, pid);

    //log_trace(cpu_logger, "Instruccion recibida: %s", instruccion);

    return instruccion;
}

void decode(char *instruccion)
{

    char *argumentos[3] = {NULL, NULL, NULL};
    separar_en_tres_strings(instruccion, argumentos);
    int codigo_instruccion = obtener_instruccion(argumentos[0]);


    switch (codigo_instruccion)
    {
    case NOOP:
        execute(NOOP, argumentos, pid);
        break;
    case WRITE: // DIRECCION, DATOS

        execute(WRITE, argumentos, pid);

        break;
    case READ: // DIRECCIÓN, TAMAÑO

        execute(READ, argumentos, pid);

        break;
    case GOTO: // VALOR

        execute(GOTO, argumentos, pid);

        break;
    // SYSCALLS //No estoy seguro si es dispatch o interrupt
    case IO: // DISPOSITIVO, TIEMPO
        pthread_mutex_lock(mutex_pc);
        pc += 1;
        pthread_mutex_unlock(mutex_pc);
        t_buffer *buffer_IO = crear_buffer();
        cargar_uint32_al_buffer(buffer_IO, pid);
        cargar_uint32_al_buffer(buffer_IO, pc);
        cargar_string_al_buffer(buffer_IO, argumentos[1]);
        cargar_int_al_buffer(buffer_IO, atoi(argumentos[2]));

        t_paquete *paqueteTemp = crear_paquete(SYS_COMIENZO_IO, buffer_IO);
        enviar_paquete(paqueteTemp, fd_kernel_dispatch);
        io_dump = true;
        // sem_post(sem_pid_pc);
        desalojar_proceso(tlb, cache);
        esperar_contexto = true;

        return;
        break;
    case INIT_PROC: // ARCHIVO DE INSTRUCCIONES, TAMAÑO

        t_buffer *tempINIT = crear_buffer();
        cargar_uint32_al_buffer(tempINIT, pid);
        cargar_string_al_buffer(tempINIT, argumentos[1]);
        cargar_int_al_buffer(tempINIT, atoi(argumentos[2]));
        t_paquete *paqueteInit = crear_paquete(SYS_INIT_PROC, tempINIT);
        enviar_paquete(paqueteInit, fd_kernel_dispatch);

        pthread_mutex_lock(mutex_pc);
        pc += 1;
        pthread_mutex_unlock(mutex_pc);
        sem_post(sem_pid_pc);

        break;
    case DUMP_MEMORY:
        pthread_mutex_lock(mutex_pc);
        pc += 1;
        pthread_mutex_unlock(mutex_pc);
        fin_proceso = false; // verificar que se tiene que cortar el proceso cuando hay dump (se bloquea)
        t_buffer *tempDUMP = crear_buffer();
        cargar_uint32_al_buffer(tempDUMP, pid);
        cargar_uint32_al_buffer(tempDUMP, pc);
        t_paquete *paqueteDump = crear_paquete(SYS_DUMP_MEMORY, tempDUMP);
        enviar_paquete(paqueteDump, fd_kernel_dispatch);

        io_dump = true;
        // sem_post(sem_pid_pc);
        esperar_contexto = true;
        desalojar_proceso(tlb, cache);
        return;
        break;
    case EXIT: // se tiene que limpiar toda la tlb y cache cuando termine este proceso
        fin_proceso = true;
        fin_proceso = false; // verificar que se tiene que cortar el proceso cuando hay IO (se bloquea)
        t_buffer *tempEXIT = crear_buffer();
        cargar_uint32_al_buffer(tempEXIT, pid);
        t_paquete *paqueteExit = crear_paquete(SYS_EXIT, tempEXIT);
        enviar_paquete(paqueteExit, fd_kernel_dispatch);
        // sem_post(sem_pid_pc);
        io_dump = true;
        desalojar_proceso(tlb, cache);
        esperar_contexto = true;
        return;
        break;
    default:
        log_warning(cpu_logger, "Instruccion desconocida de KERNEL DISPATCH.");
        break;
    }
}

void execute(int instruccion, char **argumentos, uint32_t pid)
{ 
    switch (instruccion)
    {
    case NOOP:
        log_info(cpu_logger, "## PID: <%d> - Ejecutando <NOOP> - < >", pid);
        pthread_mutex_lock(mutex_pc);
        pc += 1;
        pthread_mutex_unlock(mutex_pc);

        sem_post(sem_pid_pc);
        break;
    case WRITE:
        
        pthread_mutex_lock(mutex_pc);
        pc += 1;
        pthread_mutex_unlock(mutex_pc);

        int direccionLogica_write = atoi(argumentos[1]);
        char *datos = argumentos[2];
        log_info(cpu_logger, "## PID: <%d> - Ejecutando <WRITE> - < %d  %s >", pid, direccionLogica_write, datos);

        ejecutar_write(direccionLogica_write, datos, pid, cache, tlb);

        sem_post(sem_pid_pc);
        break;
    case READ:
        pthread_mutex_lock(mutex_pc);
        pc += 1;
        pthread_mutex_unlock(mutex_pc);

        int direccionLogica_read = atoi(argumentos[1]);
        int tamanio = atoi(argumentos[2]);

        log_info(cpu_logger, "## PID: <%d> - Ejecutando <READ> - < %d %d >", pid, direccionLogica_write, tamanio);

        ejecutar_read(direccionLogica_read, tamanio, pid, cache, tlb);

        sem_post(sem_pid_pc);
        break;
    case GOTO:

        uint32_t pc_instruccion = atoi(argumentos[1]);

        log_info(cpu_logger, "## PID: <%d> - Ejecutando <GOTO> - < %d >", pid, pc_instruccion);

        pthread_mutex_lock(mutex_pc);
        pc = pc_instruccion;
        pthread_mutex_unlock(mutex_pc);
        log_trace(cpu_logger, "PC actualizado a %d", pc);

        sem_post(sem_pid_pc);
        break;
    }
}

void check_interrupt()
{

    if (interrupcion_pendiente)
    {
       // log_debug(cpu_logger, "Interrupción pendiente detectada");
        t_buffer *buffer_inter = crear_buffer();
        cargar_uint32_al_buffer(buffer_inter, pid);
        cargar_uint32_al_buffer(buffer_inter, pc);
        t_paquete *paquete_interrupt = crear_paquete(SYS_INTERRUPT, buffer_inter);
        enviar_paquete(paquete_interrupt, fd_kernel_interrupt);

        fin_proceso = false; // verificar que se tiene que cortar el proceso cuando hay IO (se bloquea)
        sem_wait(sem_interrupt);
        interrupcion_pendiente = false; // Reiniciar la interrupción pendiente
    }
    else
    {
        log_trace(cpu_logger, "No hay interrupciones pendientes");
    }
}

void inicializar_tlb()
{

    if (ENTRADAS_TLB > 0)
    {
        tlb = malloc(ENTRADAS_TLB * sizeof(t_tlb));
        for (int i = 0; i <= ENTRADAS_TLB - 1; i++)
        {
            tlb[i].nro_pagina = -1;
            tlb[i].pid = -1;
            tlb[i].marco = -1;
            tlb[i].tiempo_en_tlb = 0;
        }
    }
    else
    {
    }
}

void inicializar_cache()
{

    if (ENTRADAS_CACHE > 0)
    {
        cache = malloc(ENTRADAS_CACHE * sizeof(t_cache));

        for (int i = 0; i <= ENTRADAS_CACHE - 1; i++)
        {
            cache[i].nro_pagina = -1;
            cache[i].pid = -1;
            cache[i].contenido = NULL;
            cache[i].bit_uso = 0;
            cache[i].modificado = 0;
            cache[i].tiempo_en_cache = 0;
        }
    }
    else
    {
    }
}

void inicializar_semaforos_cache()
{
    mutex_cache = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(mutex_cache, NULL);
}

void inicializar_semaforos_tlb()
{
    mutex_tlb = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(mutex_tlb, NULL);
}

void inicializar_semaforo_interrupt()
{
    sem_interrupt = malloc(sizeof(sem_t));
    sem_init(sem_interrupt, 0, 0);
}

char *pedir_a_memoria_instruccion(uint32_t pc, uint32_t pid)
{
    t_buffer *buffer = crear_buffer();
    cargar_uint32_al_buffer(buffer, pc);
    cargar_uint32_al_buffer(buffer, pid);
    t_paquete *paquete = crear_paquete(REQUEST_INSTRUCTION, buffer);
    enviar_paquete(paquete, fd_memoria);

    sem_wait(sem_instruccion);
    return instruccion;
}

void separar_en_tres_strings(const char *linea, char **argumentos)
{
    if (linea == NULL || argumentos == NULL)
    {
        return;
    }
    int i = 0, inicio = 0, palabra_actual = 0;
    size_t len = strlen(linea);

    for (i = 0; i <= len; i++)
    {
        // Si encontramos un espacio o el final de la cadena
        if (linea[i] == ' ' || linea[i] == '\0')
        {
            size_t tamanio_palabra = i - inicio;
            argumentos[palabra_actual] = malloc(tamanio_palabra + 1); // +1 para '\0'
            if (argumentos[palabra_actual] == NULL)
            {
                // Liberar memoria en caso de error
                for (int j = 0; j < palabra_actual; j++)
                    free(argumentos[j]);
                return;
            }

            // Copiar la palabra al nuevo espacio
            memcpy(argumentos[palabra_actual], &linea[inicio], tamanio_palabra);
            argumentos[palabra_actual][tamanio_palabra] = '\0'; // Agregar '\0'

            palabra_actual++;
            inicio = i + 1; // Actualizar el inicio para la próxima palabra

            // Si ya tenemos 3 palabras, salimos del bucle
            if (palabra_actual == 3)
                break;
        }
    }

    // Si hay menos de 3 palabras, llenamos los restantes con NULL
    while (palabra_actual < 3)
    {
        argumentos[palabra_actual] = NULL;
        palabra_actual++;
    }
}

void limpiar_cadena(char *str)
{
    if (str == NULL)
        return;

    size_t len = strlen(str);
    for (size_t i = 0; i < len; i++)
    {
        if (str[i] == '\n' || str[i] == '\r' || str[i] == ' ')
        {
            str[i] = '\0'; // Reemplazar caracteres no deseados con '\0'
            break;         // Terminar la cadena en el primer carácter no deseado
        }
    }
}

int obtener_instruccion(char *str)
{
    limpiar_cadena(str);
    if (strcmp(str, "NOOP") == 0)
        return NOOP;
    if (strcmp(str, "WRITE") == 0)
        return WRITE;
    if (strcmp(str, "READ") == 0)
        return READ;
    if (strcmp(str, "GOTO") == 0)
        return GOTO;
    if (strcmp(str, "IO") == 0)
        return IO;
    if (strcmp(str, "INIT_PROC") == 0)
        return INIT_PROC;
    if (strcmp(str, "DUMP_MEMORY") == 0)
        return DUMP_MEMORY;
    if (strcmp(str, "EXIT") == 0)
        return EXIT;
    else
        return -1;
}

uint32_t mmu(int tamanioPagina, int marco, int direccionLogica)
{

    int desplazamiento = calcular_desplazamiento(direccionLogica);

    uint32_t direccionFisica = (marco * tamanioPagina) + desplazamiento;

    return direccionFisica;
}

int calcular_desplazamiento(int direccionLogica)
{
    int des = direccionLogica % tamanioPagina;
    return des;
}

int tlb_hit(int numero_pagina, u_int32_t pid, t_tlb *tlb)
{ // DEBERIA SER GLOBAL DE CPU ME PARECE

    for (int i = 0; i <= ENTRADAS_TLB - 1; i++)
    {
        if (tlb[i].nro_pagina == numero_pagina && tlb[i].pid == pid)
        {
            log_info(cpu_logger, " PID: <PID> - TLB HIT - < %d >", numero_pagina);
            return tlb[i].marco; // queda sumar desplazamineto
        }
    }
    log_info(cpu_logger, " PID: <PID> - TLB MISS - < %d >", numero_pagina);
    return -1; // NO HAY TLB HIT
}

int cache_hit(int numero_pagina, t_cache *cache)
{
     

    for (int i = 0; i <= ENTRADAS_CACHE - 1; i++)
    {
        if (cache[i].nro_pagina == numero_pagina && cache[i].pid == pid)
        {
            return i;
        }
    }
    return -1; // NO HAY CACHE HIT
}

int buscar_en_MP(int numero_pagina, int entradasPorTabla, int cantidadDeNiveles, int tamanioPagina, int direccionLogica, u_int32_t pid)
{

    int *entradaNIvelX = entrada_nivel_X(numero_pagina, entradasPorTabla, cantidadDeNiveles);

    t_buffer *buffer_mmu = crear_buffer();
    cargar_uint32_al_buffer(buffer_mmu, pid);
    for (int i = 0; i < cantidadDeNiveles; i++)
    {
        cargar_int_al_buffer(buffer_mmu, entradaNIvelX[i]);
    }
    t_paquete *paquete_mmu = crear_paquete(REQUEST_FRAME, buffer_mmu);
    enviar_paquete(paquete_mmu, fd_memoria);
    ///

    sem_wait(sem_marco);
    return marco_proceso;
}

void ejecutar_write(int direccionLogica, char *datos, uint32_t pid, t_cache *cache, t_tlb *tlb)
{

    int numero_pagina = nro_pagina(direccionLogica, tamanioPagina);
    int rta_cache = cache_hit(numero_pagina, cache);
    int rta_tlb = tlb_hit(numero_pagina, pid, tlb);

    if (ENTRADAS_CACHE > 0)
    {

        if (rta_cache == -1)
        {
            log_info(cpu_logger, " PID: <%d> - Cache Miss - Pagina: <%d> ", pid, numero_pagina);
            if (ENTRADAS_TLB > 0 && rta_tlb != -1)
            {
                log_trace(cpu_logger, "TENEMOS CACHE Y TLB, ESTA EN TLB");
                pthread_mutex_lock(mutex_tlb);
                int marco = rta_tlb;
                log_info(cpu_logger, " PID: <%d> - OBTENER MARCO - Pagina: <%d> - Marco: <%d>", pid, numero_pagina, marco);
                pthread_mutex_unlock(mutex_tlb);
                uint32_t direccionFisica = marco * tamanioPagina;
                t_buffer *buffer_write_memoria = crear_buffer();
                cargar_uint32_al_buffer(buffer_write_memoria, pid);
                cargar_uint32_al_buffer(buffer_write_memoria, direccionFisica);
                t_paquete *paquete_write = crear_paquete(REQUEST_READ_FULL_PAGE, buffer_write_memoria);
                enviar_paquete(paquete_write, fd_memoria);

                sem_wait(sem_pagina);
                pthread_mutex_lock(mutex_cache);
                actualizar_cache(cache, tlb, numero_pagina, pid, datos,tamanio_contenido ,1, direccionLogica, direccionFisica);
                pthread_mutex_unlock(mutex_cache);
                actualizar_tlb(tlb, numero_pagina, pid, marco);

            }
            else if (ENTRADAS_TLB > 0 && rta_tlb == -1)
            {
                log_trace(cpu_logger, "TENEMOS CACHE Y TLB, no esta en ninguno de las dos");
                int marco = buscar_en_MP(numero_pagina, entradasPorTabla, cantidadDeNiveles, tamanioPagina, direccionLogica, pid);
                log_info(cpu_logger, " PID: <%d> - OBTENER MARCO - Pagina: <%d> - Marco: <%d>", pid, numero_pagina, marco);
                uint32_t direccionFisica = marco * tamanioPagina;

                t_buffer *buffer_write_memoria = crear_buffer();
                cargar_uint32_al_buffer(buffer_write_memoria, pid);

                cargar_uint32_al_buffer(buffer_write_memoria, direccionFisica);


                t_paquete *paquete_write = crear_paquete(REQUEST_READ_FULL_PAGE, buffer_write_memoria);
                enviar_paquete(paquete_write, fd_memoria);

                sem_wait(sem_pagina);

                pthread_mutex_lock(mutex_cache);

                actualizar_cache(cache, tlb, numero_pagina, pid, datos,strlen(datos), 1, direccionLogica, direccionFisica);

                pthread_mutex_unlock(mutex_cache);
                pthread_mutex_lock(mutex_tlb);

                actualizar_tlb(tlb, numero_pagina, pid, marco);

                pthread_mutex_unlock(mutex_tlb);
            }
            else if (ENTRADAS_TLB <= 0)
            {
                log_trace(cpu_logger, "NO ESTA EN CACHE Y NO EN TLB");
                int marco = buscar_en_MP(numero_pagina, entradasPorTabla, cantidadDeNiveles, tamanioPagina, direccionLogica, pid);
                log_info(cpu_logger, " PID: <%d> - OBTENER MARCO - Pagina: <%d> - Marco: <%d>", pid, numero_pagina, marco);
                uint32_t direccionFisica = marco * tamanioPagina;
                t_buffer *buffer_write_memoria = crear_buffer();
                cargar_uint32_al_buffer(buffer_write_memoria, pid);
                cargar_uint32_al_buffer(buffer_write_memoria, direccionFisica);

                t_paquete *paquete_write = crear_paquete(REQUEST_READ_FULL_PAGE, buffer_write_memoria);
                enviar_paquete(paquete_write, fd_memoria);

                sem_wait(sem_pagina);
                pthread_mutex_lock(mutex_cache);

                actualizar_cache(cache, tlb, numero_pagina, pid, datos,strlen(datos), 1, direccionLogica, direccionFisica);

                pthread_mutex_unlock(mutex_cache);
            }
        }
        else
        {
            log_info( cpu_logger, " PID: <%d> - Cache Hit - Pagina: <%d> ",pid, numero_pagina);
            if (ENTRADAS_TLB > 0 && rta_tlb != -1)
            {

                int marco = rta_tlb;
                log_info(cpu_logger, " PID: <%d> - OBTENER MARCO - Pagina: <%d> - Marco: <%d>", pid, numero_pagina, marco);
                uint32_t direccionFisica = marco * tamanioPagina;
                t_buffer *buffer_write_memoria = crear_buffer();
                cargar_uint32_al_buffer(buffer_write_memoria, pid);
                cargar_uint32_al_buffer(buffer_write_memoria, direccionFisica);
                t_paquete *paquete_write = crear_paquete(REQUEST_READ_FULL_PAGE, buffer_write_memoria);
                enviar_paquete(paquete_write, fd_memoria);

                sem_wait(sem_pagina);
                pthread_mutex_lock(mutex_cache);

                actualizar_cache(cache, tlb, numero_pagina, pid, datos,strlen(datos), 1, direccionLogica, direccionFisica);

                pthread_mutex_unlock(mutex_cache);
            }
            else if (ENTRADAS_TLB > 0 && rta_tlb == -1)
            {

                log_trace(cpu_logger, "TENEMOS CACHE Y TLB, no esta en ninguno de las dos");
                int marco = buscar_en_MP(numero_pagina, entradasPorTabla, cantidadDeNiveles, tamanioPagina, direccionLogica, pid);
                log_info(cpu_logger, " PID: <%d> - OBTENER MARCO - Pagina: <%d> - Marco: <%d>", pid, numero_pagina, marco);
                uint32_t direccionFisica = marco * tamanioPagina;

                t_buffer *buffer_write_memoria = crear_buffer();
                cargar_uint32_al_buffer(buffer_write_memoria, pid);

                cargar_uint32_al_buffer(buffer_write_memoria, direccionFisica);

                t_paquete *paquete_write = crear_paquete(REQUEST_READ_FULL_PAGE, buffer_write_memoria);
                enviar_paquete(paquete_write, fd_memoria);

                sem_wait(sem_pagina);

                pthread_mutex_lock(mutex_cache);
                actualizar_cache(cache, tlb, numero_pagina, pid, datos,strlen(datos), 1, direccionLogica, direccionFisica);
                pthread_mutex_unlock(mutex_cache);
                pthread_mutex_lock(mutex_tlb);
                actualizar_tlb(tlb, numero_pagina, pid, marco);
                pthread_mutex_unlock(mutex_tlb);
            }
            else
            {

                log_trace(cpu_logger, "NO HAY CACHE NI TLB");
                int marco = buscar_en_MP(numero_pagina, entradasPorTabla, cantidadDeNiveles, tamanioPagina, direccionLogica, pid);
                log_info(cpu_logger, " PID: <%d> - OBTENER MARCO - Pagina: <%d> - Marco: <%d>", pid, numero_pagina, marco);
                uint32_t direccionFisica = marco * tamanioPagina;
                t_buffer *buffer_write_memoria = crear_buffer();
                cargar_uint32_al_buffer(buffer_write_memoria, pid);
                cargar_uint32_al_buffer(buffer_write_memoria, direccionFisica);
                t_paquete *paquete_write = crear_paquete(REQUEST_READ_FULL_PAGE, buffer_write_memoria);
                enviar_paquete(paquete_write, fd_memoria);
                sem_wait(sem_pagina);
                pthread_mutex_lock(mutex_cache);
                actualizar_cache(cache, tlb, numero_pagina, pid, datos,strlen(datos), 1, direccionLogica, direccionFisica);
                pthread_mutex_unlock(mutex_cache);
            }
        }
    }
    else if (ENTRADAS_CACHE <= 0)
    {
        if (ENTRADAS_TLB > 0 && rta_tlb != -1)
        {
            log_trace(cpu_logger, "HAY TLB HIT");
            pthread_mutex_lock(mutex_tlb);
            int marco = rta_tlb;
            pthread_mutex_unlock(mutex_tlb);
            uint32_t direccionFisica = mmu(tamanioPagina, marco, direccionLogica);
            log_info(cpu_logger, " PID: <%d> - OBTENER MARCO - Pagina: <%d> - Marco: <%d>", pid, numero_pagina, marco);
            t_buffer *buffer_write_memoria = crear_buffer();
            cargar_uint32_al_buffer(buffer_write_memoria, pid);
            cargar_uint32_al_buffer(buffer_write_memoria, direccionFisica);
            cargar_int_al_buffer(buffer_write_memoria, strlen(datos));
            cargar_string_al_buffer(buffer_write_memoria, datos);
            t_paquete *paquete_write = crear_paquete(REQUEST_WRITE, buffer_write_memoria);
            enviar_paquete(paquete_write, fd_memoria);

            log_info(cpu_logger, " PID: <%d> - Accion: ESCRIBIR - Direccion Fisica: <%d> - Datos: <%s>", pid, direccionFisica, datos);
        }
        else if (ENTRADAS_TLB > 0 && rta_tlb == -1)
        {
            log_trace(cpu_logger, "NO HAY TLB HIT");
            int marco = buscar_en_MP(numero_pagina, entradasPorTabla, cantidadDeNiveles, tamanioPagina, direccionLogica, pid);
            log_info(cpu_logger, " PID: <%d> - OBTENER MARCO - Pagina: <%d> - Marco: <%d>", pid, numero_pagina, marco);
            pthread_mutex_lock(mutex_tlb);
            actualizar_tlb(tlb, numero_pagina, pid, marco);
            pthread_mutex_unlock(mutex_tlb);
            uint32_t direccionFisica = mmu(tamanioPagina, marco, direccionLogica);
            t_buffer *buffer_write_memoria = crear_buffer();
            cargar_uint32_al_buffer(buffer_write_memoria, pid);
            cargar_uint32_al_buffer(buffer_write_memoria, direccionFisica);
            cargar_int_al_buffer(buffer_write_memoria, strlen(datos));
            cargar_string_al_buffer(buffer_write_memoria, datos);
            t_paquete *paquete_write = crear_paquete(REQUEST_WRITE, buffer_write_memoria);
            enviar_paquete(paquete_write, fd_memoria);

            log_info(cpu_logger, " PID: <%d> - Accion: ESCRIBIR - Direccion Fisica: <%d> - Datos: <%s>", pid, direccionFisica, datos);
        }
        else if (ENTRADAS_TLB <= 0)
        {
            log_trace(cpu_logger, "NO HAY CACHE, NO HAY TLB");
            int marco = buscar_en_MP(numero_pagina, entradasPorTabla, cantidadDeNiveles, tamanioPagina, direccionLogica, pid);
            uint32_t direccionFisica = mmu(tamanioPagina, marco, direccionLogica);
            log_info(cpu_logger, " PID: <%d> - OBTENER MARCO - Pagina: <%d> - Marco: <%d>", pid, numero_pagina, marco);
            t_buffer *buffer_write_memoria = crear_buffer();
            cargar_uint32_al_buffer(buffer_write_memoria, pid);
            cargar_uint32_al_buffer(buffer_write_memoria, direccionFisica);
            cargar_int_al_buffer(buffer_write_memoria, strlen(datos));
            cargar_string_al_buffer(buffer_write_memoria, datos);
            t_paquete *paquete_write = crear_paquete(REQUEST_WRITE, buffer_write_memoria);
            enviar_paquete(paquete_write, fd_memoria);

            log_info(cpu_logger, " PID: <%d> - Accion: ESCRIBIR - Direccion Fisica: <%d> - Datos: <%s>", pid, direccionFisica, datos);
        }
        else
        {
            log_error(cpu_logger, "Error al ejecutar WRITE: No se pudo encontrar la pagina en memoria ni en TLB.");
            // Manejar el error de manera adecuada
        }
    }
}

void ejecutar_read(int direccionLogica, int tamanio, uint32_t pid, t_cache *cache, t_tlb *tlb)
{//SE TIENEN QUE MOSTRAR LOS DATOS LEIDOS
    int numero_pagina = nro_pagina(direccionLogica, tamanioPagina);
    int cache_hit_result = cache_hit(numero_pagina, cache);
    int tlb_hit_result = tlb_hit(numero_pagina, pid, tlb);
    if (ENTRADAS_CACHE > 0)
    {

        if (cache_hit_result != -1)
        {
            log_info( cpu_logger, " PID: <%d> - Cache Hit - Pagina: <%d> ",pid, numero_pagina);
            char *contenido = cache[cache_hit_result].contenido;
            
            mostrar_cache(cache[cache_hit_result]);
            
        }
        else if (cache_hit_result == -1)
        {
            if (ENTRADAS_TLB > 0 && tlb_hit_result != -1)
            {
                log_trace(cpu_logger, "TENEMOS CACHE Y TLB, ESTA EN TLB");
                pthread_mutex_lock(mutex_tlb);
                int marco = tlb_hit_result;
                pthread_mutex_unlock(mutex_tlb);
                uint32_t direccionFisica = marco * tamanioPagina;
                log_info(cpu_logger, " PID: <%d> - OBTENER MARCO - Pagina: <%d> - Marco: <%d>", pid, numero_pagina, marco);
                t_buffer *buffer_read_memoria = crear_buffer();
                cargar_uint32_al_buffer(buffer_read_memoria, pid);
                cargar_uint32_al_buffer(buffer_read_memoria, direccionFisica);
                cargar_int_al_buffer(buffer_read_memoria, tamanio);

                t_paquete *paquete_read = crear_paquete(REQUEST_READ_FULL_PAGE, buffer_read_memoria);
                enviar_paquete(paquete_read, fd_memoria);
                // esperar respuesta de memoria
                sem_wait(sem_pagina);

                pthread_mutex_lock(mutex_cache);
                actualizar_cache(cache, tlb, numero_pagina, pid, datos_memoria,tamanio_contenido, 0, direccionLogica, direccionFisica);
                pthread_mutex_unlock(mutex_cache);
                pthread_mutex_lock(mutex_tlb);
                actualizar_tlb(tlb, numero_pagina, pid, marco);
                pthread_mutex_unlock(mutex_tlb);

            }
            else if (ENTRADAS_TLB > 0 && tlb_hit_result == -1)
            {
                log_trace(cpu_logger, "TENEMOS CACHE Y TLB, no esta en ninguno de las dos");
                int marco = buscar_en_MP(numero_pagina, entradasPorTabla, cantidadDeNiveles, tamanioPagina, direccionLogica, pid);
                log_info(cpu_logger, " PID: <%d> - OBTENER MARCO - Pagina: <%d> - Marco: <%d>", pid, numero_pagina, marco);
                uint32_t direccionFisica = marco * tamanioPagina;
                t_buffer *buffer_read_memoria = crear_buffer();
                cargar_uint32_al_buffer(buffer_read_memoria, pid);
                cargar_uint32_al_buffer(buffer_read_memoria, direccionFisica);
                cargar_int_al_buffer(buffer_read_memoria, tamanio);
                t_paquete *paquete_read = crear_paquete(REQUEST_READ_FULL_PAGE, buffer_read_memoria);
                enviar_paquete(paquete_read, fd_memoria);
                // esperar respuesta de memoria
                sem_wait(sem_pagina);
                pthread_mutex_lock(mutex_cache);
                actualizar_cache(cache, tlb, numero_pagina, pid, datos_memoria,tamanio_contenido, 0, direccionLogica, direccionFisica);
                pthread_mutex_unlock(mutex_cache);
                pthread_mutex_lock(mutex_tlb);
                actualizar_tlb(tlb, numero_pagina, pid, marco);
                pthread_mutex_unlock(mutex_tlb);

            }
            else if (ENTRADAS_TLB == 0)
            {

                int marco = buscar_en_MP(numero_pagina, entradasPorTabla, cantidadDeNiveles, tamanioPagina, direccionLogica, pid);
                log_info(cpu_logger, " PID: <%d> - OBTENER MARCO - Pagina: <%d> - Marco: <%d>", pid, numero_pagina, marco);
                uint32_t direccionFisica = marco * tamanioPagina;
                t_buffer *buffer_read_memoria = crear_buffer();
                cargar_uint32_al_buffer(buffer_read_memoria, pid);
                cargar_int_al_buffer(buffer_read_memoria, direccionFisica);
                cargar_int_al_buffer(buffer_read_memoria, tamanio);
                t_paquete *paquete_read = crear_paquete(REQUEST_READ_FULL_PAGE, buffer_read_memoria);
                enviar_paquete(paquete_read, fd_memoria);
                // esperar respuesta de memoria
                sem_wait(sem_pagina);
                pthread_mutex_lock(mutex_cache);
                actualizar_cache(cache, tlb, numero_pagina, pid, datos_memoria,tamanio_contenido, 0, direccionLogica, direccionFisica);
                pthread_mutex_unlock(mutex_cache);

                
            }
            else
            {
                log_error(cpu_logger, "Error al ejecutar READ: No se pudo encontrar la pagina en memoria ni en TLB.");
            }
        }
    }
    else if (ENTRADAS_CACHE <= 0)
    {

        if (ENTRADAS_TLB > 0 && tlb_hit_result != -1)
        {

            int marco = tlb_hit_result;
            log_info(cpu_logger, " PID: <%d> - OBTENER MARCO - Pagina: <%d> - Marco: <%d>", pid, numero_pagina, marco);
            uint32_t direccionFisica = mmu(tamanioPagina, marco, direccionLogica);
            t_buffer *buffer_read_memoria = crear_buffer();
            cargar_uint32_al_buffer(buffer_read_memoria, pid);
            cargar_uint32_al_buffer(buffer_read_memoria, direccionFisica);
            cargar_int_al_buffer(buffer_read_memoria, tamanio);
            t_paquete *paquete_read = crear_paquete(REQUEST_READ, buffer_read_memoria);

            enviar_paquete(paquete_read, fd_memoria);
            // esperar respuesta de memoria
            sem_wait(sem_pagina);
            pthread_mutex_lock(mutex_tlb);
            actualizar_tlb(tlb, numero_pagina, pid, marco);
            pthread_mutex_unlock(mutex_tlb);

            leer_void_desde_memoria(datos_memoria, tamanio_contenido, direccionFisica);
        }
        else if (ENTRADAS_TLB > 0 && tlb_hit_result == -1)
        {

            int marco = buscar_en_MP(numero_pagina, entradasPorTabla, cantidadDeNiveles, tamanioPagina, direccionLogica, pid);
            log_info(cpu_logger, " PID: <%d> - OBTENER MARCO - Pagina: <%d> - Marco: <%d>", pid, numero_pagina, marco);
            uint32_t direccionFisica = mmu(tamanioPagina, marco, direccionLogica);
            t_buffer *buffer_read_memoria = crear_buffer();
            cargar_uint32_al_buffer(buffer_read_memoria, pid);
            cargar_int_al_buffer(buffer_read_memoria, direccionFisica);
            cargar_int_al_buffer(buffer_read_memoria, tamanio);
            t_paquete *paquete_read = crear_paquete(REQUEST_READ, buffer_read_memoria);
            enviar_paquete(paquete_read, fd_memoria);
            // esperar respuesta de memoria
            sem_wait(sem_pagina);
            pthread_mutex_lock(mutex_tlb);
            actualizar_tlb(tlb, numero_pagina, pid, marco);
            pthread_mutex_unlock(mutex_tlb);

            leer_void_desde_memoria(datos_memoria, tamanio_contenido, direccionFisica);
        }
        else if (ENTRADAS_TLB == 0)
        {

            int marco = buscar_en_MP(numero_pagina, entradasPorTabla, cantidadDeNiveles, tamanioPagina, direccionLogica, pid);
            log_info(cpu_logger, " PID: <%d> - OBTENER MARCO - Pagina: <%d> - Marco: <%d>", pid, numero_pagina, marco);
            uint32_t direccionFisica = mmu(tamanioPagina, marco, direccionLogica);
            t_buffer *buffer_read_memoria = crear_buffer();
            cargar_uint32_al_buffer(buffer_read_memoria, pid);
            cargar_int_al_buffer(buffer_read_memoria, direccionFisica);
            cargar_int_al_buffer(buffer_read_memoria, tamanio);
            t_paquete *paquete_read = crear_paquete(REQUEST_READ, buffer_read_memoria);
            enviar_paquete(paquete_read, fd_memoria);
            // esperar respuesta de memoria

            sem_wait(sem_pagina);

            leer_void_desde_memoria(datos_memoria, tamanio_contenido, direccionFisica);

            
        }
    }
}

void leer_void_desde_memoria(void* data, size_t size, uint32_t direccionFisica) {
    // Loguear como string (puede cortarse en \0 intermedio)
    char* temp_str = malloc(size + 1);
    memcpy(temp_str, data, size);
    temp_str[size] = '\0';

    // Loguear como bytes/hex para ver el resultado real
    char* buffer_hex = malloc(size * 3 + 1);
    buffer_hex[0] = '\0';
    for (size_t i = 0; i < size; i++) {
        sprintf(buffer_hex + strlen(buffer_hex), "%02X ", ((unsigned char*)data)[i]);
    }
    log_trace(cpu_logger, "Contenido de memoria como hex: %s", buffer_hex);

    // Loguear como texto plano (sin cortar en \0, muestra todo como está)
    log_trace(cpu_logger, "Contenido de memoria como texto: ");
    fwrite(data, 1, size, stdout);
    printf("\n");

    log_info(cpu_logger, " PID: <%d> - Accion: LEER - Direccion Fisica: <%d> - Datos: <%s>", pid, direccionFisica,data);

    free(temp_str);
    free(buffer_hex);
    
}

void mostrar_cache(t_cache cache) {

        if (cache.contenido != NULL) {
            // Mostrar como string (puede cortarse en \0)
            char temp_str[tamanioPagina + 1];
            memcpy(temp_str, cache.contenido, tamanioPagina);
            temp_str[tamanioPagina] = '\0';
            log_info(cpu_logger, "Cache como string: '%s'", temp_str);

            // Mostrar como hex
            char buffer_hex[tamanioPagina * 3 + 1];
            buffer_hex[0] = '\0';
            for (int j = 0; j < tamanioPagina; j++) {
                sprintf(buffer_hex + strlen(buffer_hex), "%02X ", ((unsigned char*)cache.contenido)[j]);
            }
            log_info(cpu_logger, "Cache como hex: %s", buffer_hex);
        } else {
            log_info(cpu_logger, "Cache vacía.");
        }
    
}


void cargar_contenido(t_cache *cache, t_tlb *tlb, int indice, char *contenido, size_t tamanio_cont, uint32_t direccionFisica, int direccionLogica){

    int desplazamiento = calcular_desplazamiento(direccionLogica);
    int numero_de_pagina = nro_pagina(direccionLogica, tamanioPagina);
    limpiar_salto_de_linea(contenido);

    if (cache[indice].contenido != NULL) {
        free(cache[indice].contenido);
    }
    cache[indice].contenido = calloc(1, tamanioPagina);

    if(cache_hit(numero_de_pagina, cache) != -1){
        limpiar_y_copiar_a_cache(cache[indice].contenido, cache, indice);
        memcpy((char*)cache[indice].contenido + desplazamiento, contenido, strlen(contenido));
        limpiar_y_copiar_a_cache(cache[indice].contenido, cache, indice);

    } else {
        limpiar_y_copiar_a_cache(cache[indice].contenido, cache, indice);
        // Copiamos primero el contenido recibido de memoria
        memset((char*)cache[indice].contenido, 0, tamanioPagina); // Limpiamos el contenido

        memcpy(cache[indice].contenido, datos_memoria, tamanio_contenido);
        // Luego escribimos el nuevo contenido a partir del desplazamiento
        memcpy((char*)cache[indice].contenido + desplazamiento, contenido, strlen(contenido));
        limpiar_y_copiar_a_cache(cache[indice].contenido, cache, indice);
    }

}

void limpiar_y_copiar_a_cache(void* pagina, t_cache *cache, int indice) {
    unsigned char* bytes = (unsigned char*)pagina;
    // Si empieza con 0x20 0x00 0x00 0x00, quita el header y rellena
    if (bytes[0] == 0x20 && bytes[1] == 0x00 && bytes[2] == 0x00 && bytes[3] == 0x00) {
        memmove(bytes, bytes + 4, tamanioPagina - 4);
        memset(bytes + tamanioPagina - 4, 0, 4);
    }
    // Copia el contenido limpio a la caché
    memcpy(cache[indice].contenido, bytes, tamanioPagina);
}

void limpiar_salto_de_linea(char* str) {
    if (!str) return;
    size_t len = strlen(str);
    if (len > 0 && str[len-1] == '\n') {
        str[len-1] = '\0';
    }
} 


int nro_pagina(int direccionLogica, int tamanioPagina)
{

    int numero = floor(direccionLogica / tamanioPagina);

    return numero;
}

int *entrada_nivel_X(int numero_Pagina, int cant_entradas_tabla, int cant_niveles)
{
    int *entradasNivel = malloc(cant_niveles * sizeof(int));
    for (int i = 0; i < cant_niveles; i++)
    {
        int divisor = pow(cant_entradas_tabla, cant_niveles - i - 1);
        entradasNivel[i] = (numero_Pagina / divisor) % cant_entradas_tabla;
    }
    return entradasNivel;
}

void algoritmo_tlb_fifo(t_tlb *tlb, int nro_pagina, u_int32_t pid, int marco)
{

    int ind = pagina_vacia_tlb(tlb);
    int pag = nro_pagina_en_tlb(tlb, nro_pagina, pid);
    int indice_mas_antiguo = pagina_mas_antigua(tlb);

    if (ind != -1 && pag == -1)
    { // busca entrada vacia

        tlb[ind].nro_pagina = nro_pagina;
        tlb[ind].pid = pid;
        tlb[ind].marco = marco;
        tlb[ind].tiempo_en_tlb = 0;
    }
    else if (pag != -1)
    { // busca entrada existente

        tlb[pag].nro_pagina = nro_pagina; // Actualizamos el marco
        tlb[pag].pid = pid;
        tlb[pag].marco = marco;
    }
    else
    { // reemplaza la mas antigua

        tlb[indice_mas_antiguo].nro_pagina = nro_pagina;
        tlb[indice_mas_antiguo].pid = pid;
        tlb[indice_mas_antiguo].marco = marco;
        tlb[indice_mas_antiguo].tiempo_en_tlb = 0;
    }
    for (int i = 0; i <= ENTRADAS_TLB - 1; i++)
    {
        if (tlb[i].nro_pagina != -1 && tlb[i].pid != -1)
        {
            tlb[i].tiempo_en_tlb++;
        }
    }
}

void algoritmo_tlb_lru(t_tlb *tlb, int nro_pagina, u_int32_t pid, int marco)
{
    if (pagina_vacia_tlb(tlb) != -1 && nro_pagina_en_tlb(tlb, nro_pagina, pid) == -1)
    { // busca entrada vacia
        int indice_vacio = pagina_vacia_tlb(tlb);
        log_trace(cpu_logger, "pagina vacia ");
        tlb[indice_vacio].nro_pagina = nro_pagina;
        tlb[indice_vacio].pid = pid;
        tlb[indice_vacio].marco = marco;
        tlb[indice_vacio].tiempo_en_tlb = 0;
    }
    else if (nro_pagina_en_tlb(tlb, nro_pagina, pid) != -1)
    { // busca entrada existente
        log_trace(cpu_logger, "tlb_hit");
        int nro = nro_pagina_en_tlb(tlb, nro_pagina, pid);
        tlb[nro].nro_pagina = nro_pagina;
        tlb[nro].pid = pid;
        tlb[nro].marco = marco;
        tlb[nro].tiempo_en_tlb = 0;
    }
    else
    { // reemplaza la mas antigua
        log_trace(cpu_logger, "no esta en tlb");
        int indice_mas_antiguo = pagina_mas_antigua(tlb);
        tlb[indice_mas_antiguo].nro_pagina = nro_pagina;
        tlb[indice_mas_antiguo].pid = pid;
        tlb[indice_mas_antiguo].marco = marco;
        tlb[indice_mas_antiguo].tiempo_en_tlb = 0;
    }
    for (int i = 0; i <= ENTRADAS_TLB - 1; i++)
    {
        if (tlb[i].nro_pagina != -1 && tlb[i].pid != -1)
        {
            tlb[i].tiempo_en_tlb++;
        }
    }
}

int pagina_vacia_tlb(t_tlb *tlb)
{
    for (int i = 0; i <= ENTRADAS_TLB - 1; i++)
    {
        if (tlb[i].nro_pagina == -1 && tlb[i].pid == -1)
        {
            return i; // Retorna el índice de la entrada vacía
        }
    }
    return -1; // No hay entradas vacías
}

int nro_pagina_en_tlb(t_tlb *tlb, int nro_pagina, u_int32_t pid)
{
    for (int i = 0; i <= ENTRADAS_TLB - 1; i++)
    {
        if (tlb[i].nro_pagina == nro_pagina && tlb[i].pid == pid)
        {
            return i; // La página está en TLB
        }
    }
    return -1; // La página no está en TLB
}

int pagina_mas_antigua(t_tlb *tlb)
{
    int indice_mas_antiguo = 0;
    int tiempo_mas_antiguo = tlb[0].tiempo_en_tlb;

    for (int i = 1; i <= ENTRADAS_TLB - 1; i++)
    {
        if (tiempo_mas_antiguo > tlb[i].tiempo_en_tlb)
        {
        }
        else
        {
            indice_mas_antiguo = i;
            tiempo_mas_antiguo = tlb[i].tiempo_en_tlb;
        }
    }
    return indice_mas_antiguo;
}

int pagina_mas_antigua_cache(t_cache *cache)
{
    int indice_mas_antiguo = 0;
    int tiempo_mas_antiguo = cache[0].tiempo_en_cache;

    for (int i = 1; i <= ENTRADAS_CACHE - 1; i++)
    {
        if (tiempo_mas_antiguo > cache[i].tiempo_en_cache)
        {
        }
        else
        {
            indice_mas_antiguo = i;
            tiempo_mas_antiguo = cache[i].tiempo_en_cache;
        }
    }
    return indice_mas_antiguo;
}

int pagina_vacia_cache(t_cache *cache)
{
    for (int i = 0; i <= ENTRADAS_CACHE - 1; i++)
    {
        if (cache[i].nro_pagina == -1 && cache[i].pid == -1)
        {
            return i;
        }
    }
    return -1; // No hay entradas vacías
}

int nro_pagina_en_cache(t_cache *cache, int nro_pagina, u_int32_t pid)
{
    for (int i = 0; i <= ENTRADAS_CACHE - 1; i++)
    {
        if (cache[i].nro_pagina == nro_pagina && cache[i].pid == pid)
        {
            return i;
        }
    }
    return -1; // La página no está en la cache
}



void actualizar_cache(t_cache *cache, t_tlb *tlb, int nro_pagina, u_int32_t pid, char *contenido,size_t size_contenido, int modificado, int direccionLogica, uint32_t direccionFisica)
{
    usleep(RETARDO_CACHE * 1000);
    if (strcmp(REEMPLAZO_CACHE, "CLOCK") == 0)
    {
        algoritmo_cache_clock(cache, tlb, nro_pagina, pid, contenido,size_contenido, modificado, direccionLogica, direccionFisica);
    }
    else if (strcmp(REEMPLAZO_CACHE, "CLOCK_MODIFICADO") == 0)
    {
        algoritmo_cache_clock_modificado(cache, tlb, nro_pagina, pid, contenido,size_contenido, modificado, direccionLogica, direccionFisica);
    }
    else
    {
        log_trace(cpu_logger, "Algoritmo de reemplazo de cache no soportado: %s", REEMPLAZO_CACHE);
        exit(EXIT_FAILURE);
    }
    
     for (int i = 0; i <= ENTRADAS_CACHE - 1; i++)
    {
        log_trace(cpu_logger, "Cache[%d]: nro_pagina=%d, pid=%d, contenido=%s, bit_uso=%d, modificado=%d, tiempo_en_cache=%d",
                 i, cache[i].nro_pagina, cache[i].pid, (char *)cache[i].contenido, cache[i].bit_uso, cache[i].modificado, cache[i].tiempo_en_cache);
    }
        log_info(cpu_logger, " PID: <%d> - Cache Add - Pagina: <%d> ", pid, nro_pagina); 
}

void actualizar_tlb(t_tlb *tlb, int nro_pagina, u_int32_t pid, int marco)
{

    if (strcmp(REEMPLAZO_TLB, "FIFO") == 0)
    {
        algoritmo_tlb_fifo(tlb, nro_pagina, pid, marco);
    }
    else if (strcmp(REEMPLAZO_TLB, "LRU") == 0)
    {
        algoritmo_tlb_lru(tlb, nro_pagina, pid, marco);
    }
    else
    {
        log_error(cpu_logger, "Algoritmo de reemplazo de TLB no soportado: %s", REEMPLAZO_TLB);
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i <= ENTRADAS_TLB - 1; i++)
    {
        log_info(cpu_logger, "TLB[%d]: nro_pagina=%d, pid=%d, marco=%d, tiempo_en_tlb=%d",
                 i, tlb[i].nro_pagina, tlb[i].pid, tlb[i].marco, tlb[i].tiempo_en_tlb);
    }
}

void algoritmo_cache_clock(t_cache *cache, t_tlb *tlb, int nro_pagina, u_int32_t pid, char *contenido,size_t size_contenido , int modificado, int direccionLogica, uint32_t direccionFisica)
{

    int pag = pagina_vacia_cache(cache);
    int nro = nro_pagina_en_cache(cache, nro_pagina, pid);

    if (pag != -1 && nro == -1)
    { // Busca una entrada vacía

        log_trace(cpu_logger, "HAY ESPACIO VACIO Y NO ESTA EN CACHE");

        cargar_contenido(cache, tlb, pag, contenido, size_contenido, direccionFisica, direccionLogica);

        cache[pag].nro_pagina = nro_pagina;
        cache[pag].pid = pid;

        cache[pag].bit_uso = 1;
        cache[pag].modificado = modificado;
        cache[pag].tiempo_en_cache = 0;
        cache[pag].direccionFisica = direccionFisica;

        for (int i = 0; i <= ENTRADAS_CACHE - 1; i++)
        {
            if (cache[i].nro_pagina != -1 && cache[i].pid != -1)
            {
                cache[i].tiempo_en_cache++;
            }
        }
        mostrar_cache(cache[pag]);
    }
    else if (nro != -1)
    { // Busca una entrada existente
        cargar_contenido(cache, tlb, nro, contenido, size_contenido, direccionFisica, direccionLogica);

        cache[nro].nro_pagina = nro_pagina;
        cache[nro].pid = pid;

        cache[nro].bit_uso = 1;
        cache[nro].modificado = modificado;
        cache[nro].direccionFisica = direccionFisica;

        mostrar_cache(cache[nro]);
    }
    else
    { // Reemplaza la página usando el algoritmo CLOCK
        log_trace(cpu_logger, "NO HAY CACHE HIT, REEMPLAZANDO");

        if (todos_bit_uso_en_uno(cache))
        {
            log_info(cpu_logger, "PONIENDO BITS EN 0");
            for (int j = 0; j <= ENTRADAS_CACHE - 1; j++)
            {
                cache[j].bit_uso = 0;
            }
        }
        int indice_mas_antiguo = entrada_bit_uso_0_mas_antigua(cache); // Busca la entrada con bit_uso == 0 más antigua
        int rta_tlb = tlb_hit(cache[indice_mas_antiguo].nro_pagina, cache[indice_mas_antiguo].pid, tlb);

        if (cache[indice_mas_antiguo].nro_pagina != nro_pagina && cache[indice_mas_antiguo].modificado == 1)
        { // Si la página está modificada, se debe escribir en memoria
            log_trace(cpu_logger, "PAGINA MAS ANTIGUA MODIFICADA, ESCRIBIENDO EN MEMORIA");

            if (ENTRADAS_TLB > 0 && rta_tlb != -1)
            {
                int indice = entrada_bit_uso_0_mas_antigua(cache);
                t_buffer *buffer_write = crear_buffer();
                cargar_uint32_al_buffer(buffer_write, cache[indice_mas_antiguo].pid);
                cargar_uint32_al_buffer(buffer_write, cache[indice].direccionFisica);
                cargar_string_al_buffer(buffer_write, cache[indice_mas_antiguo].contenido);
                t_paquete *paquete_write = crear_paquete(REQUEST_WRITE_FULL_PAGE, buffer_write);
                enviar_paquete(paquete_write, fd_memoria);
                
                cache[indice].nro_pagina = nro_pagina;
                cache[indice].pid = pid;

                cargar_contenido(cache, tlb, indice, contenido, size_contenido, direccionFisica, direccionLogica);

                cache[indice].bit_uso = 1;
                cache[indice].modificado = modificado;
                cache[indice].nro_pagina = nro_pagina;
                cache[indice].tiempo_en_cache = 0;
                cache[indice].direccionFisica = direccionFisica;

                mostrar_cache(cache[indice]);
            }
            else if (ENTRADAS_TLB > 0 && rta_tlb == -1)
            {
                int indice2 = entrada_bit_uso_0_mas_antigua(cache);

                t_buffer *buffer_write = crear_buffer();
                cargar_uint32_al_buffer(buffer_write, cache[indice_mas_antiguo].pid);
                cargar_uint32_al_buffer(buffer_write, cache[indice2].direccionFisica);
                cargar_string_al_buffer(buffer_write, cache[indice_mas_antiguo].contenido);
                t_paquete *paquete_write = crear_paquete(REQUEST_WRITE_FULL_PAGE, buffer_write);
                enviar_paquete(paquete_write, fd_memoria);
                
                cargar_contenido(cache, tlb, indice2, contenido, size_contenido, direccionFisica, direccionLogica);
                
                cache[indice2].nro_pagina = nro_pagina;
                cache[indice2].pid = pid;

                cache[indice2].bit_uso = 1;
                cache[indice2].modificado = modificado;
                cache[indice2].nro_pagina = nro_pagina;
                cache[indice2].tiempo_en_cache = 0;
                cache[indice2].direccionFisica = direccionFisica;

                mostrar_cache(cache[indice2]);
            }
            else
            {
                int j = entrada_bit_uso_0_mas_antigua(cache);
                log_info(cpu_logger, "NO HAY TLB");

                t_buffer *buffer_write = crear_buffer();
                cargar_uint32_al_buffer(buffer_write, cache[indice_mas_antiguo].pid);
                cargar_uint32_al_buffer(buffer_write, cache[j].direccionFisica);
                cargar_string_al_buffer(buffer_write, cache[indice_mas_antiguo].contenido);
                t_paquete *paquete_write = crear_paquete(REQUEST_WRITE_FULL_PAGE, buffer_write);
                enviar_paquete(paquete_write, fd_memoria);
                
                
                cargar_contenido(cache, tlb, j, contenido, size_contenido, direccionFisica, direccionLogica);

                cache[j].pid = pid;

                cache[j].bit_uso = 1;
                cache[j].modificado = modificado;
                cache[j].nro_pagina = nro_pagina;
                cache[j].tiempo_en_cache = 0;
                cache[j].direccionFisica = direccionFisica;

                mostrar_cache(cache[j]);
            }

            log_info(cpu_logger, " PID: <%d> - Memory Update - Pagina: <%d> - Frame:", pid, nro_pagina,nro_pagina);

        }else{

            cargar_contenido(cache, tlb, indice_mas_antiguo, contenido, size_contenido, direccionFisica, direccionLogica);

                cache[indice_mas_antiguo].bit_uso = 1;
                cache[indice_mas_antiguo].modificado = modificado;
                cache[indice_mas_antiguo].nro_pagina = nro_pagina;
                cache[indice_mas_antiguo].pid = pid;
                cache[indice_mas_antiguo].tiempo_en_cache = 0;
                cache[indice_mas_antiguo].direccionFisica = direccionFisica;

                mostrar_cache(cache[indice_mas_antiguo]);

        }
        for (int i = 0; i <= ENTRADAS_CACHE - 1; i++)
        {
            if (cache[i].nro_pagina != -1 && cache[i].pid != -1)
            {
                cache[i].tiempo_en_cache++;
            }
        }
    }
}

int entrada_bit_uso_0_mas_antigua(t_cache *cache)
{
    int indice = -1;
    int max_tiempo = -1;

    for (int i = 0; i <= ENTRADAS_CACHE - 1; i++)
    {
        if (cache[i].bit_uso == 0)
        {
            if (indice == -1 || cache[i].tiempo_en_cache > max_tiempo)
            {
                indice = i;
                max_tiempo = cache[i].tiempo_en_cache;
            }
        }
    }
    return indice; // -1 si no hay ninguna con bit_uso == 0
}

int entrada_bit_uso_0_modificado_0_mas_antigua(t_cache *cache)
{
    int indice = -1;
    int max_tiempo = -1;

    for (int i = 0; i <= ENTRADAS_CACHE - 1; i++)
    {
        if (cache[i].bit_uso == 0 && cache[i].modificado == 0)
        {
            if (indice == -1 || cache[i].tiempo_en_cache > max_tiempo)
            {
                indice = i;
                max_tiempo = cache[i].tiempo_en_cache;
            }
        }
    }
    return indice; // -1 si no hay ninguna con bit_uso == 0 y modificado == 0
}

int entrada_bit_uso_0_modificado_1_mas_antigua(t_cache *cache)
{
    int indice = -1;
    int max_tiempo = -1;

    for (int i = 0; i <= ENTRADAS_CACHE - 1; i++)
    {
        if (cache[i].bit_uso == 0 && cache[i].modificado == 1)
        {
            if (indice == -1 || cache[i].tiempo_en_cache > max_tiempo)
            {
                indice = i;
                max_tiempo = cache[i].tiempo_en_cache;
            }
        }
    }
    return indice; // -1 si no hay ninguna con bit_uso == 0 y modificado == 1
}

bool todos_bit_uso_en_uno(t_cache *cache)
{
    for (int i = 0; i <= ENTRADAS_CACHE - 1; i++)
    {
        if (cache[i].bit_uso != 1)
        {
            return false;
        }
    }
    return true;
}

void algoritmo_cache_clock_modificado(t_cache *cache, t_tlb *tlb, int nro_pagina, u_int32_t pid, char *contenido,size_t size_contenido, int modificado, int direccionLogica, uint32_t direccionFisica)
{
    int pagi = pagina_vacia_cache(cache);
    int nro = nro_pagina_en_cache(cache, nro_pagina, pid);
    if (pagi != -1 && nro == -1)
    { // Busca una entrada vacía
        log_trace(cpu_logger, "HAY ESPACIO VACIO Y NO ESTA EN CACHE");

        cargar_contenido(cache, tlb, pagi, contenido,size_contenido, direccionFisica, direccionLogica);

        cache[pagi].nro_pagina = nro_pagina;
        cache[pagi].pid = pid;

        cache[pagi].bit_uso = 1;
        cache[pagi].modificado = modificado;
        cache[pagi].tiempo_en_cache = 0;
        cache[pagi].direccionFisica = direccionFisica;

        for (int i = 0; i <= ENTRADAS_CACHE - 1; i++)
        {
            if (cache[i].nro_pagina != -1 && cache[i].pid != -1)
            {
                cache[i].tiempo_en_cache++;
            }
        }
        mostrar_cache(cache[pagi]);
    }
    else if (nro != -1)
    { // Busca una entrada existente
        log_trace(cpu_logger, "HAY CACHE HIT");

        cargar_contenido(cache, tlb, nro, contenido,size_contenido, direccionFisica, direccionLogica);

        cache[nro].nro_pagina = nro_pagina;
        cache[nro].pid = pid;

        cache[nro].bit_uso = 1;
        cache[nro].modificado = modificado;
        cache[nro].direccionFisica = direccionFisica;

        mostrar_cache(cache[nro]);
    }
    else
    { // Reemplaza la página usando el algoritmo CLOCK-M, hacer lo de mandar en memoria etc
        log_trace(cpu_logger, "NO HAY CACHE HIT, REEMPLAZANDO");
        int victima = victima_clock_m(cache, tlb, nro_pagina, pid, contenido, modificado, direccionLogica);

        cargar_contenido(cache, tlb, victima, contenido,size_contenido, direccionFisica, direccionLogica);

        cache[victima].nro_pagina = nro_pagina;
        cache[victima].pid = pid;

        cache[victima].bit_uso = 1;
        cache[victima].modificado = modificado;
        cache[victima].tiempo_en_cache = 0;
        cache[victima].direccionFisica = direccionFisica;

        for (int i = 0; i <= ENTRADAS_CACHE - 1; i++)
        {
            if (cache[i].nro_pagina != -1 && cache[i].pid != -1)
            {
                cache[i].tiempo_en_cache++;
            }
        }
        mostrar_cache(cache[victima]);
    }
}

int victima_clock_m(t_cache *cache, t_tlb *tlb, int nro_pagina, u_int32_t pid, char *contenido, int modificado, int direccionLogica)
{
    int indice = -1;
    int max_tiempo = -1;

    // Paso 1: Buscar (0,0) más antiguo
    for (int i = 0; i <= ENTRADAS_CACHE - 1; i++)
    {
        if (cache[i].bit_uso == 0 && cache[i].modificado == 0)
        {
            if (indice == -1 || cache[i].tiempo_en_cache > max_tiempo)
            {
                indice = i;
                max_tiempo = cache[i].tiempo_en_cache;
            }
        }
    }
    if (indice != -1)
        return indice;

    // Paso 2: Buscar (0,1) más antiguo
    indice = -1;
    max_tiempo = -1;
    for (int i = 0; i <= ENTRADAS_CACHE - 1; i++)
    {
        if (cache[i].bit_uso == 0 && cache[i].modificado == 1)
        {
            if (indice == -1 || cache[i].tiempo_en_cache > max_tiempo)
            {
                indice = i;
                max_tiempo = cache[i].tiempo_en_cache;
            }
        }
    }
    if (existe_0_1(cache))
    {
        int rta_tlb = tlb_hit(nro_pagina, pid, tlb);
        int indice=entrada_bit_uso_0_modificado_1_mas_antigua(cache);
        if (ENTRADAS_TLB > 0 && rta_tlb != -1)
        {
            int marco = rta_tlb;
            log_info(cpu_logger, " PID: <%d> - OBTENER MARCO - Pagina: <%d> - Marco: <%d>", pid, nro_pagina, marco);
            uint32_t direccionFisica = marco * tamanioPagina;
            t_buffer *buffer_write = crear_buffer();
            cargar_uint32_al_buffer(buffer_write, pid);
            cargar_uint32_al_buffer(buffer_write, cache[indice].direccionFisica);
            cargar_string_al_buffer(buffer_write, cache[indice].contenido);
            t_paquete *paquete_write = crear_paquete(REQUEST_WRITE_FULL_PAGE, buffer_write);
            enviar_paquete(paquete_write, fd_memoria);
            //
            log_info(cpu_logger, " PID: <%d> - Memory Update - Pagina: <%d> - Frame: <%d>", pid, nro_pagina,marco);
        }
        else if (ENTRADAS_TLB > 0 && tlb_hit(nro_pagina, pid, tlb) == -1)
        {
            int marco = buscar_en_MP(nro_pagina, entradasPorTabla, cantidadDeNiveles, tamanioPagina, direccionLogica, pid);
            log_info(cpu_logger, " PID: <%d> - OBTENER MARCO - Pagina: <%d> - Marco: <%d>", pid, nro_pagina, marco);
            actualizar_tlb(tlb, nro_pagina, pid, marco);
            uint32_t direccionFisica = marco * tamanioPagina;
            t_buffer *buffer_write = crear_buffer();
            cargar_uint32_al_buffer(buffer_write, pid);
            cargar_uint32_al_buffer(buffer_write, cache[indice].direccionFisica);
            cargar_string_al_buffer(buffer_write, cache[indice].contenido);
            t_paquete *paquete_write = crear_paquete(REQUEST_WRITE_FULL_PAGE, buffer_write);
            enviar_paquete(paquete_write, fd_memoria);
            log_info(cpu_logger, " PID: <%d> - Memory Update - Pagina: <%d> - Frame: <%d>", pid, nro_pagina,marco);
        }
        else
        {
            int marco = buscar_en_MP(nro_pagina, entradasPorTabla, cantidadDeNiveles, tamanioPagina, direccionLogica, pid);
            log_info(cpu_logger, " PID: <%d> - OBTENER MARCO - Pagina: <%d> - Marco: <%d>", pid, nro_pagina, marco);
            uint32_t direccionFisica = marco * tamanioPagina;
            t_buffer *buffer_write = crear_buffer();
            cargar_uint32_al_buffer(buffer_write, pid);
            cargar_uint32_al_buffer(buffer_write, cache[indice].direccionFisica);
            cargar_string_al_buffer(buffer_write, cache[indice].contenido);
            t_paquete *paquete_write = crear_paquete(REQUEST_WRITE_FULL_PAGE, buffer_write);
            enviar_paquete(paquete_write, fd_memoria);
            log_info(cpu_logger, " PID: <%d> - Memory Update - Pagina: <%d> - Frame: <%d>", pid, nro_pagina,marco);
        }
        
    }
    if (indice != -1)
        return indice;

    // Paso 3: Poner todos los bit_uso en 0 y repetir
    for (int i = 0; i <= ENTRADAS_CACHE - 1; i++)
    {
        cache[i].bit_uso = 0;
    }
    // Repetir el ciclo
    return victima_clock_m(cache, tlb, nro_pagina, pid, contenido, modificado, direccionLogica);
}

bool existe_0_1(t_cache *cache)
{
    for (int i = 0; i <= ENTRADAS_CACHE - 1; i++)
    {
        if (cache[i].bit_uso == 0 && cache[i].modificado == 1)
        {
            return true;
        }
    }
    return false;
}

void desalojar_proceso_tlb(t_tlb *tlb)
{

    for (int i = 0; i <= ENTRADAS_TLB - 1; i++)
    {
        tlb[i].nro_pagina = -1;
        tlb[i].pid = -1;
        tlb[i].marco = -1;
        tlb[i].tiempo_en_tlb = 0;
    }
}

void desalojar_proceso_cache(t_cache *cache)
{

    for (int i = 0; i <= ENTRADAS_CACHE - 1; i++)
    {

        if (cache[i].modificado == 1 && cache[i].pid != -1 && cache[i].nro_pagina != -1)
        {
            if (ENTRADAS_TLB > 0 && nro_pagina_en_tlb(tlb, cache[i].nro_pagina, cache[i].pid) != -1)
            {
                int marco = tlb_hit(cache[i].nro_pagina, cache[i].pid, tlb);
                log_info(cpu_logger, " PID: <%d> - OBTENER MARCO - Pagina: <%d> - Marco: <%d>", pid, nro_pagina, marco);
                uint32_t direccionFisica = marco * tamanioPagina;
                t_buffer *buffer_write = crear_buffer();
                cargar_uint32_al_buffer(buffer_write, cache[i].pid);
                cargar_uint32_al_buffer(buffer_write, cache[i].direccionFisica);
                cargar_string_al_buffer(buffer_write, cache[i].contenido);
                t_paquete *paquete_write = crear_paquete(REQUEST_WRITE_FULL_PAGE, buffer_write);
                enviar_paquete(paquete_write, fd_memoria);
            }
            else if (ENTRADAS_TLB > 0 && nro_pagina_en_tlb(tlb, cache[i].nro_pagina, cache[i].pid) == -1)
            {
                int marco = buscar_en_MP(cache[i].nro_pagina, entradasPorTabla, cantidadDeNiveles, tamanioPagina, 0, cache[i].pid);
                log_info(cpu_logger, " PID: <%d> - OBTENER MARCO - Pagina: <%d> - Marco: <%d>", pid, nro_pagina, marco);
                uint32_t direccionFisica = marco * tamanioPagina;
                t_buffer *buffer_write = crear_buffer();
                cargar_uint32_al_buffer(buffer_write, cache[i].pid);
                cargar_uint32_al_buffer(buffer_write, cache[i].direccionFisica);
                cargar_string_al_buffer(buffer_write, cache[i].contenido);
                t_paquete *paquete_write = crear_paquete(REQUEST_WRITE_FULL_PAGE, buffer_write);
                enviar_paquete(paquete_write, fd_memoria);
            }
            else
            {
                int marco = buscar_en_MP(cache[i].nro_pagina, entradasPorTabla, cantidadDeNiveles, tamanioPagina, 0, cache[i].pid);
                log_info(cpu_logger, " PID: <%d> - OBTENER MARCO - Pagina: <%d> - Marco: <%d>", pid, nro_pagina, marco);
                uint32_t direccionFisica = marco * tamanioPagina;
                t_buffer *buffer_write = crear_buffer();
                cargar_uint32_al_buffer(buffer_write, cache[i].pid);
                cargar_uint32_al_buffer(buffer_write, cache[i].direccionFisica);
                cargar_string_al_buffer(buffer_write, cache[i].contenido);
                t_paquete *paquete_write = crear_paquete(REQUEST_WRITE_FULL_PAGE, buffer_write);
                enviar_paquete(paquete_write, fd_memoria);
            }
        }
        // Eliminar la entrada de la caché
        cache[i].nro_pagina = -1;
        cache[i].pid = -1;
        if (cache[i].contenido)
        {
            free(cache[i].contenido);
            cache[i].contenido = NULL;
        }
        cache[i].bit_uso = 0;
        cache[i].modificado = 0;
        cache[i].tiempo_en_cache = 0;
    }
}

void desalojar_proceso(t_tlb *tlb, t_cache *cache)
{
    pthread_mutex_lock(mutex_tlb);
    desalojar_proceso_tlb(tlb);
    pthread_mutex_unlock(mutex_tlb);

    // Desalojar Cache
    pthread_mutex_lock(mutex_cache);
    desalojar_proceso_cache(cache);
    pthread_mutex_unlock(mutex_cache);
}

char *bytes_a_string(uint8_t *data, size_t len)
{
    // Cada byte ocupará 3 caracteres: "FF " + '\0' final
    char *str = malloc(len * 3 + 1);
    if (!str)
        return NULL;

    char *ptr = str;
    for (size_t i = 0; i < len; i++)
    {
        sprintf(ptr, "%02X ", data[i]); // escribe en ptr
        ptr += 3;                       // avanza el puntero
    }
    ptr = '\0'; // null-terminar
    return str;
}
