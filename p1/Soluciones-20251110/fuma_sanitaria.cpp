// g++ -std=c++11 -pthread -o fuma_sanitaria Soluciones-20251110/fuma_sanitaria.cpp scd.cpp

#include <algorithm>
#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random>
#include <algorithm>
#include <chrono>
#include <vector>
#include "../scd.h"

using namespace std;
using namespace scd;

constexpr int num_fumadores = 3;         // número de fumadores
Semaphore mostrador_libre = 1;           // 1 si el mostrador está libre, 0 si no
mutex mutex_cout;                        // mutex para que no se mezclen las salidas en pantalla
vector<Semaphore> ingrediente_en_mostrador; // vector de semáforos de fumadores
// vale: 1 si el i-ésimo fumador tiene su ingrediente disponible en
// el mostrador, 0 si no.

// VARIABLES PARA LA HEBRA SANITARIA
Semaphore hebra_sanitaria(0);               // hebra sanitaria empieza bloqueada
vector<int> contador_cigarrillos(num_fumadores, 0); // cuenta cigarrillos fumados por cada fumador

//--------------------------------------------
// Función que simula la acción de producir un ingrediente, como un retardo
// aleatorio de la hebra (devuelve número de ingrediente producido)

int producir_ingrediente()
{
     // calcular milisegundos aleatorios de duración de la acción de fumar)
     chrono::milliseconds duracion_produ( aleatorio<10,100>() );

    // informa de que comienza a producir
    cout << "Estanquero: empieza a producir ingrediente (" << duracion_produ.count() << " ms)" << endl;

    // espera bloqueada un tiempo igual a ''duracion_produ' milisegundos
    this_thread::sleep_for(duracion_produ);

   int num_ingrediente = aleatorio<0,num_fumadores-1>();
    // informa de que ha terminado de producir
    cout << "Estanquero: termina de producir ingrediente " << num_ingrediente << endl;

    return num_ingrediente;
}

//--------------------------------------------
// función que ejecuta la hebra del estanquero

void funcion_hebra_estanquero()
{
    while (true)
    {
        // selecciona aleatoriamente un fumador
        int num_fumador = producir_ingrediente();
        if (*max_element(contador_cigarrillos.begin(), contador_cigarrillos.end()) == 6) break;
        // esperar a que el mostrador esté libre
        mostrador_libre.sem_wait();

        // informa del ingrediente que ha puesto
        mutex_cout.lock();
        cout << "Estanquero: pone en mostrador ingrediente para fumador " << num_fumador << "." << endl;
        mutex_cout.unlock();

        // señala que está disponible el ingrediente para el fumador número 'num_fumador'
        ingrediente_en_mostrador[num_fumador].sem_signal();
    }
}

//--------------------------------------------
// Función que simula la acción de fumar, como un retardo aleatoria de la hebra

void fumar(int num_fumador)
{
   // calcular milisegundos aleatorios de duración de la acción de fumar)
   chrono::milliseconds duracion_fumar( aleatorio<20,200>() );

   // informa de que comienza a fumar
    mutex_cout.lock();
    cout << "Fumador " << num_fumador << " : empieza a fumar (" << duracion_fumar.count() << " ms)" << endl;
    mutex_cout.unlock();
    // espera bloqueada un tiempo igual a ''duracion_fumar' milisegundos
    this_thread::sleep_for(duracion_fumar);

    // informa de que ha terminado de fumar
    mutex_cout.lock();
    cout << "Fumador " << num_fumador << " : termina de fumar, comienza espera de ingrediente." << endl;
    mutex_cout.unlock();
}

//--------------------------------------------
// función que ejecuta la hebra del fumador
void funcion_hebra_fumador(int num_fumador)
{
    int cigarrillos_fumados = 0;
    while (cigarrillos_fumados < 6) // limitar a 6 cigarrillos
    {
         ingrediente_en_mostrador[num_fumador].sem_wait();
        mutex_cout.lock();
        cout << "Fumador " << num_fumador << " : obtiene su ingrediente del mostrador" << endl;
        mutex_cout.unlock();
       mostrador_libre.sem_signal();
       fumar(num_fumador);
        // incrementar contador de cigarrillos
        contador_cigarrillos[num_fumador]++;

        // activar hebra sanitaria si alcanzó el umbral de 2 cigarrillos
        if(contador_cigarrillos[num_fumador] >= 2)
        {
            hebra_sanitaria.sem_signal(); // desbloquea hebra sanitaria

            // informar que el fumador ha recibido la advertencia
            mutex_cout.lock();
            cout << "Soy el fumador " << num_fumador << " y me han llamado vicioso" << endl;
            mutex_cout.unlock();
        }

        cigarrillos_fumados++;
    }
}

//--------------------------------------------
// Hebra sanitaria: muestra aviso cada 2 cigarrillos

void funcion_hebra_sanitaria()
{
    while (true)
    {
        // Espera a ser desbloqueada por un fumador
        hebra_sanitaria.sem_wait();

        int fumador_listo = -1; // variable temporal para identificar al fumador que alcanzó el umbral

        // Buscar un fumador que haya fumado 2 cigarrillos
        for(int i = 0; i < num_fumadores; i++)
        {
            if(contador_cigarrillos[i] >= 2)
            {
                fumador_listo = i;           // identificamos al fumador listo
                contador_cigarrillos[i] = 0; // reiniciamos su contador
                break;                       // solo consideramos el primero que cumpla la condición
            }
        }

        if (*max_element(contador_cigarrillos.begin(), contador_cigarrillos.end()) == 6) break;

        // Si ningún fumador alcanzó el umbral, volver a esperar
        if(fumador_listo == -1)
            continue;

        // Aviso sanitario
        mutex_cout.lock();
        cout << "FUMAR MATA: ya lo sabes, fumador " << fumador_listo << endl;
        mutex_cout.unlock();

        // Desbloquear al fumador para que continúe su ciclo
        ingrediente_en_mostrador[fumador_listo].sem_signal();
    }
}

//--------------------------------------------
int main()
{
    for(int i=0; i<num_fumadores; i++)
        ingrediente_en_mostrador.push_back(Semaphore(0));

    thread hebra_estanquero(funcion_hebra_estanquero),
                  hebra_fumador[num_fumadores],
                  sanitaria(funcion_hebra_sanitaria); // hebra sanitaria

    for(unsigned long i =0; i<num_fumadores; i++)
        hebra_fumador[i] = thread(funcion_hebra_fumador, i);

    // esperar a que las hebras terminen
    hebra_estanquero.join();
    for(int i=0; i<num_fumadores; i++)
        hebra_fumador[i].join();
    sanitaria.join();
}
//----------------------------------------------------------------------
