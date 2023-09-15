//  Edgar Cop� Rubio,  edgarcoperubio@usal.es https://github.com/edcoru
//  Juan Calles Rivas,  juan.calles@usal.es	https://github.com/RustinceHD/
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "lomo.h"

#define NUM_BUCLES 12

typedef struct bucles
{
    int casillas_totales;
    int casilla_disponibles;
} bucles;

typedef struct
{
    int id_buzon;
    int id_semaforo;
    int nTrenes;
    int referencia_mapa_entero;
    int *referencia_mapa_puntero;
    int *arrayHijos;
    bucles *grafos;
    int indice;
    int hijosActuales;
    int x;
    int y;
} datos_manejadora;
// Estructura para el manejo de los parámetros necesarios en la manejadora

typedef union semun
{
    int val;
    struct semid_ds *buf;
    unsigned *array;
} uni;
// Union necesaria para utilizar la funcion semctl en encina

datos_manejadora data_handler;

// Cabeceras de las funciones necesarias
int salida(int);
void liberar_recursos_ipc(int, int, int, void *);
void handler(int);
int llenar_blucles(bucles *);
int lib_inter(int, int, bucles *, int);
int mirar_inter(int, int, bucles *, int);
int wait_sem(int, bucles *, int);
int pre_wait_sem(int, bucles *, int);
int signal_sem(int, bucles *, int);

int main(int argc, char *argv[])
{
    // int* mapaCompartido;
    struct sigaction sa, antiguo; // definir una estructura sigaction
    sa.sa_handler = handler;      // asignar la función handler al campo sa_handler
    sa.sa_flags = 0;              // indicar que se usará el campo sa_sigaction
    struct sembuf wait_sem_sembuf;
    wait_sem_sembuf.sem_num = 1;
    wait_sem_sembuf.sem_flg = 0;
    wait_sem_sembuf.sem_op = -1;
    struct sembuf signal_sem_sembuf;
    signal_sem_sembuf.sem_num = 1;
    signal_sem_sembuf.sem_flg = 0;
    signal_sem_sembuf.sem_op = 1;
    struct sembuf signal_sem_padre;
    signal_sem_padre.sem_num = 1;
    signal_sem_padre.sem_flg = 0;
    // Creamos las estructuras necesarias para ayudar en el manejo de los semáforos

    sigaction(SIGINT, &sa, &antiguo);

    uni sem_ini, sem_padre, sem_ini_0;
    sem_ini.val = 1;
    sem_ini_0.val = 0;
    int *mapaCompartido;
    int flag = 0;
    pid_t pid;
    int buzon, semaforo, mapa, retardo, nTrenes, cordx, cordy, i, j, k, l, luz_roja, luz_verde, tamanoTren, preX, preY;
    struct mensaje tren, hueco;
    tamanoTren = 19;
    // Declaramos las variables necesarias para el programa

    if (argc > 3)
    {
        salida(-99);
    }

    if (argc == 2 && strcmp(argv[1], "--mapa") == 0)
    {
        if (-1 == LOMO_generar_mapa("i6743891", "i0964742"))
            salida(-98);
        return 0;
    }
    else if (argc == 3 && atoi(argv[1]) >= 0 && atoi(argv[2]) >= 1 && atoi(argv[2]) <= 100)
    {
        retardo = atoi(argv[1]);
        nTrenes = atoi(argv[2]);
    }
    else
    {
        salida(-99);
    }
    // Realizamos todas las comprobaciones necesarias de los argumentos

    data_handler.nTrenes = nTrenes;
    data_handler.arrayHijos = malloc(nTrenes * sizeof(pid_t));
    if (data_handler.arrayHijos == NULL)
    {
        salida(-96);
    }
    // Creamos una matriz dinámica para alamacenar los PIDs de los hijos y comprobamos errores
    buzon = msgget(IPC_PRIVATE, IPC_CREAT | 0600);
    // Creamos el buzon necesario para la biblioteca
    semaforo = semget(IPC_PRIVATE, 2 + nTrenes, IPC_CREAT | 0600);
    // Creamos los semáforos necesarios para la práctica
    mapa = shmget(IPC_PRIVATE, (75 * 17 * sizeof(int)) + (nTrenes * sizeof(int)) + (NUM_BUCLES * sizeof(bucles)), IPC_CREAT | 0600);
    mapaCompartido = shmat(mapa, 0, 0);
    data_handler.arrayHijos = &(mapaCompartido[(75 * 17)]);
    data_handler.grafos = (void *)&(data_handler.arrayHijos[(nTrenes)]);
    // Creamos la memoria compartida y obtenemos una referencia a ellas

    if (mapa < 0 || semaforo < 0 || buzon < 0 || mapaCompartido == NULL)
    {
        liberar_recursos_ipc(buzon, semaforo, mapa, mapaCompartido);
        data_handler.arrayHijos = NULL;
        salida(-97);
    }
    // Comprobamos que no hayan ocurrido errores en la creación de los recursos IPC y los liberamos en ese caso
    data_handler.id_buzon = buzon;
    data_handler.id_semaforo = semaforo;

    data_handler.referencia_mapa_entero = mapa;
    data_handler.referencia_mapa_puntero = mapaCompartido;

    for (i = 0; i < 16; i++)
    {
        for (j = 0; j < 74; j++)
        {
            mapaCompartido[i * 75 + j] = 0;
        }
    }

    llenar_blucles(data_handler.grafos);
    for (i = 1; i <= nTrenes; i++)
        semctl(semaforo, i, SETVAL, sem_ini_0);
    sem_padre.val = 0;
    semctl(semaforo, nTrenes + 1, SETVAL, sem_padre);
    semctl(semaforo, 1, SETVAL, sem_ini);
    signal_sem_padre.sem_op = nTrenes;
    // Inicializamos los semáforos necesarios para la ejecucuión de los trenes
    //  semctl(semaforo,2,SETVAL,sem_ini);
    LOMO_inicio(retardo, semaforo, buzon, "i6743891", "i0964742");

    // crear los hijos que se han pasado por parametro y que serán trenecitos
    for (data_handler.indice = 0; data_handler.indice < nTrenes; data_handler.indice++)
    {
        pid = fork();
        if (pid == -1)
            salida(-94);
        if (pid == 0)
        {
            // esperan a que el padre guarde todos los pids de los hijos
            sigaction(SIGINT, &antiguo, NULL);
            wait_sem_sembuf.sem_num = nTrenes + 1;
            semop(semaforo, &wait_sem_sembuf, 1);
            // dependiendo de su pid los trenes se les asigna el sem�foro al cual van a hacer
            // wait (luz roja) y signal (luz verde)
            for (l = 0; l < nTrenes; l++)
            {
                if (getpid() == data_handler.arrayHijos[l])
                    break;
            }
            l++;
            luz_roja = l;
            if (luz_roja == nTrenes)
            {
                luz_verde = 1;
            }
            else
            {
                luz_verde = 1 + luz_roja;
            }
            wait_sem_sembuf.sem_num = luz_roja;
            signal_sem_sembuf.sem_num = luz_verde;
            //  wait_sem_sembuf.sem_num = 1;
            //  signal_sem_sembuf.sem_num = 1;
            tren.tipo = TIPO_TRENNUEVO;
            msgsnd(buzon, &tren, sizeof(struct mensaje) - sizeof(long), 0);
            msgrcv(buzon, &tren, sizeof(struct mensaje) - sizeof(long), TIPO_RESPTRENNUEVO, 0);

            tren.tipo = TIPO_PETAVANCE;
            cordy = tren.y;

            preX = 0;
            preY = 0;

            while (1)
            {

                semop(semaforo, &wait_sem_sembuf, 1);

                // if (mirar_inter(preX, preY, data_handler.grafos, tamanoTren) == 1)
                //{

                // fprintf(stderr, "El pid del proceso con el semaforo es : %i \n", getpid());
                tren.tipo = TIPO_PETAVANCE;
                cordy = tren.y;
                msgsnd(buzon, &tren, sizeof(struct mensaje) - sizeof(long), 0);
                // Esperamos a nuestro turno y pedimos movernos
                msgrcv(buzon, &tren, sizeof(struct mensaje) - sizeof(long), TIPO_RESPPETAVANCETREN0 + tren.tren, 0);
                // Nos aceptan movernos y nos indican la posición a la que movernos
                fprintf(stderr, "PID:%6i X:%2i Y:%2i|%i-%3i|%i-%i|%i-%i|%i-%i|%i-%i|%i-%i|%i-%i|%i-%i|%i-%i|%i-%i|%i-%i|%i-%i|\n", getpid(), tren.x, tren.y, data_handler.grafos[0].casillas_totales, data_handler.grafos[0].casilla_disponibles, data_handler.grafos[1].casillas_totales, data_handler.grafos[1].casilla_disponibles, data_handler.grafos[2].casillas_totales, data_handler.grafos[2].casilla_disponibles, data_handler.grafos[3].casillas_totales, data_handler.grafos[3].casilla_disponibles, data_handler.grafos[4].casillas_totales, data_handler.grafos[4].casilla_disponibles, data_handler.grafos[5].casillas_totales, data_handler.grafos[5].casilla_disponibles, data_handler.grafos[6].casillas_totales, data_handler.grafos[6].casilla_disponibles, data_handler.grafos[7].casillas_totales, data_handler.grafos[7].casilla_disponibles, data_handler.grafos[8].casillas_totales, data_handler.grafos[8].casilla_disponibles, data_handler.grafos[9].casillas_totales, data_handler.grafos[9].casilla_disponibles, data_handler.grafos[10].casillas_totales, data_handler.grafos[10].casilla_disponibles, data_handler.grafos[11].casillas_totales, data_handler.grafos[11].casilla_disponibles);
                if (mapaCompartido[tren.y * 75 + tren.x] == 0 && mirar_inter(preX, preY, data_handler.grafos, tamanoTren) == 1)
                {

                    tren.tipo = TIPO_AVANCE;
                    msgsnd(buzon, &tren, sizeof(struct mensaje) - sizeof(long), 0);
                    // Comprobamos que esté libre dicha posición y en ese caso nos movemos
                    mapaCompartido[tren.y * 75 + tren.x] = 1;
                    // lib_inter(tren.x, tren.y, data_handler.grafos, tamanoTren);
                    //   Marcamos esa posición como ocupada
                    cordx = tren.x;
                    preX = tren.x;
                    preY = tren.y;
                    lib_inter(tren.x, tren.y, data_handler.grafos, tamanoTren);
                    msgrcv(buzon, &tren, sizeof(struct mensaje) - sizeof(long), TIPO_RESPAVANCETREN0 + tren.tren, 0);

                    // Recibimos la confirmación del movimiento y las posicion que dejamos libre
                    LOMO_espera(cordy, tren.y);
                    // lib_inter(preX, preY, data_handler.grafos, tamanoTren);
                    if (tren.y >= 0 && tren.x >= 0)
                    {
                        mapaCompartido[tren.y * 75 + tren.x] = 0;

                        if (flag == 0)
                        {
                            flag = 1;
                            tamanoTren = cordx;
                            if (tamanoTren >= 16)
                            {
                                signal_sem(0, data_handler.grafos, 19);
                                tamanoTren = cordx;
                                wait_sem(0, data_handler.grafos, tamanoTren);
                            }
                        }
                    }
                    // Comprobamos que la posición a dejar libre sea valida y si es así la liberamos
                }
                // }
                semop(semaforo, &signal_sem_sembuf, 1);
                // Cedemos el paso al siguiente proceso
            }
        }
        else
        {
            data_handler.hijosActuales++;
            data_handler.arrayHijos[data_handler.indice] = pid;
        }
    }
    sigaction(SIGINT, &sa, NULL);
    pon_error("TRENES A PUNTO DE SALIR");
    // pistoletazo de salida para los trenes
    signal_sem_padre.sem_num = nTrenes + 1;
    semop(semaforo, &signal_sem_padre, 1);
    wait(NULL);
    // Cambiamos el comportamiento del CTRL-C y esperamos a que terminen todos los procesos
    return 0;
}

// Función para los distintos errores que puedan darse durante la ejecución
int salida(int ERROR)
{
    switch (ERROR)
    {
    case -99:
        printf("Error al pasa los argumentos ponga <--mapa> para ver el código html del mapa");
        exit(1);
        break;
    case -98:
        printf("Error en la creación del mapa ");
        exit(-98);
        break;
    case -97:
        perror("Error en la creación de los recursos ipc");
        // liberar el buzón
        exit(-97);
        break;
    case -96:
        perror("Error en la creación del semáforo");
        exit(-96);
        break;
    case -95:
        perror("ERROR A LA HORA DE RESERVAR MEMORIA DINÁMICA");
        exit(-95);
    case -94:
        perror("ERROR A LA HORA DE CREAR LOS HIJOS");
        handler(1);
        exit(-94);
    case -93:
        printf("Error al abrir el archivo.\n");
        exit(-93);

    default:
        break;
    }
}

// Función para liberar todos los recursos IPC
void liberar_recursos_ipc(int buzon_id, int semaforo_id, int mapa_id, void *mapa_puntero)
{
    if (semctl(semaforo_id, 0, IPC_RMID) < 0)
    {
        perror("**ERROR AL LIBERAR EL SEMAFORO**");
    }

    if (msgctl(buzon_id, IPC_RMID, NULL) < 0)
    {
        perror("**Error al liberar el buzón**");
    }
    if (shmdt(mapa_puntero) < 0)
    {
        perror("**Error eliminar la referencia a la memoria compartida**");
    }
    if (shmctl(mapa_id, IPC_RMID, NULL) < 0)
    {
        perror("**Error al liberar la memoria compartida**");
    }
}

// Función manejadora para capturar el CTRL-C
void handler(int sig)
{
    int i = 0, estado, j, k, return_z, hijos[data_handler.nTrenes], flag = 0;

    for (i = 0; i < data_handler.nTrenes; i++)
    {
        hijos[i] = data_handler.arrayHijos[i];
        if (hijos[i] == getpid())
            flag = 1;
    }

    if (flag == 0)
    {
        return_z = LOMO_fin();

        liberar_recursos_ipc(data_handler.id_buzon, data_handler.id_semaforo, data_handler.referencia_mapa_entero, data_handler.referencia_mapa_puntero);

        for (i = 0; i < data_handler.hijosActuales; i++)
            kill(hijos[i], SIGTERM);

        for (i = 0; i < data_handler.hijosActuales; i++)
            waitpid(hijos[i], &estado, 0);

        fprintf(stderr, "ILLO ILLO 14/03/2023 18:27\n");
        data_handler.arrayHijos = NULL;
        data_handler.referencia_mapa_puntero = NULL;

        exit(return_z);
    }
    else
    {
        wait(NULL);
    }
}

int llenar_blucles(bucles *arrayGrafos)
{

    // 1º grafo 16.0 54.16
    // tamaño casillas 106
    // entradas [-15.0 [-55.0 [-55.12 [-15.12
    // salidas  [-15.16 [-15.9 -15.4 [-55.7 [-55.16

    arrayGrafos[0].casillas_totales = arrayGrafos[0].casilla_disponibles = 106;

    // 2º frafo 36.7 54.16
    // tamaño casilla 54
    // entrada [54.6 [55.12 [35.12 [35.7
    // salidas [55.7 [55.16 [35.16

    arrayGrafos[1].casillas_totales = arrayGrafos[1].casilla_disponibles = 54;

    // 3º grafo 16.7 54.16
    // tamaño casilla 90
    // entrada [15.12 [54.6 [55.12
    // salidas [16.6 [15.9 [15.16 [55.16 [55.7

    arrayGrafos[2].casillas_totales = arrayGrafos[2].casilla_disponibles = 90;

    // 4º g5rafo 0.4 16.12
    // tamaño 46
    // entrada [0.13 [16.13
    // salida [17.12 [17.7 [16.3

    arrayGrafos[3].casillas_totales = arrayGrafos[3].casilla_disponibles = 46;

    // 5º GRAFO 0.9 16.12
    // tamaño 36
    // entrada [0.8 [0.13 [16.13
    // salida [17.12 [16.8

    arrayGrafos[4].casillas_totales = arrayGrafos[4].casilla_disponibles = 36;

    // 6º grafo 36.12 54.16
    // tamaño 40
    // entrada [-54.11 [-55.12 [-35.12
    // salida  [-36.11 [-35.16 [-55.16

    arrayGrafos[5].casillas_totales = arrayGrafos[5].casilla_disponibles = 40;

    // 7º GRAFO 54.0 68.7
    // TAMAÑO 41
    // entrada  [53.0 [53.7 [69.0 [68.8 || 53.0 69.0 53.7 68.8
    // salida [54.8 [69.7 ||            || 54.8 69.7

    arrayGrafos[6].casillas_totales = arrayGrafos[6].casilla_disponibles = 41;

    // 8º GRAFO 54.12 68.16
    // tamaño 36
    //  entrada [54.11 [53.12 [69.12 [69.16 || 53.12 54.11 69.16 69.12
    // salida [53.16 [68.11                 || 53.16 68.11

    arrayGrafos[7].casillas_totales = arrayGrafos[7].casilla_disponibles = 35;

    // 9º GRAFO 54.0 74.740
    // TAMAÑO 52
    // entada [53.0 [53.7 [68.6
    // salida [54.8 [74.8

    arrayGrafos[8].casillas_totales = arrayGrafos[8].casilla_disponibles = 52;

    // 10º GRAFO 68.7 74.16
    // tamaño 29
    // entrada [67.7 [67.16
    // salida [67.12 [68.6 [74.6

    arrayGrafos[9].casillas_totales = arrayGrafos[9].casilla_disponibles = 29;

    // 11º GRAFO 68.7 74.12
    // tamaño 26
    // entrada [63.13 [67.7
    // salida [67.12 [68.6 [74.6 [74.13

    arrayGrafos[10].casillas_totales = arrayGrafos[10].casilla_disponibles = 21;

    // 12º GRAFO 54.0 68.16
    // tamaño 58
    // entradas [53.0 [53.7 [53.12 69.16 69.12 69.0
    // salida  [53.16 [69.7
    arrayGrafos[11].casillas_totales = arrayGrafos[11].casilla_disponibles = 58;

    return 0;
}

int wait_sem(int num_bucle, bucles *op, int tamano_tren)
{

    if (op[num_bucle].casilla_disponibles >= tamano_tren)
    {
        // pon_error("HACIENDO WAIT");
        op[num_bucle].casilla_disponibles -= tamano_tren;
        // fprintf(stderr, "WAIT TREN [%i][%i-%i] NUM BUCLE [%i]\n", getpid(), op[num_bucle].casilla_disponibles, op[num_bucle].casillas_totales, num_bucle + 1);
        // fflush(stderr);
        return op[num_bucle].casilla_disponibles;
    }
    else
    {
        char buffer[100];
        sprintf(buffer, "ERROR EN WAIT BUCLE %i TAMANO DEL TREN [%i] TAMANO DEL BUCLE [%i|%i]", num_bucle, tamano_tren, op[num_bucle].casillas_totales, op[num_bucle].casilla_disponibles);
        pon_error(buffer);
        return -1;
    }
}
int pre_wait_sem(int num_bucle, bucles *op, int tamano_tren)
{
    char buffer[100];
    int i = 0;
    for (i = 0; i < data_handler.nTrenes; i++)
    {
        if (data_handler.arrayHijos[i] == getpid())
            break;
    }
    sprintf(buffer, "X:[%i]y:[%i]TREN[%i]PREWAIT[%i]TAM[%i]TBUCLE[%i|%i]", data_handler.x, data_handler.y, i, 1 + num_bucle, tamano_tren, op[num_bucle].casillas_totales, op[num_bucle].casilla_disponibles);
    if (op[num_bucle].casilla_disponibles >= tamano_tren)
    {
        // pon_error("HACIENDO PRE WAIT");
        return 1;
    }
    else
    {
        // pon_error(buffer);
        return -1;
    }
}

int signal_sem(int num_bucle, bucles *op, int tamano_tren)
{
    // pon_error("HACIENDO SIGNAL");
    op[num_bucle].casilla_disponibles += tamano_tren;
    if (op[num_bucle].casillas_totales >= op[num_bucle].casilla_disponibles)
    {
        // fprintf(stderr, "SIGNAL TREN [%i][%i-%i] NUM BUCLE [%i]\n", getpid(), op[num_bucle].casilla_disponibles, op[num_bucle].casillas_totales, num_bucle + 1);

        return op[num_bucle].casilla_disponibles;
    }
    else
    {
        // op[num_bucle].casilla_disponibles -= tamano_tren;
        pon_error("FALLO SUS");
        return -1;
    }
}

int mirar_inter(int x, int y, bucles *struct_grafo, int TamanoTren)
{
    data_handler.x = x;
    data_handler.y = y;
    switch (y)
    {
    case 0:
        switch (x)
        {

        case 15:
            // 1º grafo
            if ((pre_wait_sem(0, struct_grafo, TamanoTren) == 1))
            {

                if (wait_sem(0, struct_grafo, TamanoTren) > 0)
                {
                    return 1;
                }
                else
                {
                    return -2;
                }
            }
            else
            {
                return -1;
            }
            break;
        case 53:
            // grafo 7 y 9 y 12

            if ((pre_wait_sem(6, struct_grafo, TamanoTren) == 1) && (pre_wait_sem(8, struct_grafo, TamanoTren) == 1) && (pre_wait_sem(11, struct_grafo, TamanoTren) == 1))
            {

                if (wait_sem(6, struct_grafo, TamanoTren) > 0 && wait_sem(8, struct_grafo, TamanoTren) > 0 && wait_sem(11, struct_grafo, TamanoTren) > 0)
                {
                    return 1;
                }
                else
                {
                    return -2;
                }
            }
            else
            {
                return -1;
            }

            break;
        case 55:
            // 1º grafo

            if ((pre_wait_sem(0, struct_grafo, TamanoTren) == 1))
            {

                if (wait_sem(0, struct_grafo, TamanoTren) > 0)
                {
                    return 1;
                }
                else
                {
                    return -2;
                }
            }
            else
            {
                return -1;
            }

            break;
        case 69:
            // 7
            if ((pre_wait_sem(6, struct_grafo, TamanoTren) == 1) && (pre_wait_sem(11, struct_grafo, TamanoTren) == 1))
            {

                if (wait_sem(6, struct_grafo, TamanoTren) > 0 && wait_sem(11, struct_grafo, TamanoTren) > 0)
                {
                    return 1;
                }
                else
                {
                    return -2;
                }
            }
            else
            {
                return -1;
            }

            break;
        default:
            return 1;
            break;
        };

        break;
    case 6:
        switch (x)
        {
        case 54:
            // 2 y 3
            if ((pre_wait_sem(1, struct_grafo, TamanoTren) == 1) && (pre_wait_sem(2, struct_grafo, TamanoTren) == 1))
            {

                if (wait_sem(1, struct_grafo, TamanoTren) > 0 && wait_sem(2, struct_grafo, TamanoTren) > 0)
                {
                    return 1;
                }
                else
                {
                    return -2;
                }
            }
            else
            {
                return -1;
            }
            break;

        default:
            return 1;
            break;
        }
        break;
    case 7:
        switch (x)
        {
        case 35:
            // 2

            if ((pre_wait_sem(1, struct_grafo, TamanoTren) == 1))
            {

                if (wait_sem(1, struct_grafo, TamanoTren) > 0)
                {
                    return 1;
                }
                else
                {
                    return -2;
                }
            }
            else
            {
                return -1;
            }
            break;

        case 53:
            // 7 y 9 y 12
            if ((pre_wait_sem(6, struct_grafo, TamanoTren) == 1) && (pre_wait_sem(8, struct_grafo, TamanoTren) == 1) && (pre_wait_sem(11, struct_grafo, TamanoTren) == 1))
            {

                if (wait_sem(6, struct_grafo, TamanoTren) > 0 && wait_sem(8, struct_grafo, TamanoTren) > 0 && wait_sem(11, struct_grafo, TamanoTren) > 0)
                {
                    return 1;
                }
                else
                {
                    return -2;
                }
            }
            else
            {

                return -1;
            }
            break;
        case 67:
            // 10 11

            if ((pre_wait_sem(9, struct_grafo, TamanoTren) == 1) && (pre_wait_sem(10, struct_grafo, TamanoTren) == 1))
            {

                if (wait_sem(9, struct_grafo, TamanoTren) > 0 && wait_sem(10, struct_grafo, TamanoTren) > 0)
                {
                    return 1;
                }
                else
                {
                    return -2;
                }
            }
            else
            {
                return -1;
            }
            break;
        default:
            return 1;
            break;
        }
        break;
    case 8:
        switch (x)
        {
        case 0:
            // 5

            if ((pre_wait_sem(4, struct_grafo, TamanoTren) == 1))
            {

                if (wait_sem(4, struct_grafo, TamanoTren) > 0)
                {
                    return 1;
                }
                else
                {
                    return -2;
                }
            }
            else
            {
                return -1;
            }
            break;

        case 68:
            // 8

            if ((pre_wait_sem(8, struct_grafo, TamanoTren) == 1) && pre_wait_sem(6, struct_grafo, TamanoTren) == 1)
            {

                if (wait_sem(8, struct_grafo, TamanoTren) > 0 && wait_sem(6, struct_grafo, TamanoTren) > 0)
                {
                    return 1;
                }
                else
                {
                    return -2;
                }
            }
            else
            {
                return -1;
            }
            break;
        default:
            return 1;
            break;
        }
        break;

    case 11:
        switch (x)
        {
        case 54:
            // 6 y 8
            if ((pre_wait_sem(5, struct_grafo, TamanoTren) == 1) && (pre_wait_sem(7, struct_grafo, TamanoTren) == 1))
            {

                if (wait_sem(5, struct_grafo, TamanoTren) > 0 && wait_sem(7, struct_grafo, TamanoTren) > 0)
                {
                    return 1;
                }
                else
                {
                    return -2;
                }
            }
            else
            {
                return -1;
            }
            break;
        default:
            return 1;
            break;
        }

        break;
    case 12:
        switch (x)
        {
        case 15:
            // 1, 3
            if ((pre_wait_sem(0, struct_grafo, TamanoTren) == 1) && (pre_wait_sem(2, struct_grafo, TamanoTren) == 1))
            {

                if (wait_sem(0, struct_grafo, TamanoTren) > 0 && wait_sem(2, struct_grafo, TamanoTren) > 0)
                {
                    return 1;
                }
                else
                {
                    return -2;
                }
            }
            else
            {
                return -1;
            }
            break;
        case 35:
            // 2, 6
            if ((pre_wait_sem(1, struct_grafo, TamanoTren) == 1) && (pre_wait_sem(5, struct_grafo, TamanoTren) == 1))
            {

                if (wait_sem(1, struct_grafo, TamanoTren) > 0 && wait_sem(5, struct_grafo, TamanoTren) > 0)
                {
                    return 1;
                }
                else
                {
                    return -2;
                }
            }
            else
            {
                return -1;
            }
            break;
        case 53:
            // 8 y 11

            if ((pre_wait_sem(7, struct_grafo, TamanoTren) == 1) && pre_wait_sem(11, struct_grafo, TamanoTren) == 1)
            {

                if (wait_sem(7, struct_grafo, TamanoTren) > 0 && wait_sem(11, struct_grafo, TamanoTren) > 0)
                {
                    return 1;
                }
                else
                {
                    return -2;
                }
            }
            else
            {
                return -1;
            }
            break;
        case 55:
            // 1 2 3 6
            if ((pre_wait_sem(0, struct_grafo, TamanoTren) == 1) && (pre_wait_sem(1, struct_grafo, TamanoTren) == 1) && (pre_wait_sem(2, struct_grafo, TamanoTren) == 1) && (pre_wait_sem(5, struct_grafo, TamanoTren) == 1))
            {

                if (wait_sem(0, struct_grafo, TamanoTren) > 0 && wait_sem(1, struct_grafo, TamanoTren) > 0 && wait_sem(2, struct_grafo, TamanoTren) > 0 && wait_sem(5, struct_grafo, TamanoTren) > 0)
                {
                    return 1;
                }
                else
                {
                    return -2;
                }
            }
            else
            {
                return -1;
            }
            break;
        case 69:
            //

            if ((pre_wait_sem(11, struct_grafo, TamanoTren) == 1) && (pre_wait_sem(7, struct_grafo, TamanoTren) == 1))
            {

                if (wait_sem(11, struct_grafo, TamanoTren) > 0 && wait_sem(7, struct_grafo, TamanoTren) > 0)
                {
                    return 1;
                }
                else
                {
                    return -2;
                }
            }
            else
            {
                return -1;
            }
            break;

        default:
            return 1;
            break;
        }
        break;
    case 13:
        switch (x)
        {
        case 0:
            // 4 5

            if ((pre_wait_sem(3, struct_grafo, TamanoTren) == 1) && pre_wait_sem(4, struct_grafo, TamanoTren) == 1)
            {

                if (wait_sem(3, struct_grafo, TamanoTren) > 0 && wait_sem(4, struct_grafo, TamanoTren) > 0)
                {
                    return 1;
                }
                else
                {
                    return -2;
                }
            }
            else
            {
                return -1;
            }
            break;
        case 16:
            // 4 y 5

            if ((pre_wait_sem(3, struct_grafo, TamanoTren) == 1) && (pre_wait_sem(4, struct_grafo, TamanoTren) == 1))
            {

                if (wait_sem(3, struct_grafo, TamanoTren) > 0 && wait_sem(4, struct_grafo, TamanoTren) > 0)
                {
                    return 1;
                }
                else
                {
                    return -2;
                }
            }
            else
            {
                return -1;
            }
            break;
        case 68:
            // 11

            if ((pre_wait_sem(10, struct_grafo, TamanoTren) == 1))
            {

                if (wait_sem(10, struct_grafo, TamanoTren) > 0)
                {
                    return 1;
                }
                else
                {
                    return -2;
                }
            }
            else
            {
                return -1;
            }
            break;
        default:
            return 1;
            break;
        }
        break;
    case 16:
        switch (x)
        {
        case 67:
            // 10

            if ((pre_wait_sem(9, struct_grafo, TamanoTren) == 1))
            {

                if (wait_sem(9, struct_grafo, TamanoTren) > 0)
                {
                    return 1;
                }
                else
                {
                    return -2;
                }
            }
            else
            {
                return -1;
            }
            break;
        case 69:
            // 8

            if ((pre_wait_sem(7, struct_grafo, TamanoTren) == 1) && (pre_wait_sem(11, struct_grafo, TamanoTren) == 1))
            {

                if (wait_sem(7, struct_grafo, TamanoTren) > 0 && wait_sem(11, struct_grafo, TamanoTren) > 0)
                {
                    return 1;
                }
                else
                {
                    return -2;
                }
            }
            else
            {
                return -1;
            }
            break;

        default:
            return 1;
            break;
        }
        break;
    default:
        return 1;
        break;
    };
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int lib_inter(int x, int y, bucles *struct_grafo, int TamanoTren)
{
    switch (y)
    {
    case 3:
        switch (x)
        {
        case 16:
            // 4

            if (signal_sem(3, struct_grafo, TamanoTren) > 0)
            {
                return 1;
            }
            else
            {
                return -1;
            }
            break;
        default:
            return 1;
            break;
        }
        break;
    case 4:
        switch (x)
        {
        case 15:
            // Grafo 1
            if (signal_sem(0, struct_grafo, TamanoTren) > 0)
            {
                return 1;
            }
            else
            {
                return -1;
            }
            break;

        default:
            return 1;
            break;
        }

        break;
    case 6:
        switch (x)
        {
        case 16:
            // 3
            if (signal_sem(2, struct_grafo, TamanoTren) > 0)
            {
                return 1;
            }
            else
            {
                return -1;
            }
            break;
        case 68:
            //  10 11

            if (signal_sem(9, struct_grafo, TamanoTren) > 0 && signal_sem(10, struct_grafo, TamanoTren) > 0)
            {
                return 1;
            }
            else
            {
                return -1;
            }
            break;
        case 74:
            // 10 11

            if (signal_sem(9, struct_grafo, TamanoTren) > 0 && signal_sem(10, struct_grafo, TamanoTren) > 0)
            {
                return 1;
            }
            else
            {
                return -1;
            }
            break;
        default:
            return 1;
            break;
        }
        break;
    case 7:
        switch (x)
        {
        case 17:
            // 4
            if (signal_sem(3, struct_grafo, TamanoTren) > 0)
            {
                return 1;
            }
            else
            {
                return -1;
            }
            break;
        case 55:
            // // 1 2 3

            if (signal_sem(0, struct_grafo, TamanoTren) > 0 && signal_sem(1, struct_grafo, TamanoTren) > 0 && signal_sem(2, struct_grafo, TamanoTren) > 0)
            {
                return 1;
            }
            else
            {
                return -1;
            }
            break;
        case 69:
            // 7
            if (signal_sem(6, struct_grafo, TamanoTren) > 0 && signal_sem(11, struct_grafo, TamanoTren) > 0)
            {
                return 1;
            }
            else
            {
                return -1;
            }
            break;
        default:
            return 1;
            break;
        }
        break;
    case 8:
        switch (x)
        {
        case 16:
            // 5
            if (signal_sem(4, struct_grafo, TamanoTren) > 0)
            {
                return 1;
            }
            else
            {
                return -1;
            }
            break;
        case 54:
            // 7 9

            if (signal_sem(6, struct_grafo, TamanoTren) > 0 && signal_sem(8, struct_grafo, TamanoTren) > 0)
            {
                return 1;
            }
            else
            {
                return -1;
            }
            break;
        case 74:
            // 9
            if (signal_sem(8, struct_grafo, TamanoTren) > 0)
            {
                return 1;
            }
            else
            {
                return -1;
            }
            break;
        default:
            return 1;
            break;
        }
        break;
    case 9:
        switch (x)
        {
        case 15:
            // 1 3

            if (signal_sem(0, struct_grafo, TamanoTren) > 0 && signal_sem(2, struct_grafo, TamanoTren) > 0)
            {
                return 1;
            }
            else
            {
                return -1;
            }
            break;
        default:
            return 1;
            break;
        }
        break;
    case 11:
        switch (x)
        {
        case 36:
            // 6
            if (signal_sem(5, struct_grafo, TamanoTren) > 0)
            {
                return 1;
            }
            else
            {
                return -1;
            }
            break;
        case 68:
            // 8
            if (signal_sem(7, struct_grafo, TamanoTren) > 0)
            {
                return 1;
            }
            else
            {
                return -1;
            }
            break;

        default:
            return 1;
            break;
        }
        break;
    case 12:
        switch (x)
        {
        case 17:
            // 4 5

            if (signal_sem(3, struct_grafo, TamanoTren) > 0 && signal_sem(4, struct_grafo, TamanoTren) > 0)
            {
                return 1;
            }
            else
            {
                return -1;
            }
            break;
        case 67:
            // 10 11

            if (signal_sem(9, struct_grafo, TamanoTren) > 0 && signal_sem(10, struct_grafo, TamanoTren) > 0)
            {
                return 1;
            }
            else
            {
                return -1;
            }
            break;

        default:
            return 1;
            break;
        }
        break;
    case 13:
        switch (x)
        {
        case 74:
            // 11
            if (signal_sem(10, struct_grafo, TamanoTren) > 0)
            {
                return 1;
            }
            else
            {
                return -1;
            }
            break;
        default:
            return 1;
            break;
        }
        break;
    case 16:
        switch (x)
        {
        case 15:
            // 1 3

            if (signal_sem(0, struct_grafo, TamanoTren) > 0 && signal_sem(2, struct_grafo, TamanoTren) > 0)
            {
                return 1;
            }
            else
            {
                return -1;
            }
            break;
        case 35:
            // 2 6

            if (signal_sem(1, struct_grafo, TamanoTren) > 0 && signal_sem(5, struct_grafo, TamanoTren) > 0)
            {
                return 1;
            }
            else
            {
                return -1;
            }
            break;
        case 53:
            // 8
            if (signal_sem(7, struct_grafo, TamanoTren) > 0 && signal_sem(11, struct_grafo, TamanoTren) > 0)
            {
                return 1;
            }
            else
            {
                return -1;
            }
            break;
        case 55:
            // 1 2 3 6

            if (signal_sem(0, struct_grafo, TamanoTren) > 0 && signal_sem(1, struct_grafo, TamanoTren) > 0 && signal_sem(2, struct_grafo, TamanoTren) > 0 && signal_sem(5, struct_grafo, TamanoTren) > 0)
            {
                return 1;
            }
            else
            {
                return -1;
            }
            break;

        default:
            return 1;
            break;
        }
        break;
    default:
        return 1;
        break;
    }
}
