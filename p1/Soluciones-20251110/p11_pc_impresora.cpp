// g++ -std=c++11 -pthread -o impresora Soluciones-20251110/p11_pc_impresora.cpp scd.cpp

/**
 * @file   prodcons-impresora.cpp
 * @author Carlos Ureña
 * @date 2022-10-16
 * @brief
 *  Prod-cons (1-prod, 1-cons, FIFO), con hebra impresora adicional (ver enunciado en el PDF)
 */

#include <iostream>
#include <thread>
#include <cmath>
#include "../scd.h" // incluye tipo 'Semaphore', entre otros

using namespace std ; // permite acortar la notación ('abc' en lugar de 'std::abc')
using namespace scd ; // permite usar 'Semaphore' en lugar de 'scd::Semaphore'

// constantes y variables globales (compartidas)
const int tam_vector   = 10 ;   // tamaño del vector
const int num_items    = 20 ;   // número de items
const int cada_cuantos = 5;
const int vueltas_impr = num_items/cada_cuantos; // iteraciones de la impresora

int buffer[tam_vector] = {0};

int siguiente_dato         = 0 ;
int num_casillas_ocupadas  = 0 ;

// semáforos
Semaphore libres          = tam_vector , // semáforo con el número de casillas libres
          ocupadas        = 0 , // semáforo con el número de casillas ocupadas
          impresora       = 0 , // semáforo que se pone a 1 para señalar que la impresora debe imprimir una vez
          productora      = 0 , // semáforo que se pone a 1 para señalar que el productor puede continuar una vez
          seccion_critica = 1 ; // semáforo para exclusión mútua para manipular la variable de cuentas


// -----------------------------------------------------------------------------
// funcion que produce un valor (produce los valores en secuencia)

unsigned producir_dato()
{
  this_thread::sleep_for ( chrono::milliseconds(aleatorio<20,100>())); // retraso aleatorio

  const unsigned dato_producido = siguiente_dato;
  siguiente_dato++;

  cout << "Producido: " << dato_producido << endl;
  return dato_producido;
}

// -----------------------------------------------------------------------------
// función que consume un valor (retraso aleatorio + imprimir)

void consumir_dato ( int dato )
{
   this_thread::sleep_for ( chrono::milliseconds(aleatorio<20,100>())); // retraso aleatorio

   cout << "Consumidor: dato consumido: " << dato << endl;
}


// -----------------------------------------------------------------------------
// función que ejecuta la hebra productora (escribe en el vector)
// (escribe los valores desde 0 hasta 'num_items-1', ambos incluidos)

void funcion_hebra_productora(  )
{
   unsigned primera_libre = 0;

   for( unsigned long i = 0 ; i < num_items ; i++ )
   {
      unsigned dato = producir_dato();

      sem_wait(libres); // esperar a que haya al menos una celda libre y decrementar su número

         sem_wait(seccion_critica);
            buffer[primera_libre] = dato;
            primera_libre = (primera_libre+1) % tam_vector;
            num_casillas_ocupadas++;
            if (dato % cada_cuantos == 0)
            {
               sem_signal( impresora );
               sem_wait( productora );
            }
         sem_signal(seccion_critica);

      sem_signal(ocupadas); // incrementa el semáforo con el numero de celdas ocupadas.
   }
}

// -----------------------------------------------------------------------------
// función que ejecuta la hebra consumidora (lee el vector)
// (lee los valores desde 0 hasta 'num_items-1 ', ambos incluidos)

void funcion_hebra_consumidora(  )
{
   unsigned primera_ocupada = 0; // para acceder al vector

   for( unsigned long i = 0 ; i < num_items ; i++ )
   {
      unsigned dato;

      sem_wait( ocupadas ); // esperar a que haya al menos una celda ocupada y decrementar su numero

         sem_wait( seccion_critica );
            dato = buffer[primera_ocupada];
            primera_ocupada = (primera_ocupada+1) % tam_vector;
            num_casillas_ocupadas--;
         sem_signal( seccion_critica );

      sem_signal( libres );  // incrementa el semáforo con el número de celdas libres
      consumir_dato( dato );
   }
}

// -----------------------------------------------------------------------------
// función que ejecuta la hebra impresora
// (mira cuántas casillas están ocupadas)

void funcion_hebra_impresora(  )
{
   for( unsigned long i = 0 ; i < vueltas_impr ; i++ )
   {
      sem_wait(impresora);
      cout << "Impresora: número de casillas ocupadas: " << num_casillas_ocupadas << endl;
      sem_signal(productora);
   }
}

// -----------------------------------------------------------------------------
// hebra principal (pone las otras dos en marcha)

int main()
{

   // crear y poner en marcha las dos hebras
   thread
      hebra_productora( funcion_hebra_productora ),
      hebra_consumidora( funcion_hebra_consumidora ),
      hebra_impresora( funcion_hebra_impresora );

   // esperar a que terminen todas las hebras
   hebra_productora.join();
   hebra_consumidora.join();
   hebra_impresora.join();

}
