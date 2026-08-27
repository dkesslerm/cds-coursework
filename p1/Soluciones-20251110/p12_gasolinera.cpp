// g++ -std=c++11 -pthread -o gasolinera Soluciones-20251110/p12_gasolinera.cpp scd.cpp

/**
 * @file p1_gasolinera.cpp
 * @author Carlos Ureña (curena@ugr.es)
 * @version 0.1
 * @date 2022-11-03
 * @copyright Copyright (c) 2022
 *
 * @brief
 * Implementar un programa con semáforos (en un archivo llamado p1_gasolinera.cpp), con los siguientes requerimientos .... (ver pdf) .....
 *
 */

#include <iostream>
#include <thread>
#include <cmath>
#include "../scd.h" // incluye tipo 'Semaphore', entre otros

using namespace std ; // permite acortar la notación ('abc' en lugar de 'std::abc')
using namespace scd ; // permite usar 'Semaphore' en lugar de 'scd::Semaphore'


enum tipo_coche { gasolina, diesel };

// constantes y variables globales (compartidas)
constexpr unsigned
   num_coches_gaso = 6, // numero total de surtidores de gasolina
   num_coches_dies = 4, // número total de surtidores diesel
   num_surt_gaso   = 3,
   num_surt_dies   = 2 ;

constexpr unsigned
   tiempo_min_repostar_ms = 20,
   tiempo_max_repostar_ms = 200,
   tiempo_min_fuera_ms    = 20,
   tiempo_max_fuera_ms    = 200 ;

unsigned
   num_surt_en_uso = 0 ;  // número de surtidores en uso

Semaphore
   surt_gaso_libres = num_surt_gaso, // semáforo cuyo valor es el número de surtidores de gasolina libres
   surt_dies_libres = num_surt_dies, // idem , de gasoil
   mutex_num_surt   = 1,             // semáforo para acceder o imprimir el número de surtidores en uso
   mutex_cout       = 1 ;            // semáforo para exclusión mutua en los 'cout' (al imprimir)

/** -------------------------------------------------------------------------- **/
/// @brief fnución que simula un retardo aleatorio al repostar
/// @param tc tipo de coche
/// @param nc número de orden del coche dentro de su tipo
///
void repostar( const tipo_coche tc, const unsigned nc )
{
   const std::string str_tipo = (tc == tipo_coche::diesel) ? "diesel" : "gasolina" ;
   sem_wait( mutex_cout );
      cout << "coche " << str_tipo << " número " << tc << " comienza a repostar" << endl ;
   sem_signal( mutex_cout );
   this_thread::sleep_for(chrono::milliseconds( aleatorio<tiempo_min_repostar_ms,tiempo_max_repostar_ms>() ) );
   sem_wait( mutex_cout );
      cout << "coche " << str_tipo << " número " << tc << " termina de repostar" << endl ;
   sem_signal( mutex_cout );

}

/** -------------------------------------------------------------------------- **/
/// @brief función que ejecutan las hebras de coches de gasolina
/// @param ncg - número de orden del coche de gasolina
///
void funcion_hebra_gasolina( unsigned ncg )
{
   while( true )
   {

      sem_wait( surt_gaso_libres ); // esperar a que quede un surtidor de gasolina libre

         // incrementar e imprimir el valor de 'num_surt_en_uso'
         sem_wait( mutex_num_surt ); // espera a que ninguna hebra esté leyendo o escribiendo 'num_surt_en uso'
            num_surt_en_uso ++ ;
            sem_wait( mutex_cout );
               cout << "Coche gasolina número " << ncg << " comienza a repostar, hay " << num_surt_en_uso << " surtidores en uso." << endl ;
            sem_signal( mutex_cout );
         sem_signal( mutex_num_surt ); // señalar que ninguna hebra está ya leyendo o escribiendo 'num_surt_en uso'

         // repostar gasolina
         repostar( tipo_coche::gasolina, ncg );

         // decrementar e imprimir el valor de 'num_surt_en_uso'
         sem_wait( mutex_num_surt ); // espera a que ninguna hebra esté leyendo o escribiendo 'num_surt_en uso'
            num_surt_en_uso -- ;
            sem_wait( mutex_cout );
               cout << "Coche gasolina número " << ncg << " termina de repostar, hay " << num_surt_en_uso << " surtidores en uso." << endl ;
            sem_signal( mutex_cout );
         sem_signal( mutex_num_surt ); // señalar que ninguna hebra está ya leyendo o escribiendo 'num_surt_en uso'

      sem_signal( surt_gaso_libres ); // señalar que queda un surtidor de gasolina libre

      this_thread::sleep_for(chrono::milliseconds( aleatorio<tiempo_min_fuera_ms,tiempo_max_fuera_ms>() ) );
   }
}

/** -------------------------------------------------------------------------- **/
/// @brief función que ejecutan las hebras de coches diesel (== de gasoil)
/// @param ncd - número de orden del coche diesel
///
void funcion_hebra_diesel( unsigned ncd )
{
   while( true )
   {

      sem_wait( surt_dies_libres ); // esperar a que quede un surtidor de diesel libre

        // incrementar e imprimir el valor de 'num_surt_en_uso'
         sem_wait( mutex_num_surt ); // espera a que ninguna hebra esté leyendo o escribiendo 'num_surt_en uso'
            num_surt_en_uso ++ ;
            sem_wait( mutex_cout );
               cout << "Coche diesel número " << ncd << " comienza a repostar, hay " << num_surt_en_uso << " surtidores en uso." << endl ;
            sem_signal( mutex_cout );
         sem_signal( mutex_num_surt ); // señalar que ninguna hebra está ya leyendo o escribiendo 'num_surt_en uso'

         // repostar diesel
         repostar( tipo_coche::diesel, ncd );

         // decrementar e imprimir el valor de 'num_surt_en_uso'
         sem_wait( mutex_num_surt ); // espera a que ninguna hebra esté leyendo o escribiendo 'num_surt_en uso'
            num_surt_en_uso -- ;
            sem_wait( mutex_cout );
               cout << "Coche diesel número " << ncd << " termina de repostar, hay " << num_surt_en_uso << " surtidores en uso." << endl ;
            sem_signal( mutex_cout );
         sem_signal( mutex_num_surt ); // señalar que ninguna hebra está ya leyendo o escribiendo 'num_surt_en uso'

      sem_signal( surt_dies_libres ); // señalar que queda un surtidor de diesel libre

      this_thread::sleep_for(chrono::milliseconds( aleatorio<tiempo_min_fuera_ms,tiempo_max_fuera_ms>() ) );
   }
}

/** -------------------------------------------------------------------------- **/
/// @brief función principal
/// @return int
///
int main()
{
   cout << "-----------------------------------------------------------" << endl
        << "Práctica 1. Problemas adicionales. Problema A2 (gasolinera)" << endl
        << "-----------------------------------------------------------" << endl
        << flush ;


   // crear y lanzar las hebras
   thread hebras_coches_gaso[num_coches_gaso],
          hebras_coches_dies[num_coches_dies] ;

   for( unsigned i = 0 ; i < num_coches_gaso ; i++ )
      hebras_coches_gaso[i] = thread( funcion_hebra_gasolina, i );

   for( unsigned i = 0 ; i < num_coches_dies ; i++ )
      hebras_coches_dies[i] = thread( funcion_hebra_diesel, i );

   // esperar a que terminen las hebras
   for( unsigned i = 0 ; i < num_coches_gaso ; i++ )
      hebras_coches_gaso[i].join();
   for( unsigned i = 0 ; i < num_coches_dies ; i++ )
      hebras_coches_dies[i].join();



}
