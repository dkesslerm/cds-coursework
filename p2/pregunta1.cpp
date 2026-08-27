// g++ -std=c++11 -pthread -o impresora p11_pc_impresora.cpp scd.cpp

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
#include "scd.h" // incluye tipo 'Semaphore', entre otros

using namespace std ; // permite acortar la notación ('abc' en lugar de 'std::abc')
using namespace scd ; // permite usar 'Semaphore' en lugar de 'scd::Semaphore'

// constantes y variables globales (compartidas)
const int tam_vector   = 6 ;   // tamaño del vector
const int num_cajas    = 24 ;   // número de cajas a producir
const int num_empleados = 3;
const int num_transportistas = 2;

int buffer[tam_vector] = {0};

int num_cajas_procesadas = 0;
int num_casillas_ocupadas  = 0 ;

mutex
   mtx ;                 // mutex de escritura en pantalla

// semáforos
Semaphore libres          = tam_vector , // semáforo con el número de casillas libres
          ocupadas        = 0 , // semáforo con el número de casillas ocupadas
          s_supervisor       = 0 , // semáforo que se pone a 1 para señalar que la impresora debe imprimir una vez
          productora      = 0 , // semáforo que se pone a 1 para señalar que el productor puede continuar una vez
          seccion_critica = 1 ; // semáforo para exclusión mútua para manipular la variable de cuentas


// -----------------------------------------------------------------------------
// funcion que produce un valor (produce los valores en secuencia)

unsigned producir_caja(int id, int i)
{
  this_thread::sleep_for ( chrono::milliseconds(aleatorio<20,100>())); // retraso aleatorio

  const unsigned caja_producida = id * num_cajas / num_empleados + i;
  mtx.lock();
  cout << "empleado " << id << " inserta caja " << caja_producida
       << " (ocupadas: " << num_casillas_ocupadas << ")" << endl;
  mtx.unlock();
  return caja_producida;
}

// -----------------------------------------------------------------------------
// función que consume un valor (retraso aleatorio + imprimir)

void retirar_caja ( int id, int caja )
{
    this_thread::sleep_for ( chrono::milliseconds(aleatorio<20,100>())); // retraso aleatorio

    mtx.lock();
    cout << "transportista " << id << " retira caja " << caja
        << " (ocupadas: " << num_casillas_ocupadas << ")" << endl;
    mtx.unlock();

}


// -----------------------------------------------------------------------------
// función que ejecuta la hebra productora (escribe en el vector)
// (escribe los valores desde 0 hasta 'num_items-1', ambos incluidos)

void empleado(int id)
{
   unsigned primera_libre = 0;
    int num_cajas_por_empleado = num_cajas / num_empleados;
    // se dividen las cajas entre los empleados
   for( unsigned long i = 0 ; i < num_cajas_por_empleado ; i++ )
   {
      unsigned caja = producir_caja(id, i);

      sem_wait(libres); // esperar a que haya al menos una celda libre y decrementar su número

         sem_wait(seccion_critica);
            buffer[primera_libre] = caja;
            primera_libre = (primera_libre+1) % tam_vector;
            num_casillas_ocupadas++;
         sem_signal(seccion_critica);

      sem_signal(ocupadas); // incrementa el semáforo con el numero de celdas ocupadas.
   }
}

// -----------------------------------------------------------------------------
// función que ejecuta la hebra consumidora (lee el vector)
// (lee los valores desde 0 hasta 'num_items-1 ', ambos incluidos)

void transportista( int id )
{
   unsigned primera_ocupada = 0; // para acceder al vector
    int num_cajas_por_transportista = num_cajas / num_transportistas;
    // se dividen las cajas entre los transportistas
   for( unsigned long i = 0 ; i < num_cajas_por_transportista ; i++ )
   {
      unsigned caja;

      sem_wait( ocupadas ); // esperar a que haya al menos una celda ocupada y decrementar su numero

         sem_wait( seccion_critica );
            caja = buffer[primera_ocupada];
            primera_ocupada = (primera_ocupada+1) % tam_vector;
            num_casillas_ocupadas--;
            num_cajas_procesadas++; // aumentamos las cajas procesadas
            // cada 6 cajas procesadas llamamos al supervisor
            if (num_cajas_procesadas % 6 == 0)
            {
               sem_signal( s_supervisor );
               sem_wait( productora );
            }

         sem_signal( seccion_critica );

      sem_signal( libres );  // incrementa el semáforo con el número de celdas libres
      retirar_caja( id, caja );
   }
}

// -----------------------------------------------------------------------------
// función que ejecuta la hebra supervisora

void supervisor (  )
{
   while( num_cajas_procesadas < num_cajas)
   {
      sem_wait(s_supervisor);
    // comprobamos que no tengamos que terminar
      if (num_cajas_procesadas < num_cajas) {
        mtx.lock();
        cout << "---- SUPERVISOR: registradas 6 cajas ----" << endl;
        mtx.unlock();
      }
      sem_signal(productora);
   }
}


int main()
{
    // multiples empleados y transportistas
    thread hebra_empleada[num_empleados],
           hebra_transportista[num_transportistas],
           hebra_supervisor( supervisor );

    for(int i = 0; i < num_empleados; i++) {
        hebra_empleada[i] = thread(empleado, i);
    }
    for(int i = 0; i < num_transportistas; i++) {
        hebra_transportista[i] = thread(transportista, i);
    }

   // esperar a que terminen todas las hebras

    for(int i = 0; i < num_empleados; i++) {
        hebra_empleada[i].join();
    }
    for (int i = 0; i < num_transportistas; i++) {
        hebra_transportista[i].join();
    }
   hebra_supervisor.join();
}
