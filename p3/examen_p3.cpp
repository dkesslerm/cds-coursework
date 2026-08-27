
// mpicxx -std=c++11 -o examen_p3_exe_mpi examen_p3.cpp
// mpirun -oversubscribe -np 26 ./examen_p3_exe_mpi

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
   num_tipos_bicicleta = 4;

const int
   disponibilidad_fija[num_tipos_bicicleta] = { 10, 6, 4, 2 } ,   // número de bicicletas totales, según el tipo de bicicleta
   disponibilidad_fija_total = disponibilidad_fija[0] + disponibilidad_fija[1] + disponibilidad_fija[2] + disponibilidad_fija[3],
   num_usuarios          = 25,   // número de usuarios
   id_estacion       = num_usuarios , // identificador del proceso de la estacion
   num_procesos        = num_usuarios + 1 , // número total de procesos
   etiq_terminar       = num_tipos_bicicleta  ;         // etiqueta usada para finalizar el uso de la bicicleta
         // las etiquetas usadas para 'inicio de repostar' son las etiquetas desde 0 hasta num_tipos_bicicleta-1

int
   contador_usos = 0,
   num_bicicletas_libres[ num_tipos_bicicleta ] = { disponibilidad_fija[0], disponibilidad_fija[1], disponibilidad_fija[2], disponibilidad_fija[3]},
   num_bicicletas_libres_totales = disponibilidad_fija_total;

// Nombres de las bicicletas
string nombre_bicicleta[num_tipos_bicicleta] = {"Urbana", "Montaña", "Eléctrica", "Cargo"};

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
/// @brief función que ejecutan los procesos 'usuario'
/// @param id identificador (número de orden) del usuario, empezando en 0
///
void funcion_usuario( int id )
{
    int tipo_bicicleta = aleatorio<0, 3>();
    while (true) {
        cout << "Usuario " << id << " manda petición para usar una bicicleta " << nombre_bicicleta[tipo_bicicleta] << endl;
        MPI_Ssend(&tipo_bicicleta, 1, MPI_INT, id_estacion, tipo_bicicleta, MPI_COMM_WORLD);

        // Simulamos espera durante uso de bicicleta
        this_thread::sleep_for(chrono::milliseconds(aleatorio<20, 100>()));

        cout << "Usuario " << id << " termina de usar la bicicleta de tipo " << nombre_bicicleta[tipo_bicicleta] << endl;
        MPI_Ssend(&tipo_bicicleta, 1, MPI_INT, id_estacion, etiq_terminar, MPI_COMM_WORLD);

        // Una vez que ya ha terminado de utilizar la bicicleta, hacemos una espera hasta que quiera volver a utilizar la bicicleta
        this_thread::sleep_for(chrono::milliseconds(aleatorio<100, 1000>()));
     }
}

// ---------------------------------------------------------------------
/// @brief función que ejecutan los procesos estacion
///
void funcion_estacion( ) {
    int valor = 0;
    MPI_Status estado;
    int hayMensaje = 0; // Si ha llegado algún mensaje
    while (true) {
        MPI_Iprobe(MPI_ANY_SOURCE, etiq_terminar, MPI_COMM_WORLD, &hayMensaje, &estado);
        if (hayMensaje > 0) {
            // Hay algún usuario que quiere terminar su uso de bicicleta
            MPI_Recv(&valor, 1, MPI_INT, MPI_ANY_SOURCE, etiq_terminar, MPI_COMM_WORLD, &estado);
            num_bicicletas_libres[valor]++; // aumentamos las bicicletas libres de ese tipo
            num_bicicletas_libres_totales++; // aumentamos las bicicletas libres totales
            contador_usos++; // aumentamos las bicicletas procesadas (para el resumen

            // Informamos que el usuario abandona la estación
            cout << "\tfin de uso. Estación: finaliza usuario " << estado.MPI_SOURCE << " con bicicleta " << nombre_bicicleta[valor]
                 << ", disponibilidad " << num_bicicletas_libres_totales << endl;

            // Si hemos acumulado 5 usos totales, se imprime el resumen
            if (contador_usos == 5) {
                double porcentaje_global_libres =(double)num_bicicletas_libres_totales / disponibilidad_fija_total * 100.0;
                double porcentaje_global_ocupadas = 100 - porcentaje_global_libres;
                cout << "**************************************" << endl;
                cout << "Total de bicicletas en uso por cada tipo" << endl;
                cout << nombre_bicicleta[0] << ": " << disponibilidad_fija[0] - num_bicicletas_libres[0] << " de " << disponibilidad_fija[0] << endl;
                cout << nombre_bicicleta[1] << ": " << disponibilidad_fija[1] - num_bicicletas_libres[1] << " de " << disponibilidad_fija[1] << endl;
                cout << nombre_bicicleta[2] << ": " << disponibilidad_fija[2] - num_bicicletas_libres[2] << " de " << disponibilidad_fija[2] << endl;
                cout << nombre_bicicleta[3] << ": " << disponibilidad_fija[3] - num_bicicletas_libres[3] << " de " << disponibilidad_fija[3] << endl;
                cout << "Total de bicicletas en uso global: " << disponibilidad_fija_total - num_bicicletas_libres_totales << endl;
                cout << "Porcentaje global de uso: " << porcentaje_global_ocupadas << "%" << endl;
                cout << "Porcentaje global de disponibilidad: " << porcentaje_global_libres << "%" << endl;
                cout << "**************************************" << endl;

                contador_usos = 0;
            }
        }

        for (int i = 0; i < num_tipos_bicicleta; i++) {
            int hayEsperando = 0;
            if (num_bicicletas_libres[i] > 0) {
                MPI_Iprobe(MPI_ANY_SOURCE, i, MPI_COMM_WORLD, &hayEsperando, &estado);
                if (hayEsperando > 0) {
                    MPI_Recv(&valor, 1, MPI_INT, estado.MPI_SOURCE, i,  MPI_COMM_WORLD, &estado);
                    num_bicicletas_libres[valor]--;
                    num_bicicletas_libres_totales--;
                    hayMensaje = 1;
                    cout << "\tinicio tipo " << valor << ". Estación: inicia usuario " << estado.MPI_SOURCE << " con bicicleta " << nombre_bicicleta[valor]
                         << ", disponibilidad " << num_bicicletas_libres[valor] << endl;
                }
            }
        }

        // Si no hemos recibido ningún mensaje
        if (hayMensaje == 0) {
            cout << "En espera de solicitud de servicio..." << endl;
            this_thread::sleep_for(chrono::milliseconds(60));
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
      if( id_propio == id_estacion )
         funcion_estacion(); // 1 estacion, solo si coincide con el último proceso
      else
         funcion_usuario( id_propio );
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
