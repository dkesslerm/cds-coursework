// -----------------------------------------------------------------------------
//
// Sistemas concurrentes y Distribuidos.
// Práctica 3. Implementación de algoritmos distribuidos con MPI
//
// Archivo: p31_gasolinera_1surt.cpp
// Implementación del problema de gasolinera con un único tipo de surtidor
// Plantilla para completar.
//
// Historial:
// Actualizado a C++11 en Septiembre de 2017
// -----------------------------------------------------------------------------


#include <mpi.h>
#include <thread> // this_thread::sleep_for
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include <iostream>

using namespace std;
using namespace std::this_thread ;
using namespace std::chrono ;

// constantes

const int
   num_surtidores   = 3,   // número de surtidores
   num_coches       = 7,   // número de coches
   id_gasolinera    = num_coches , // identificador del proceso de la gasolinera
   num_procesos     = num_coches + 1 , // número total de procesos
   etiq_empezar     = 0 ,  // etiqueta para empezar a repostar
   etiq_terminar    = 1 ;  // etiqueta usada para finalizar de repostar

//----------------------------------------------------------------------
/// @brief plantilla de función para generar un entero aleatorio uniformemente
/// distribuido entre dos valores enteros, ambos incluidos
/// (ambos tienen que ser dos constantes, conocidas en tiempo de compilación)
/// @tparam min - valor mínimo (int)
/// @tparam max - valor máximo (int)
/// @return número 'int' aleatorio uniformemente distribuido entew 'min' y 'max', ambos incluidos
///
template< int min, int max > int aleatorio()
{
  static default_random_engine generador( (random_device())() );
  static uniform_int_distribution<int> distribucion_uniforme( min, max ) ;
  return distribucion_uniforme( generador );
}

// ---------------------------------------------------------------------
/// @brief función que ejecutan los procesos 'coche'
/// @param id identificador (número de orden) del coche, empezando en 0
///
void funcion_coche( int id )
{
  int valor_env = 1; // valor a enviar

   while ( true )
   {
      cout << "El coche "<< id << " está solicitando entrar a algún surtidor " << endl;

      MPI_Ssend( &valor_env, 1, MPI_INT, id_gasolinera, etiq_empezar, MPI_COMM_WORLD );
      this_thread::sleep_for( chrono::milliseconds( aleatorio<20,200>() ) );
      MPI_Ssend( &valor_env, 1, MPI_INT, id_gasolinera, etiq_terminar, MPI_COMM_WORLD );
   }
}

// ---------------------------------------------------------------------
/// @brief función que ejecutan los procesos gasolinera
///
void funcion_gasolinera( )
{
   int
      valor          = 0,  // valor recibido o enviado
      id_coche       = 0,  // identificador del coche que quiere empezar a repostar o acabar
      ns_libres      = num_surtidores ,  // número de surtidores libres ahora mismo
      etiq_aceptable = 0;  // etiqueta aceptable (dependen del número de surtidores libres)
   MPI_Status
      estado ;                // metadatos de las dos recepciones

   while ( true )
   {
      // (1) determinar la etiqueta o etiquetas que se pueden aceptar:

      if ( ns_libres > 0)
         etiq_aceptable = MPI_ANY_TAG;
      else
         etiq_aceptable = etiq_terminar;

      // (2) recibir el mensaje con esa etiqueta o etiquetas

      MPI_Recv( &valor, 1, MPI_INT, MPI_ANY_SOURCE, etiq_aceptable, MPI_COMM_WORLD, &estado );

      // (3) procesar el mensaje recibido.

      switch( estado.MPI_TAG )
      {
         case etiq_empezar :
            cout << "\tGASOLINERA: El coche " << estado.MPI_SOURCE << " ha empezado a repostar " << endl;
            ns_libres -- ;
            break ;

         case etiq_terminar:
            cout << "\tGASOLINERA: El coche " << id_coche << " ha terminado de repostar " << endl;
            ns_libres ++ ;
            break ;
     }
  }
}
// ---------------------------------------------------------------------

int main( int argc, char** argv )
{
   int id_propio, num_procesos_actual ;

   MPI_Init( &argc, &argv );
   MPI_Comm_rank( MPI_COMM_WORLD, &id_propio );
   MPI_Comm_size( MPI_COMM_WORLD, &num_procesos_actual );


   if ( num_procesos == num_procesos_actual )
   {
      if( id_propio == id_gasolinera )
         funcion_gasolinera();
      else
         funcion_coche( id_propio );
   }
   else
   {
      if ( id_propio == 0 ) // solo el primero escribe error, indep. del rol
      {
         cout
            << "el número de procesos esperados es:    " << num_procesos << endl
            << "el número de procesos en ejecución es: " << num_procesos_actual << endl
            << "(programa abortado)" << endl ;
      }
   }

   MPI_Finalize( );
   return 0;
}

// ---------------------------------------------------------------------
