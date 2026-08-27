// mpicxx -std=c++11 -o gasolinera-3_exe_mpi gasolinera-3.cpp
// mpirun -oversubscribe -np 15 ./gasolinera-3_exe_mpi
// DNI: 23300373Q
// Apellidos, Nombre: Kessler Martínez, David

// -----------------------------------------------------------------------------
// Sistemas concurrentes y Distribuidos.
// Práctica 3. Implementación de algoritmos distribuidos con MPI
//
// Archivo: p31_gasolinera_1surt.cpp
// Implementación del problema de gasolinera con tres tipos de surtidores.
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
    int tipo_combustible = aleatorio<0, 2>();
    while (true) {
        cout << "Coche " << id << " manda petición para repostar combustible tipo " << tipo_combustible << endl;
        MPI_Ssend(&tipo_combustible, 1, MPI_INT, id_gasolinera, tipo_combustible, MPI_COMM_WORLD);

        this_thread::sleep_for(chrono::milliseconds(aleatorio<20, 100>()));

        cout << "Coche " << id << " termina de repostar combustible tipo " << tipo_combustible << endl;
        MPI_Ssend(&tipo_combustible, 1, MPI_INT, id_gasolinera, etiq_terminar, MPI_COMM_WORLD);

        // Una vez que ya ha repostado, hacemos una espera hasta que se le acabe la gasolina
        this_thread::sleep_for(chrono::milliseconds(aleatorio<100, 1000>()));
     }
}

// ---------------------------------------------------------------------
/// @brief función que ejecutan los procesos gasolinera
///
void funcion_gasolinera( ) {
    int valor = 0;
    MPI_Status estado;
    bool hayAlguien = false;
    int hayTerminar = 0;
    while (true) {
        MPI_Iprobe(MPI_ANY_SOURCE, etiq_terminar, MPI_COMM_WORLD, &hayTerminar, &estado);
        if (hayTerminar > 0) {
            // Hay algún coche que quiere terminar
            MPI_Recv(&valor, 1, MPI_INT, MPI_ANY_SOURCE, etiq_terminar, MPI_COMM_WORLD, &estado);
            num_surtidores_libres[valor]++;
            hayAlguien = true;
            cout << "\tGASOLINERA: El coche " << estado.MPI_SOURCE << " termina de repostar combustible tipo " << valor
                 << ". Número de surtidores disponibles: " << num_surtidores_libres[valor] << endl;
        }

        for (int i = 0; i < num_tipos_combustible; i++) {
            int hayEsperando = 0;
            if (num_surtidores_libres[i] > 0) {
                MPI_Iprobe(MPI_ANY_SOURCE, i, MPI_COMM_WORLD, &hayEsperando, &estado);
                if (hayEsperando > 0) {
                    MPI_Recv(&valor, 1, MPI_INT, estado.MPI_SOURCE, i,  MPI_COMM_WORLD, &estado);
                    num_surtidores_libres[valor]--;
                    hayAlguien = true;
                    cout << "\tGASOLINERA: El coche " << estado.MPI_SOURCE << " comienza a repostar combustible tipo " << valor
                            << ". Número de surtidores disponibles: " << num_surtidores_libres[valor] << endl;
                }
            }
        }

        if (!hayAlguien) {
            this_thread::sleep_for(chrono::milliseconds(20));
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
