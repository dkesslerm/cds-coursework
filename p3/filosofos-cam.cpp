// mpicxx -std=c++11 -o filosofos-cam_exe_mpi filosofos-cam.cpp
// mpirun -oversubscribe -np 11 ./filosofos-cam_exe_mpi

// -----------------------------------------------------------------------------
//
// Sistemas concurrentes y Distribuidos.
// Práctica 3. Implementación de algoritmos distribuidos con MPI
//
// Archivo: filosofos-plantilla.cpp
// Implementación del problema de los filósofos (sin camarero).
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

const int
   num_filosofos = 5 ,              // número de filósofos
   num_filo_ten  = 2*num_filosofos, // número de filósofos y tenedores
   num_procesos  = num_filo_ten + 1,   // número de procesos total (ahora con camarero)
   id_camarero = num_procesos - 1;


const int etiq_sentarse = 0,
          etiq_levantarse = 1,
          etiq_tenedor = 2;

//**********************************************************************
// plantilla de función para generar un entero aleatorio uniformemente
// distribuido entre dos valores enteros, ambos incluidos
// (ambos tienen que ser dos constantes, conocidas en tiempo de compilación)
//----------------------------------------------------------------------

template< int min, int max > int aleatorio()
{
  static default_random_engine generador( (random_device())() );
  static uniform_int_distribution<int> distribucion_uniforme( min, max ) ;
  return distribucion_uniforme( generador );
}

// ---------------------------------------------------------------------

void funcion_filosofos( int id )
{
  int id_filosofo = id / 2;
  int id_ten_izq = (id+1)              % num_filo_ten, //id. tenedor izq.
      id_ten_der = (id+num_filo_ten-1) % num_filo_ten; //id. tenedor der.

  int peticion = 1;

  while ( true )
  {
    // CAMARERO
    cout << "Filósofo" << id_filosofo << " solicita sentarse en una mesa" << endl;
    MPI_Ssend(&peticion, 1, MPI_INT, id_camarero, etiq_sentarse, MPI_COMM_WORLD);
    // FIN CAMARERO

    cout <<"Filósofo " << id_filosofo << " solicita ten. izq." << (id_ten_izq - 1) / 2 << endl;
    MPI_Ssend(&peticion, 1, MPI_INT, id_ten_izq, etiq_tenedor, MPI_COMM_WORLD);

    cout <<"Filósofo " << id_filosofo <<" solicita ten. der." << (id_ten_der - 1) / 2 << endl;
    MPI_Ssend(&peticion, 1, MPI_INT, id_ten_der, etiq_tenedor, MPI_COMM_WORLD);

    cout <<"Filósofo " << id_filosofo <<" comienza a comer" <<endl ;
    sleep_for( milliseconds( aleatorio<10,100>() ) );

    cout <<"Filósofo " << id_filosofo <<" suelta ten. izq. " <<(id_ten_izq - 1) / 2 << endl;
    MPI_Ssend(&peticion, 1, MPI_INT, id_ten_izq, etiq_tenedor, MPI_COMM_WORLD);

    cout<< "Filósofo " << id_filosofo <<" suelta ten. der. " << (id_ten_der - 1) / 2 << endl;
    MPI_Ssend(&peticion, 1, MPI_INT, id_ten_der, etiq_tenedor, MPI_COMM_WORLD);

    // CAMARERO
    cout << "Filósofo " << id_filosofo << " deja libre una mesa" << endl;
    MPI_Ssend(&peticion, 1, MPI_INT, id_camarero, etiq_levantarse, MPI_COMM_WORLD);
    // FIN CAMARERO

    cout << "Filosofo " << id_filosofo << " comienza a pensar" << endl;
    sleep_for( milliseconds( aleatorio<10,100>() ) );
 }
}
// ---------------------------------------------------------------------

void funcion_tenedores( int id )
{
  int valor, id_filosofo ;  // valor recibido, identificador del filósofo
  MPI_Status estado ;       // metadatos de las dos recepciones

  while ( true )
  {
     MPI_Recv(&valor, 1, MPI_INT, MPI_ANY_SOURCE, etiq_tenedor , MPI_COMM_WORLD, &estado);
     id_filosofo = estado.MPI_SOURCE / 2;
     cout << "Ten. " << (id - 1) / 2 <<" ha sido cogido por filo. " << id_filosofo << endl;

     MPI_Recv(&valor, 1, MPI_INT, estado.MPI_SOURCE, etiq_tenedor, MPI_COMM_WORLD, &estado);
     cout << "Ten. "<< (id - 1) / 2 << " ha sido liberado por filo. " << id_filosofo << endl ;
  }
}
// ---------------------------------------------------------------------

void funcion_camarero(int id) {
    int s = 0;
    int valor;
    MPI_Status estado;
    while (true) {
        // Aceptamos peticiones con etiq_levantarse siempre
        if (s < num_filosofos - 1) {
            MPI_Recv(&valor, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &estado);
        } else {
            MPI_Recv(&valor, 1, MPI_INT, MPI_ANY_SOURCE, etiq_levantarse, MPI_COMM_WORLD, &estado);
        }

        if (estado.MPI_TAG == etiq_sentarse) {
            cout << "\tCAMARERO: He sentado al filosofo " << estado.MPI_SOURCE / 2
                 << ". Quedan " << num_filosofos - 1 - s << " mesas libres." << endl;
            s++;
        } else { // etiq_levantarse
            s--;
            cout << "\tCAMARERO: Se ha levantado el filosofo " << estado.MPI_SOURCE / 2
                 << ". Quedan " << num_filosofos - 1 - s << " mesas libres." << endl;
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
      // ejecutar la función correspondiente a 'id_propio'
      if (id_propio == 10) {
          funcion_camarero(id_propio);
      } else if ( id_propio % 2 == 0 )   // si es par
         funcion_filosofos( id_propio ); //   es un filósofo
      else                               // si es impar
         funcion_tenedores( id_propio ); //   es un tenedor

   }
   else
   {
      if ( id_propio == 0 ) // solo el primero escribe error, indep. del rol
      { cout << "el número de procesos esperados es:    " << num_procesos << endl
             << "el número de procesos en ejecución es: " << num_procesos_actual << endl
             << "(programa abortado)" << endl ;
      }
   }

   MPI_Finalize( );
   return 0;
}

// ---------------------------------------------------------------------
