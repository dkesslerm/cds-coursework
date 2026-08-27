// g++ -std=c++11 -pthread -o lectores_escritores lectores_escritores.cpp scd.cpp

#include <iostream>
#include <iomanip>
#include <cassert>
#include <random>
#include <thread>
#include <chrono>
#include "scd.h"

using namespace std ;
using namespace scd ;

const int num_lectores = 10;
const int num_escritores = 3;

//-------------------------------------------------------------------------

class LecEsc : public HoareMonitor {
  private:
    int n_lec;              // Número de lectores leyendo
    bool escrib;            // True si hay algún escritor escribiendo
    CondVar lectura;        // No hay escritores escribiendo, lectura posible
    CondVar escritura;      // No hay lectores ni escritores, escritura posible

  public:
    LecEsc();
    void ini_lectura();
    void fin_lectura();
    void ini_escritura();
    void fin_escritura();
};

//-------------------------------------------------------------------------

LecEsc::LecEsc(){
    n_lec = 0;
    escrib = false;
    lectura = newCondVar();
    escritura = newCondVar();
}

void LecEsc::ini_lectura(){
    if (escrib) {
        lectura.wait();
    }
    n_lec++;
    lectura.signal();
}

void LecEsc::fin_lectura(){
    n_lec--;
    if (n_lec == 0) {
        escritura.signal();
    }
}

void LecEsc::ini_escritura(){
    if (n_lec > 0 || escrib) {
        escritura.wait();
    }
    escrib = true;
}

void LecEsc::fin_escritura(){
    escrib = false;
    if (!lectura.empty()) {
        lectura.signal();
    } else {
        escritura.signal();
    }
}

void espera(bool escritor){
    if (escritor) {
       this_thread::sleep_for(chrono::milliseconds(aleatorio<1000,10000>()));
    } else {
       this_thread::sleep_for(chrono::milliseconds(aleatorio<100,1000>()));
    }
}

//-------------------------------------------------------------------------

void funcion_hebra_lectora(MRef<LecEsc> monitor, int id){
    while(true){
        monitor->ini_lectura();
        cout << "Lector nº " << id << " entra a leer" << endl;
        espera(false);
        cout << "\tLector nº " << id << " termina de leer" << endl;
        monitor->fin_lectura();
        this_thread::sleep_for(chrono::milliseconds(aleatorio<200,2000>()));
    }
}

void funcion_hebra_escritora(MRef<LecEsc> monitor, int id){
    while(true){
        monitor->ini_escritura();
        cout << "Escritor nº " << id << " entra a escribir" << endl;
        espera(true);
        cout << "Escritor nº " << id << " sale de escribir" << endl;
        monitor->fin_escritura();
        this_thread::sleep_for(chrono::milliseconds(aleatorio<200,2000>()));
    }
}

// --------------------------------------------------

int main(int argc, char** argv){

    thread lectores[num_lectores], escritores[num_escritores];
    MRef<LecEsc> monitor = Create<LecEsc>();

    for(int i = 0; i < num_lectores; i++)
        lectores[i] = thread(funcion_hebra_lectora, monitor, i);
    for(int i = 0; i < num_escritores; i++)
        escritores[i] = thread(funcion_hebra_escritora, monitor, i);


    for(int i = 0; i < num_lectores; i++)
        lectores[i].join();
    for(int i = 0; i < num_escritores; i++)
        escritores[i].join();
    return 0;
}
