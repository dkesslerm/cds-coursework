// g++ -std=c++11 -pthread -o prodcons-multi prodcons-multi.cpp scd.cpp
// DNI: 23300373Q
// Apellidos, Nombre: Kessler Martínez, David

#include <iostream>
#include <cassert>
#include <thread>
#include "scd.h"

using namespace std ;
using namespace scd ;

//**********************************************************************
// Variables globales

const unsigned
   num_items = 40 ,   // número de items
	tam_vec   = 10 ,  // tamaño del buffer
	num_hebras_productoras = 4,
	num_hebras_consumidoras = 4;
unsigned
   cont_prod[num_items] = {0}, // contadores de verificación: para cada dato, número de veces que se ha producido.
   cont_cons[num_items] = {0}; // contadores de verificación: para cada dato, número de veces que se ha consumido.

Semaphore s_escritura = tam_vec;
Semaphore s_lectura = 0;

bool lifo = 1; // si se quiere lifo => fifo = 0
int vec[tam_vec];
int primera_libre = 0;
int primera_ocupada = 0;

int items_producidos[num_hebras_productoras] = {0}; // no lo uso significativamente

//**********************************************************************
// funciones comunes a las dos soluciones (fifo y lifo)
//----------------------------------------------------------------------

unsigned producir_dato(int id_hebra_productora, int i) {
    this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
    const unsigned dato_producido = id_hebra_productora * num_items / num_hebras_productoras + i;
    cont_prod[dato_producido]++ ;
    items_producidos[id_hebra_productora]++; // por dar más información, no necesario
    cout << "producido: " << dato_producido << " por productor "
            << id_hebra_productora << endl
            << "total producido: " << items_producidos[id_hebra_productora]
            << endl << flush ;
    return dato_producido ;
}
//----------------------------------------------------------------------

void consumir_dato(int id_hebra_consumidora, unsigned dato) {
    assert( dato < num_items );
    cont_cons[dato] ++ ;
    this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));

    cout << "                  consumido: " << dato
            << " por consumidor " << id_hebra_consumidora << endl;
}


//----------------------------------------------------------------------

void test_contadores() {
    bool ok = true ;
    cout << "comprobando contadores ...." ;
    for( unsigned i = 0 ; i < num_items ; i++ )
    {  if ( cont_prod[i] != 1 )
        {  cout << "error: valor " << i << " producido " << cont_prod[i] << " veces." << endl ;
            ok = false ;
        }
        if ( cont_cons[i] != 1 )
        {  cout << "error: valor " << i << " consumido " << cont_cons[i] << " veces" << endl ;
            ok = false ;
        }
    }
    if (ok)
        cout << endl << flush << "solución (aparentemente) correcta." << endl << flush ;
}

//----------------------------------------------------------------------

void funcion_hebra_productora(int id_hebra_productora) {
    int num_items_por_hebra_productora = num_items / num_hebras_productoras;
    for (unsigned i = 0; i < num_items_por_hebra_productora; i++) {
        int dato = producir_dato(id_hebra_productora, i);
        sem_wait(s_escritura);
        if (lifo) {
            vec[primera_libre] = dato;
            primera_libre++;
        } else {
            vec[primera_libre] = dato;
            primera_libre++;
            primera_libre %= tam_vec;
        }
        sem_signal(s_lectura);
    }
}

//----------------------------------------------------------------------

void funcion_hebra_consumidora(int id_hebra_consumidora) {
    int num_items_por_hebra_consumidora = num_items / num_hebras_consumidoras;
    for (unsigned i = 0; i < num_items_por_hebra_consumidora; i++) {
        int dato ;
        sem_wait(s_lectura);
        if (lifo) {
            primera_libre--;
            dato = vec[primera_libre];
        } else {
            dato = vec[primera_ocupada];
            primera_ocupada++;
            primera_ocupada %= tam_vec;
        }
        sem_signal(s_escritura);
        consumir_dato( id_hebra_consumidora, dato ) ;
    }
}
//----------------------------------------------------------------------

int main() {
    if (lifo) {
        cout << "-----------------------------------------------------------------" << endl
             << "Problema de los productores-consumidores (solución LIFO)." << endl
             << "------------------------------------------------------------------" << endl
             << flush ;
    } else {
        cout << "-----------------------------------------------------------------" << endl
             << "Problema de los productores-consumidores (solución FIFO)." << endl
             << "------------------------------------------------------------------" << endl
             << flush ;
    }

    thread hebra_productora[num_hebras_productoras],
          hebra_consumidora[num_hebras_consumidoras];

    for(int i = 0; i < num_hebras_productoras; i++) {
        hebra_productora[i] = thread(funcion_hebra_productora, i);
    }
    for(int i = 0; i < num_hebras_consumidoras; i++) {
        hebra_consumidora[i] = thread(funcion_hebra_consumidora, i);
    }

    for(int i = 0; i < num_hebras_productoras; i++) {
        hebra_productora[i].join();
    }
    for (int i = 0; i < num_hebras_consumidoras; i++) {
        hebra_consumidora[i].join();
    }

   test_contadores();
}
