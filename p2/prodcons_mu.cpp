// g++ -std=c++11 -pthread -o prodcons_mu prodcons_mu.cpp scd.cpp

#include <iostream>
#include <cassert>
#include <thread>
#include "scd.h"

using namespace std ;
using namespace scd ;
using namespace std::chrono;

constexpr int
   num_items = 15 ,   // número de items a producir/consumir
   num_hebras_productoras = 3,
   num_hebras_consumidoras = 5;

int
   siguiente_dato = 0 ; // siguiente valor a devolver en 'producir_dato'

constexpr int
   min_ms    = 5,     // tiempo minimo de espera en sleep_for
   max_ms    = 20 ;   // tiempo máximo de espera en sleep_for

int items_producidos[num_hebras_productoras] = {0}; // no lo uso significativamente
bool lifo = 1; // si se quiere fifo => lifo = 0;

mutex
   mtx ;                 // mutex de escritura en pantalla
unsigned
   cont_prod[num_items] = {0}, // contadores de verificación: producidos
   cont_cons[num_items] = {0}; // contadores de verificación: consumidos

//**********************************************************************
// funciones comunes a las dos soluciones (fifo y lifo)
//----------------------------------------------------------------------

int producir_dato(int id_hebra_productora, int i)
{

   this_thread::sleep_for( chrono::milliseconds( aleatorio<min_ms,max_ms>() ));
   const unsigned dato_producido = id_hebra_productora * num_items / num_hebras_productoras + i;
   cont_prod[dato_producido]++ ;
   items_producidos[id_hebra_productora]++; // por dar más información, no necesario
   mtx.lock();
   cout << "hebra productora " << id_hebra_productora << " produce " << dato_producido
        << " , total: " << items_producidos[id_hebra_productora] << endl << flush ;
   mtx.unlock();
   return dato_producido ;
}
//----------------------------------------------------------------------

void consumir_dato(int id_hebra_consumidora, unsigned valor_consumir )
{
   if ( num_items <= valor_consumir )
   {
      cout << " valor a consumir === " << valor_consumir << ", num_items == " << num_items << endl ;
      assert( valor_consumir < num_items );
   }
   cont_cons[valor_consumir] ++ ;
   this_thread::sleep_for( chrono::milliseconds( aleatorio<min_ms,max_ms>() ));
   mtx.lock();
   cout << "                  hebra consumidora " << id_hebra_consumidora << ", consume: " << valor_consumir << endl ;
   mtx.unlock();
}
//----------------------------------------------------------------------

void test_contadores()
{
   bool ok = true ;
   cout << "comprobando contadores ...." << endl ;

   for( unsigned i = 0 ; i < num_items ; i++ )
   {
      if ( cont_prod[i] != 1 )
      {
         cout << "error: valor " << i << " producido " << cont_prod[i] << " veces." << endl ;
         ok = false ;
      }
      if ( cont_cons[i] != 1 )
      {
         cout << "error: valor " << i << " consumido " << cont_cons[i] << " veces" << endl ;
         ok = false ;
      }
   }
   if (ok)
      cout << endl << flush << "solución (aparentemente) correcta." << endl << flush ;
}

// *****************************************************************************
// clase para monitor buffer, version LIFO, semántica SC, multiples prod/cons

class ProdConsMu : public HoareMonitor
{
 private:
 static const int           // constantes ('static' ya que no dependen de la instancia)
   num_celdas_total = 10;   //   núm. de entradas del buffer
 int                        // variables permanentes
   buffer[num_celdas_total],//   buffer de tamaño fijo, con los datos
   primera_libre,
   primera_ocupada,
   num_ocupadas;

 CondVar                    // colas condicion:
   ocupadas,                //  cola donde espera el consumidor (n>0)
   libres ;                 //  cola donde espera el productor  (n<num_celdas_total)

 public:                    // constructor y métodos públicos
   ProdConsMu() ;             // constructor
   int  leer();                // extraer un valor (sentencia L) (consumidor)
   void escribir( int valor ); // insertar un valor (sentencia E) (productor)
} ;

// -----------------------------------------------------------------------------

ProdConsMu::ProdConsMu() {
    primera_libre = 0;
    primera_ocupada = 0;
    num_ocupadas = 0;
    ocupadas = newCondVar();
    libres = newCondVar();
}

// -----------------------------------------------------------------------------
// función llamada por el consumidor para extraer un dato

int ProdConsMu::leer(  )
{
    int valor;
    if (lifo) {
        if (primera_libre == 0) {
            ocupadas.wait();
        }
        // cout << "leer: ocup == " << primera_libre << ", total == " << num_celdas_total << endl ;
        assert( 0 < primera_libre  );
        // hacer la operación de lectura, actualizando estado del monitor
        primera_libre--;
        valor = buffer[primera_libre] ;
    } else { // fifo
        if (num_ocupadas == 0) {
            ocupadas.wait();
        }

        assert(0 < num_ocupadas);
        valor = buffer[primera_ocupada];
        primera_ocupada++;
        primera_ocupada %= num_celdas_total;
        num_ocupadas--;
    }

    // señalar al productor que hay un hueco libre, por si está esperando
    libres.signal();

    // devolver valor
    return valor ;
}

// -----------------------------------------------------------------------------

void ProdConsMu::escribir( int valor )
{
    if (lifo) {
        if (primera_libre == num_celdas_total) {
            libres.wait();
        }
        // cout << "escribir: ocup == " << primera_libre << ", total == " << num_celdas_total << endl ;
        assert( primera_libre < num_celdas_total );
        // hacer la operación de inserción, actualizando estado del monitor
        buffer[primera_libre] = valor ;
        primera_libre++ ;
    } else {
        if (num_ocupadas == num_celdas_total) {
            libres.wait();
        }
        assert(num_ocupadas < num_celdas_total);
        buffer[primera_libre] = valor ;
        primera_libre++ ;
        primera_libre %= num_celdas_total;
        num_ocupadas++;
    }

    // señalar al consumidor que ya hay una celda ocupada (por si esta esperando)
    ocupadas.signal();
}

// *****************************************************************************
// funciones de hebras

void funcion_hebra_productora(MRef<ProdConsMu> monitor, int id_hebra_productora) {
    int num_items_por_hebra_productora = num_items / num_hebras_productoras;
    for (unsigned i = 0; i < num_items_por_hebra_productora; i++) {
        int valor = producir_dato(id_hebra_productora, i) ;
        monitor->escribir( valor );
    }
}

// -----------------------------------------------------------------------------

void funcion_hebra_consumidora(MRef<ProdConsMu> monitor, int id_hebra_consumidora) {
    int num_items_por_hebra_consumidora = num_items / num_hebras_consumidoras;
    for (unsigned i = 0; i < num_items_por_hebra_consumidora; i++) {
        int valor = monitor->leer();
        consumir_dato(id_hebra_consumidora, valor) ;
    }
}

// -----------------------------------------------------------------------------

int main()
{
    if (lifo) {
        cout << "--------------------------------------------------------------------" << endl
             << "Problema del productor-consumidor múltiples (Monitor SU, buffer LIFO). " << endl
             << "--------------------------------------------------------------------" << endl
             << flush ;
    } else {
        cout << "--------------------------------------------------------------------" << endl
             << "Problema del productor-consumidor múltiples (Monitor SU, buffer FIFO). " << endl
             << "--------------------------------------------------------------------" << endl
             << flush ;
    }
    // crear monitor ('monitor' es una referencia al mismo, de tipo MRef<...>)
    MRef<ProdConsMu> monitor = Create<ProdConsMu>() ;
    thread hebra_productora[num_hebras_productoras],
           hebra_consumidora[num_hebras_consumidoras];

    for (int i = 0; i < num_hebras_productoras; i++) {
        hebra_productora[i] = thread(funcion_hebra_productora, monitor, i);
    }
    for (int i = 0; i < num_hebras_consumidoras; i++) {
        hebra_consumidora[i] = thread(funcion_hebra_consumidora, monitor, i);
    }

    for(int i = 0; i < num_hebras_productoras; i++)
        hebra_productora[i].join();
    for (int i = 0; i < num_hebras_consumidoras; i++)
        hebra_consumidora[i].join();

    test_contadores() ;
}
