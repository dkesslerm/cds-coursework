// -----------------------------------------------------------------------------
//
// Sistemas concurrentes y Distribuidos.
// Práctica 3. Implementación de algoritmos distribuidos con MPI
//
// Archivo: p31_gasolinera_1surt.cpp
// Implementación del problema de gasolinera con tres tipos de surtidores.
// (Problema 3.2)
// Es un ejemplo de usa de MPI_IProbe + MPI_Recv, no necesita MPI_Probe
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
constexpr int
   num_tipos_combustible = 3;

const int
   num_surtidores[num_tipos_combustible] = { 2, 3, 4 } ,   // número de surtidores totales, según el tipo de combustible
   num_coches          = 14,   // número de coches
   id_gasolinera       = num_coches , // identificador del proceso de la gasolinera
   num_procesos        = num_coches + 1 , // número total de procesos
   etiq_terminar       = num_tipos_combustible  ;         // etiqueta usada para finalizar de repostar
         // las etiquetas usadas para 'inicio de repostar' son las etiquetas desde 0 hasta num_tipos_combustible-1

int
   num_surtidores_libres[ num_tipos_combustible ] = { num_surtidores[0], num_surtidores[1], num_surtidores[2] };

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

  const int tipo_combustible  = aleatorio<0,num_tipos_combustible-1>() ; // tipo de combustible (0,1,2)

   while ( true )
   {
      cout << "El coche "<< id << " está solicitando entrar a surtidor tipo " << tipo_combustible << endl;

      // el valor entero que se envía coincide con el tipo de combustible (es necesario si queremos que el
      // proceso gasolinera imprima el tipo de surtidor cuando se acaba de repostar)
      MPI_Ssend( &tipo_combustible, 1, MPI_INT, id_gasolinera, tipo_combustible, MPI_COMM_WORLD );
      cout << "El coche "<< id << " inicia repostaje en surtidor tipo " << tipo_combustible << endl;
      this_thread::sleep_for( chrono::milliseconds( aleatorio<20,200>() ) );
      cout << "El coche "<< id << " acaba repostaje en surtidor tipo " << tipo_combustible << endl;
      MPI_Ssend( &tipo_combustible, 1, MPI_INT, id_gasolinera, etiq_terminar, MPI_COMM_WORLD );
   }
}

// ---------------------------------------------------------------------
/// @brief función que ejecutan los procesos gasolinera
///
void funcion_gasolinera( )
{
   int        valor_rec      = 0,  // valor recibido
              etiq_aceptable = 0;  // etiqueta aceptable (dependen del número de surtidores libres)
   MPI_Status estado ;             // metadatos de las dos recepciones

   while ( true )
   {
      int hay_msg = 0 ; // indica si se ha recibido un mensaje de cualquier tipo en esta iteración (>1 sí, 0 no)

      // comprobar y procesar mensaje de terminar
      MPI_Iprobe( MPI_ANY_SOURCE, etiq_terminar, MPI_COMM_WORLD, &hay_msg, &estado );
      if ( hay_msg > 0 )
      {
         MPI_Recv( &valor_rec, 1, MPI_INT, estado.MPI_SOURCE, etiq_terminar, MPI_COMM_WORLD, &estado );
         num_surtidores_libres[valor_rec] ++ ;
         cout << "Gasolinera: procesado 'fin de repostar' de coche " << estado.MPI_SOURCE << " para surtidor tipo " << valor_rec << endl ;
      }

      // comprobar y aceptar mensajes de inicio de repostar con surtidores disponible, por orden de tipos.
      for( int tipo_comb = 0 ; tipo_comb < num_tipos_combustible ; tipo_comb++ )
         if ( num_surtidores_libres[ tipo_comb ] > 0 )
         {
            int hay_msg_tipo = 0 ;
            MPI_Iprobe( MPI_ANY_SOURCE, tipo_comb, MPI_COMM_WORLD, &hay_msg_tipo, &estado );
            if ( hay_msg_tipo > 0 )
            {
               MPI_Recv( &valor_rec, 1, MPI_INT, estado.MPI_SOURCE, tipo_comb, MPI_COMM_WORLD, &estado );
               num_surtidores_libres[ tipo_comb ] -- ;
               hay_msg = 1 ;
               cout << "Gasolinera: procesado 'inicio de repostar' de coche " << estado.MPI_SOURCE << " para surtidor tipo " << valor_rec << endl ;
            }
         }

      // si no hay mensajes aceptables, esperar 20 milisegundos antes de volver a intentarlo.
      if ( hay_msg == 0 )
         this_thread::sleep_for( chrono::milliseconds( 20 ));
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
