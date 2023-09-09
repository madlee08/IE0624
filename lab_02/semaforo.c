#ifndef ___AVR_ATtiny4313__
#define __AVR_ATtiny4313__
#endif

#include <avr/io.h>
#include <avr/interrupt.h>

typedef enum estados {
    paso_vehiculo,
    esperar,
    parpadear_vehiculo,
    detener_vehiculo,
    paso_peaton,
    parpadear_peaton,
    detener_peaton
} estados;

unsigned int volatile ciclos = 0;

ISR (TIMER0_OVF_vect) {
  ciclos++;
}

void retardo(unsigned int segundos) {
    ciclos=0;

    TCNT0=0; // empezar el timer0 en cero
    TCCR0B = (1<<CS00)|(1<<CS02); // empezar a contar con prescale 1024

    // la funcion se encicla aquí. el ISR por TIMER0_OVF_vect
    // ejecuta ciclos++ y eventualmente sale del while. se
    // requiere alrededor de 62 ciclos para registrar un segundo.
    while (ciclos<62*segundos) {}

    TCCR0B = (0<<CS00)|(0<<CS02); // detener timer0
}

void setup() {
    // habilitar interrupciones (global)
    sei(); 

    // configurar registros de timer0
    TCCR0A=0x00;  // modo normal, cuenta ascendente
    TCCR0B=0x00;  // seleccion de reloj: ninguna, detenido
    TIMSK = (1<<TOIE0); // habilitar interrupcion por overflow
}

void main() {
    setup();
    estados estado = paso_vehiculo;

    switch (estado) {
        case paso_vehiculo:
            if (boton){
                if (diez_segundos){
                    estado = parpadear_vehiculo;
                } else estado = esperar;
            } break;

        case esperar:
            if (tiempo_restante){
                estado = parpadear_vehiculo;
            } break;

        case parpadear_vehiculo:
            if (tres_segundos){
                estado = detener_vehiculo;
            }

        case detener_vehiculo:
            if(un_segundo){
                estado = paso_peaton;
            }

        case paso_peaton:
            if (diez_segundos) {
                estado = parpadear_peaton;
            }
        break;

        case parpadear_peaton:
            if (tres_segundos) {
                estado = detener_peaton;
            }
        break;

        case detener_peaton:
            if (un_segundo) {
                estado = paso_vehiculo;
            }
        break;
    }
}