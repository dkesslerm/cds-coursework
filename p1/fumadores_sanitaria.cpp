#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include "scd.h"

using namespace std ;
using namespace scd ;

// numero de fumadores
const int num_fumadores = 3 ;

Semaphore mostr_vacio = 1;
Semaphore ingr_disp[3] = {0, 0, 0};
Semaphore sanitario = 0;
Semaphore puedes_fumar = num_fumadores;
int cigarrillos_por_fumador[num_fumadores] = {0};
int fumador_vicioso;

const int ejecuciones = 10;
const int max_cigarros = 2;
int contador = 0;
bool fin = false;
//-------------------------------------------------------------------------
// Función que simula la acción de producir un ingrediente, como un retardo
// aleatorio de la hebra (devuelve número de ingrediente producido)

int producir_ingrediente()
{
   // calcular milisegundos aleatorios de duración de la acción de fumar)
   chrono::milliseconds duracion_produ( aleatorio<10,100>() );

   // informa de que comienza a producir
   cout << "Estanquero : empieza a producir ingrediente (" << duracion_produ.count() << " milisegundos)" << endl;

   // espera bloqueada un tiempo igual a ''duracion_produ' milisegundos
   this_thread::sleep_for( duracion_produ );

   const int num_ingrediente = aleatorio<0,num_fumadores-1>() ;

   // informa de que ha terminado de producir
   cout << "Estanquero : termina de producir ingrediente " << num_ingrediente << endl;

   return num_ingrediente ;
}

//----------------------------------------------------------------------
// función que ejecuta la hebra del estanquero

void funcion_hebra_estanquero(  )
{
    while (contador < ejecuciones) {
        contador++;
        cout << endl << "Iteración número " << contador << endl << endl;
        int i = producir_ingrediente();
        sem_wait(mostr_vacio);
        cout << "Puesto en la mesa el ingrediente: " << i << endl;
        sem_signal(ingr_disp[i]);
    }

    sem_wait(mostr_vacio);
    fin = true;
    for (int i = 0; i < num_fumadores; i++) {
        sem_signal(ingr_disp[i]);
    }
    sem_signal(sanitario);
    sem_signal(puedes_fumar);
}

//-------------------------------------------------------------------------
// Función que simula la acción de fumar, como un retardo aleatoria de la hebra

void fumar( int num_fumador )
{

   // calcular milisegundos aleatorios de duración de la acción de fumar)
   chrono::milliseconds duracion_fumar( aleatorio<20,200>() );

   // informa de que comienza a fumar

    cout << "Fumador " << num_fumador << "  :"
          << " empieza a fumar (" << duracion_fumar.count() << " milisegundos)" << endl;

   // espera bloqueada un tiempo igual a ''duracion_fumar' milisegundos
   this_thread::sleep_for( duracion_fumar );

   // informa de que ha terminado de fumar

    cout << "Fumador " << num_fumador << "  : termina de fumar, comienza espera de ingrediente." << endl;

}

//----------------------------------------------------------------------
// función que ejecuta la hebra del fumador
void  funcion_hebra_fumador( int num_fumador)
{
   while( true )
   {
       if (cigarrillos_por_fumador[num_fumador] == max_cigarros) {
           cout << "Fumador " << num_fumador << " : llamando a la hebra sanitaria." << endl << flush;
           fumador_vicioso = num_fumador;
           sem_signal(sanitario);
           sem_wait(puedes_fumar);
           cout << "Soy el fuumador " << num_fumador << " y me han llamado vicioso." << endl << flush;
           cigarrillos_por_fumador[num_fumador] = 0;
       }
       sem_wait(ingr_disp[num_fumador]);
       if (fin) break;
       cout << "Retirado ingrediente: " << num_fumador << endl;
       sem_signal(mostr_vacio);
       fumar(num_fumador);
       cigarrillos_por_fumador[num_fumador]++;
   }
}

//----------------------------------------------------------------------
// función que ejecuta la hebra sanitaria
void funcion_hebra_sanitaria() {
    while (true) {
        sem_wait(sanitario);
        if (fin) break;
        cout << "FUMAR MATA: ya lo sabes, fumador " << fumador_vicioso << "." << endl << flush;
        sem_signal(puedes_fumar);
    }
}


//----------------------------------------------------------------------

int main()
{
   // declarar hebras y ponerlas en marcha
    thread estanquero(funcion_hebra_estanquero);
    thread fumadores[num_fumadores];
    thread sanitaria(funcion_hebra_sanitaria);

    for (int i = 0; i < num_fumadores; i++) {
        fumadores[i] = thread(funcion_hebra_fumador, i);
    }

    estanquero.join();
    for (int i = 0; i < num_fumadores; i++) {
        fumadores[i].join();
    }
    sanitaria.join();

    return 0;
}
