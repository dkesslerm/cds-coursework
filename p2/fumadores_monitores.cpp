// g++ -std=c++11 -pthread -o ejecutable_fum_monitores fumadores_monitores.cpp scd.cpp

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

// para hacer la ejecución finita
// const int ejecuciones = 10;
// int contador = 0;
// bool terminar = false;

//-------------------------------------------------------------------------

class Estanco : public HoareMonitor {
private:
    int ingr_actual;
    CondVar ingr_no_disponible[num_fumadores];
    CondVar mostrador_lleno;

public:
    Estanco();
    void obtenerIngrediente(int i);
    void ponerIngrediente(int i);
    void esperarRecogidaIngrediente();

    // void finalizarEjecucion(); // para ejecución finita
};

//-------------------------------------------------------------------------

Estanco::Estanco() {
    ingr_actual = -1;
    for (int i = 0; i < num_fumadores; i++) {
        ingr_no_disponible[i] = newCondVar();
    }
    mostrador_lleno = newCondVar();
}

void Estanco::obtenerIngrediente(int i) {
    // if (terminar) return; // ejecución finita

    if (ingr_actual != i) {
        ingr_no_disponible[i].wait();
    }

    ingr_actual = -1;
    cout << "Fumador " << i << "  : Ingrediente retirado." << endl << flush;
    mostrador_lleno.signal();
}

void Estanco::ponerIngrediente(int i) {
    ingr_actual = i;
    ingr_no_disponible[i].signal();
}

void Estanco::esperarRecogidaIngrediente() {
    if (ingr_actual != -1) {
        mostrador_lleno.wait();
    }
}

// Para ejecución finita
// void Estanco::finalizarEjecucion() {
//     terminar = true;
//     for (int i = 0; i < num_fumadores; i++) {
//         ingr_no_disponible[i].signal();
//     }
// }

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
// función que ejecuta la hebra del estanquero

void funcion_hebra_estanquero(MRef<Estanco> monitor)
{
    // si queremos hacerlo finito, while (contador < ejecuciones)
    while (true) {
        // contador++;
        // cout << endl << "Iteración número " << contador << endl << endl;
        int i = producir_ingrediente();
        cout << " Estanquero : Ingrediente " << i << " puesto." << endl << flush;
        monitor->ponerIngrediente(i);
        monitor->esperarRecogidaIngrediente();
    }

    // monitor->finalizarEjecucion();
}

//----------------------------------------------------------------------
// función que ejecuta la hebra del fumador
void  funcion_hebra_fumador( int num_fumador, MRef<Estanco> monitor)
{
    // para ejecución finita, while (!terminar)
    while (true) {
       monitor->obtenerIngrediente(num_fumador);
       // if (terminar) break; // para ejecución finita
       fumar(num_fumador);
    }
}

//----------------------------------------------------------------------

int main()
{
    MRef<Estanco> monitor = Create<Estanco>();

    thread hebra_estanquero(funcion_hebra_estanquero, monitor);
    thread hebra_fumador[num_fumadores];

    for (int i = 0; i < num_fumadores; i++) {
        hebra_fumador[i] = thread(funcion_hebra_fumador, i, monitor);
    }

    hebra_estanquero.join();
    for (int i = 0; i < num_fumadores; i++) {
        hebra_fumador[i].join();
    }

    return 0;
}
