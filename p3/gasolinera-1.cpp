// mpicxx -std=c++11 -o gasolinera-1_exe_mpi gasolinera-1.cpp
// mpirun -oversubscribe -np 8 ./gasolinera-1_exe_mpi

// -----------------------------------------------------------------------------
// Sistemas concurrentes y Distribuidos.
// Práctica 3. Implementación de algoritmos distribuidos con MPI
//
// Archivo: p31_gasolinera_1surt.cpp
// Implementación del problema de gasolinera con un único tipo de surtidor
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
    int peticion;
    while (true) {
        cout << "Coche " << id << " manda petición para repostar" << endl;
        MPI_Ssend(&peticion, 1, MPI_INT, id_gasolinera, etiq_empezar, MPI_COMM_WORLD);

        this_thread::sleep_for(chrono::milliseconds(aleatorio<20, 100>()));

        cout << "Coche " << id << " sale de la gasolinera" << endl;
        MPI_Ssend(&peticion, 1, MPI_INT, id_gasolinera, etiq_terminar, MPI_COMM_WORLD);

        // Una vez que ya ha repostado, espera a que se le acabe la gasolina
        this_thread::sleep_for(chrono::milliseconds(aleatorio<100, 1000>()));
    }
}

// ---------------------------------------------------------------------
/// @brief función que ejecutan los procesos gasolinera
///
void funcion_gasolinera( )
{
    int num_surtidores_disponibles = num_surtidores;
    int valor;
    MPI_Status estado;
    while (true) {
        if (num_surtidores_disponibles > 0) {
            MPI_Recv(&valor, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &estado);
        } else {
            MPI_Recv(&valor, 1, MPI_INT, MPI_ANY_SOURCE, etiq_terminar, MPI_COMM_WORLD, &estado);
        }

        if (estado.MPI_TAG == etiq_empezar) {
            num_surtidores_disponibles--;
            cout << "\tGASOLINERA: El coche " << estado.MPI_SOURCE << " comienza a repostar."
                 << " Número de surtidores disponibles: " << num_surtidores_disponibles << endl;
        } else {
            num_surtidores_disponibles++;
            cout << "\tGASOLINERA: El coche " << estado.MPI_SOURCE << " termina de repostar."
                 << " Número de surtidores disponibles: " << num_surtidores_disponibles << endl;
        }
    }
}

// ---------------------------------------------------------------------

int main( int argc, char** argv )
{
    int id_propio, num_procesos_actual;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &id_propio);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procesos_actual);

    if (num_procesos == num_procesos_actual) {
        if (id_propio == id_gasolinera) {
            funcion_gasolinera();
        } else {
            funcion_coche(id_propio);
        }
    } else {
        if (id_propio == 0) { // el primero escribe el error
            cout << "el número de procesos esperados es:    " << num_procesos << endl
                 << "el número de procesos en ejecución es: " << num_procesos_actual << endl
                 << "(programa abortado)" << endl ;
        }
    }

    MPI_Finalize();
    return 0;
}

// ---------------------------------------------------------------------
