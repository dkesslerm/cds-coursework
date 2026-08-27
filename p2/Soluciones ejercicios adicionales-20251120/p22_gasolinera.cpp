// g++ -std=c++11 -pthread -o gasolinera Soluciones\ ejercicios\ adicionales-20251120/p22_gasolinera.cpp scd.cpp

/// -----------------------------------------------------------------------------
///
/// Sistemas concurrentes y Distribuidos.
/// Práctica 2. Casos prácticos de monitores en C++11.
/// Ejemplo de la gasolinera con un monitor SU
///
///  @brief
/// Implementar un programa con un monitor SU  (basado en la clase HoareMonitor) en un archivo
/// llamado p2_gasolinera.cpp, con los siguientes requerimientos:
///
///     (1) El programa está formado por 10 hebras que ejecutan un bucle infinito y que representan
///     a coches que necesitan entrar a repostar a una gasolinera un vez en cada iteración de su bucle.
///     Hay dos tipos de hebras coche: 4 hebras de tipo gasoil y 6 hebras de tipo gasolina.
///
///     (2) Para entrar a la gasolinera un coche debe esperar a que quede libre algún surtidor del
///     tipo que necesita. La gasolinera tiene 3 surtidores de gasolina y 2 para gasoil, inicialmente
///     todos libres.  Cada tipo de hebra ejecuta una función del programa distinta
///     (funcion_hebra_gasolina y funcion_hebra_gasoil)
///
///     (3) El tiempo de repostaje y la parte de código de los coches que no están en la gasolinera
///     se simulan con retardos aleatorios. Es necesario contabilizar en una variable compartida el número
///     total de surtidores en uso en cada momento. Los coches deben imprimir mensaje cuando logran
///     entrar en la gasolinera (tras esperar) y cuando salen, deben indicar: número y tipo de coche,
///     si entra o sale, y numero de surtidores ocupados.
///
/// El monitor debe tener cuatro métodos públicos: entra_coche_gasoil, sale_coche_gasoil,
/// entra_coche_gasolina, sale_coche_gasolina (todas tienen como parámetro el número de coche
/// dentro de su tipo). El pseudo-código de las hebras es así.
///
///
///  Historial:
///  Creado el 4 de Noviembre de 2022,
///
/// -----------------------------------------------------------------------------


#include <iostream>
#include <iomanip>
#include <cassert>
#include <random>
#include <thread>
#include "../scd.h"

using namespace std ;
using namespace scd ;

enum tipo_coche { gasolina, diesel };

// constantes y variables globales (compartidas)
constexpr unsigned
   num_surt[2]            = { 3, 2 },
   num_coches[2]          = { 6, 4 },
   tiempo_min_repostar_ms = 20,
   tiempo_max_repostar_ms = 200,
   tiempo_min_fuera_ms    = 20,
   tiempo_max_fuera_ms    = 200 ;

/** -------------------------------------------------------------------------- **/
/// @brief Monitor SU para el problema de la gasolinera de la prác. 2
///
class Gasolinera : public HoareMonitor
{
   private:

      // número de surtidores gasolina y diesel libres
      unsigned num_surt_libres[2] = { num_surt[0], num_surt[1] } ;

      // cola de coches esperando: num_surt_libres[tipo] >0
      CondVar hay_surt_libre[2] ;

      // número de surtidores ocupados (sumados de ambos tipos)
      unsigned num_surt_ocupados = 0 ;

   public:
      Gasolinera() ;
      void entra_coche_gasolina( unsigned ncg );
      void sale_coche_gasolina( unsigned ncg );
      void entra_coche_diesel( unsigned ncd );
      void sale_coche_diesel( unsigned ncd ) ;

} ;

/** -------------------------------------------------------------------------- **/
/// @brief Constructor (crea variables condición)
///
Gasolinera::Gasolinera()
{
   hay_surt_libre[tipo_coche::gasolina] = newCondVar() ;
   hay_surt_libre[tipo_coche::diesel]   = newCondVar() ;
}

/** -------------------------------------------------------------------------- **/
/// @brief realiza las esperas y actualizaciones previas al repostaje de coche diesel
/// @param ncd - (unsigned) número de orden de coche diesel
///
void Gasolinera::entra_coche_diesel( unsigned ncd )
{
   constexpr tipo_coche tc = tipo_coche::diesel ;

   if ( num_surt_libres[tc] == 0 )
      hay_surt_libre[tc].wait() ;

   assert( num_surt_libres[tc] > 0 );
   num_surt_libres[tc] -- ;
   num_surt_ocupados ++ ;

   cout << "entra coche de diesel " << ncd << ", n. surt. ocup == " << num_surt_ocupados << endl ;
}

/** -------------------------------------------------------------------------- **/
/// @brief realiza las actualizaciones posteriores al repostaje de coche diesel
/// @param ncd - (unsigned) número de orden de coche diesel
///
void Gasolinera::sale_coche_diesel( unsigned ncd )
{
   constexpr tipo_coche tc = tipo_coche::diesel ;
   num_surt_libres[tc] ++ ;
   num_surt_ocupados -- ;
   cout << "sale coche de diesel " << ncd << ", n. surt. ocup == " << num_surt_ocupados << endl ;

   hay_surt_libre[tc].signal() ;
}

/** -------------------------------------------------------------------------- **/
/// @brief realiza las esperas y actualizaciones previas al repostaje de coche gasoline
/// @param ncg - (unsigned) número de orden de coche diesel
///
void Gasolinera::entra_coche_gasolina( unsigned ncg )
{
   constexpr tipo_coche tc = tipo_coche::gasolina ;

   if ( num_surt_libres[tc] == 0 )
      hay_surt_libre[tc].wait() ;

   assert( num_surt_libres[tc] > 0 );
   num_surt_libres[tc] -- ;
   num_surt_ocupados ++ ;

   cout << "entra coche de gasoli " << ncg << ", n. surt. ocup == " << num_surt_ocupados << endl ;
}

/** -------------------------------------------------------------------------- **/
/// @brief realiza las actualizaciones posteriores al repostaje de coche gasolina
/// @param ncg - (unsigned) número de orden de coche gasolina
///
void Gasolinera::sale_coche_gasolina( unsigned ncg )
{
   constexpr tipo_coche tc = tipo_coche::gasolina ;

   num_surt_libres[tc] ++ ;
   num_surt_ocupados -- ;
   cout << "sale coche de gasoli " << ncg << ", n. surt. ocup == " << num_surt_ocupados << endl ;

   hay_surt_libre[tc].signal() ;
}

/** -------------------------------------------------------------------------- **/
/// @brief función que ejecutan las hebras de coches de gasolina
/// @param ncg - (unsigned) número de orden del coche de gasolina
///
void funcion_hebra_gasolina(  MRef<Gasolinera> gasolinera, unsigned ncg )
{
   while( true )
   {
      gasolinera->entra_coche_gasolina( ncg );
      this_thread::sleep_for(chrono::milliseconds( aleatorio<tiempo_min_repostar_ms,tiempo_max_repostar_ms>() ) );
      gasolinera->sale_coche_gasolina( ncg );

      this_thread::sleep_for(chrono::milliseconds( aleatorio<tiempo_min_fuera_ms,tiempo_max_fuera_ms>() ) );
   }
}

/** -------------------------------------------------------------------------- **/
/// @brief función que ejecutan las hebras de coches diesel (== de gasoil)
/// @param ncd - número de orden del coche diesel
///
void funcion_hebra_diesel(  MRef<Gasolinera> gasolinera, unsigned ncd )
{
   while( true )
   {
      gasolinera->entra_coche_diesel( ncd );
      this_thread::sleep_for(chrono::milliseconds( aleatorio<tiempo_min_repostar_ms,tiempo_max_repostar_ms>() ) );
      gasolinera->sale_coche_diesel( ncd );

      this_thread::sleep_for(chrono::milliseconds( aleatorio<tiempo_min_fuera_ms,tiempo_max_fuera_ms>() ) );
   }
}

/** -------------------------------------------------------------------------- **/
/// @brief función principal
/// @return int
///
int main()
{
   cout << "------------------------------------------------------------" << endl
        << "Práctica 2. Problemas adicionales. Problema de la gasolinera" << endl
        << "------------------------------------------------------------" << endl
        << flush ;

   // Crear el monitor
   MRef<Gasolinera> gasolinera = Create<Gasolinera>() ;

   // crear y lanzar las hebras
   thread hebras_coches_gaso[ num_coches[tipo_coche::gasolina] ],
          hebras_coches_dies[ num_coches[tipo_coche::diesel] ] ;

   for( unsigned i = 0 ; i < num_coches[tipo_coche::gasolina] ; i++ )
      hebras_coches_gaso[i] = thread( funcion_hebra_gasolina, gasolinera, i );

   for( unsigned i = 0 ; i < num_coches[tipo_coche::diesel] ; i++ )
      hebras_coches_dies[i] = thread( funcion_hebra_diesel, gasolinera, i );

   // esperar a que terminen las hebras
   for( unsigned i = 0 ; i < num_coches[tipo_coche::gasolina] ; i++ )
      hebras_coches_gaso[i].join();
   for( unsigned i = 0 ; i < num_coches[tipo_coche::diesel] ; i++ )
      hebras_coches_dies[i].join();
}
