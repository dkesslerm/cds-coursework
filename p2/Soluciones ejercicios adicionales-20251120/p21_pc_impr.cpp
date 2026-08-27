// g++ -std=c++11 -pthread -o impresora Soluciones\ ejercicios\ adicionales-20251120/p21_pc_impr.cpp scd.cpp

// -----------------------------------------------------------------------------
//
// Sistemas Concurrentes y Distribuidos.
// Seminario 2. Introducción a los monitores en C++11.
//
// Archivo: p2_pc_impr.cpp
// Ejemplo de un monitor en C++11 con semántica SU, para el problema
// del productor/consumidor, con múltiples productores y consumidores.
// Extendido para un hebra impresora adicional.
// Esa hebra espera bloqueada a que se produzca un multiplo de 5 y entonces imprime un mensaje.
// Opcion FIFO.
//
// Enunciado:
//
// Ejercicio 1. (prod/cons con impresora)
//
// Copia tu solución FIFO al problema de los productores-consumidores con
// monitores SU (basado en la clase HoareMonitor) en un archivo nuevo llamado
// p2_pc_impr.cpp, y extiéndelo para cumplir estos requisitos adicionales a los del problema original:
//
// (1) Asegúrate que el número total de items a producir sea múltiplo de 5 (además
// de múltiplo del número de productores y múltiplo del número de consumidores).
//
// (2) Se debe crear una nueva hebra llamada impresora, que ejecuta  un bucle.
// En cada iteración la hebra impresora debe  llamar  a un nuevo método del monitor
// llamado metodo_impresora. Ese método devuelve un valor lógico. El bucle de la
// hebra impresora acaba cuando ese método devuelve false.
//
// (3) Si al inicio del método metodo_impresora la hebra impresora detecta que
// ya se han insertado en el buffer todos los múltiplos de 5 que se tenían que
// insertar, devuelve false, en otro caso (todavía quedan múltiplos de 5 por
// insertar), imprime un mensaje (se describe a continuación) y después
// devuelve true (ten en cuenta que en total se deben insertar T/5 múltiplos
// de 5, donde T es el numero total de items a producir por todos los productores).
//
// (4) En el  mensaje que la hebra impresora imprime (en metodo_impresora)
// se debe incluir el número total de nuevos múltiplos de 5 que se han
// insertado en el vector, contados desde la anterior llamada a metodo_impresora
// o desde el inicio de la simulación (si es la primera llamada). Si dicho
// número es cero, la hebra debe esperar bloqueada hasta que dicho número
// sea mayor que 0, antes de imprimir el mensaje (por tanto, nunca imprime un cero).
//
// (5) Ten en cuenta que las hebras productoras son responsables de
// contabilizar la cantidad de múltiplos de 5 insertados y de
// desbloquear a la hebra impresora cuando sea necesario hacerlo,
// todo ello debe gestionarse dentro del método escribir del monitor.
//
// Historial:
// Creado en Agosto de 2017
// -----------------------------------------------------------------------------


#include <iostream>
#include <iomanip>
#include <cassert>
#include <random>
#include <thread>
#include "../scd.h"

using namespace std ;
using namespace scd ;

constexpr unsigned
   num_items       = 6*4*5,   // número de items a producir/consumir
   num_hebras_prod = 6 ,   // número de hebras productoras (divisor de num_items)
   num_hebras_cons = 4 ,   // número de hebras consumidoras (divisor de num_items)
   num_items_prod  = num_items/num_hebras_prod, // núm. items por productor
   num_items_cons  = num_items/num_hebras_cons, // núm. items por consumidor
   total_multiplos = num_items/5,               // número de iteraciones de la hebra impresora
   min_ms          = 5,
   max_ms          = 20 ;

mutex
   mtx ;                 // mutex de escritura en pantalla
unsigned
   cont_prod[num_items] = {0}, // contadores de verificación: producidos
   cont_cons[num_items] = {0}; // contadores de verificación: consumidos

//**********************************************************************
// funciones comunes a las dos soluciones (fifo y lifo)
//----------------------------------------------------------------------

// producir un dato recibe como parámetro el núm de hebra productora

int producir_dato( int num_prod )
{
   static int contador[num_hebras_prod] = {0} ;
   this_thread::sleep_for( chrono::milliseconds( aleatorio<min_ms,max_ms>() ));
   const int valor_producido = num_prod*num_items_prod + contador[num_prod] ;
   contador[num_prod]++ ;
   // mtx.lock();
   // cout << "hebra productora " << num_prod << ", produce " << valor_producido << endl << flush ;
   // mtx.unlock();
   cont_prod[valor_producido]++ ;
   return valor_producido ;
}
//----------------------------------------------------------------------

void consumir_dato( unsigned valor_consumir, int num_cons )
{
   if ( num_items <= valor_consumir )
   {
      cout << " valor a consumir === " << valor_consumir << ", num_items == " << num_items << endl ;
      assert( valor_consumir < num_items );
   }
   cont_cons[valor_consumir] ++ ;
   this_thread::sleep_for( chrono::milliseconds( aleatorio<min_ms,max_ms>() ));
   // mtx.lock();
   // cout << "                  hebra consumidora " << num_cons << ", consume: " << valor_consumir << endl ;
   // mtx.unlock();
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
// clase para monitor buffer, version FIFO, semántica SC, multiples prod/cons

class ProdConsMuSU : public HoareMonitor
{
 private:
 static const int           // constantes:
   num_celdas_total = 30;   //  núm. de entradas del buffer
 int                        // variables permanentes
   buffer[num_celdas_total],//  buffer de tamaño fijo, con los datos
   primera_libre,           //  indice de celda de la próxima inserción
   primera_ocupada,         //  índice de celda de la próxima lectura
   num_celdas_ocupadas;     //  numero de valores pendientes de leer

   int num_multiplos_5 = 0, num_multiplos_5_parcial = 0 ;
   int num_total_insertados = 0, num_total_extraidos = 0 ;
 std::mutex
   cerrojo_monitor ;        // cerrojo del monitor
 CondVar                    // colas condicion:
   ocupadas,                //  cola donde espera el consumidor (n>0)
   libres ,                 //  cola donde espera el productor  (n<num_celdas_total)
   impresora ;              // cola donde espera la hebra impresora

 public:                    // constructor y métodos públicos
   ProdConsMuSU() ;             // constructor
   int  leer( int n_cons );                // extraer un valor (sentencia L) (consumidor)
   void escribir( int n_prod, int valor ); // insertar un valor (sentencia E) (productor)
   bool metodo_impresora( );    // método que llama la hebra impresora en cada iteración de su bucle
} ;
// -----------------------------------------------------------------------------

ProdConsMuSU::ProdConsMuSU(  )
{
   primera_libre       = 0 ;
   primera_ocupada     = 0 ;
   num_celdas_ocupadas = 0 ;
   num_multiplos_5     = 0 ;
   num_multiplos_5_parcial = 0 ;

   ocupadas            = newCondVar();
   libres              = newCondVar();
   impresora           = newCondVar();
}
// -----------------------------------------------------------------------------
// función llamada por el consumidor para extraer un dato

int ProdConsMuSU::leer( int n_cons )
{
   // esperar bloqueado hasta que 0 < num_celdas_ocupadas
   if ( num_celdas_ocupadas == 0 )
      ocupadas.wait();

   //cout << "leer: ocup == " << num_celdas_ocupadas << ", total == " << num_celdas_total << endl ;
   assert( 0 < num_celdas_ocupadas  );

   // hacer la operación de lectura, actualizando estado del monitor
   const int valor = buffer[primera_ocupada] ;
   primera_ocupada = (primera_ocupada+1) % num_celdas_total ;
   num_celdas_ocupadas-- ;

   num_total_extraidos ++ ;

   // mensaje
   cout << "                              consumidor " << n_cons << " extraído " << valor << " (extraídos " << num_total_extraidos << "/" << num_items << ")" << endl ;

   // señalar al productor que hay un hueco libre, por si está esperando
   libres.signal();



   // devolver valor
   return valor ;
}
// -----------------------------------------------------------------------------

void ProdConsMuSU::escribir( int n_prod, int valor )
{

   // esperar bloqueado hasta que num_celdas_ocupadas < num_celdas_total
   if ( num_celdas_ocupadas == num_celdas_total )
      libres.wait();

   //cout << "escribir: ocup == " << num_celdas_ocupadas << ", total == " << num_celdas_total << endl ;
   assert( num_celdas_ocupadas < num_celdas_total );

   // mensaje


   // hacer la operación de inserción, actualizando estado del monitor
   buffer[primera_libre] = valor ;
   primera_libre = (primera_libre+1) % num_celdas_total ;
   num_celdas_ocupadas++ ;

   num_total_insertados ++ ;

   cout << "productor " << n_prod << " ha insertado " << valor << " (insertados " << num_total_insertados << "/" << num_items << ")" ;

   // si es multiplo de 5: contabilizarlo y despertar a la hebra impresora.
   if ( valor % 5 == 0 )
   {
      cout << " SÍ múltiplo" << endl ;
      num_multiplos_5 ++ ;
      num_multiplos_5_parcial ++ ;
      impresora.signal();
   }
   else
      cout << " no múltiplo" << endl ;

   // señalar al consumidor que ya hay una celda ocupada (por si esta esperando)
   ocupadas.signal();
}
// -----------------------------------------------------------------------------

bool ProdConsMuSU::metodo_impresora(  )
{
   if ( num_multiplos_5 == total_multiplos )
   {
      cout << "--------- impresora: termina -----" << endl ;
      return false ;
   }

   if ( num_multiplos_5_parcial == 0 )
      impresora.wait();

   cout << "--------- impresora: múltiplos: parcial =" << num_multiplos_5_parcial << "  total actual =" << num_multiplos_5 << " total final =" << total_multiplos << endl ;

   num_multiplos_5_parcial = 0 ;

   return true ;
}

// *****************************************************************************
// funciones de hebras

void funcion_hebras_productoras( MRef<ProdConsMuSU>  monitor, int num_prod )
{
   register_thread_name("productor",num_prod);

   for( unsigned i = 0 ; i < num_items_prod ; i++ )
   {
      int valor = producir_dato( num_prod ) ;
      monitor->escribir( num_prod, valor );
   }
}
// -----------------------------------------------------------------------------

void funcion_hebras_consumidoras( MRef<ProdConsMuSU>  monitor, int num_cons )
{
   register_thread_name("consumidor",num_cons);

   for( unsigned i = 0 ; i < num_items_cons ; i++ )
   {
      int valor = monitor->leer( num_cons );
      consumir_dato( valor, num_cons ) ;
   }
}

// -----------------------------------------------------------------------------

void funcion_hebra_impresora( MRef<ProdConsMuSU>  monitor )
{
   register_thread_name("impresora");

   while( monitor->metodo_impresora() )
   {
      this_thread::sleep_for( chrono::milliseconds( aleatorio<min_ms,max_ms>() ) );
   }
}

// -----------------------------------------------------------------------------

int main()
{
   cout << "---------------------------------------------------------------------------------------" << endl
        << "Problema de los productores-consumidores (Monitor SU, buffer FIFO, múltiples prod/cons). " << endl
        << "--------------------------------------------------------------------------------------" << endl
        << flush ;

   // comprobar que las constantes tienen valores correctos
   assert( num_items % num_hebras_cons == 0 );
   assert( num_items % num_hebras_prod == 0 );
   assert( num_items % 5 == 0 );

   // crear monitor  ('monitor' es una referencia al mismo, de tipo MRef<...>)
   MRef<ProdConsMuSU> monitor = Create<ProdConsMuSU>() ;

   // crear y lanzar las hebras
   thread hebras_prod[num_hebras_prod],
          hebras_cons[num_hebras_cons] ;

   for( unsigned i = 0 ; i < num_hebras_prod ; i++ )
      hebras_prod[i] = thread( funcion_hebras_productoras, monitor, i );

   for( unsigned i = 0 ; i < num_hebras_cons ; i++ )
      hebras_cons[i] = thread( funcion_hebras_consumidoras, monitor, i );

   thread hebra_impresora( funcion_hebra_impresora, monitor );

   // esperar a que terminen las hebras
   for( unsigned i = 0 ; i < num_hebras_prod ; i++ )
      hebras_prod[i].join();
   for( unsigned i = 0 ; i < num_hebras_cons ; i++ )
      hebras_cons[i].join();

   hebra_impresora.join();

   test_contadores() ;
}
