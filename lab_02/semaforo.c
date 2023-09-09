#ifndef ___AVR_ATtiny4313__
#define __AVR_ATtiny4313__
#endif

#include <avr/io.h>
#include <avr/interrupt.h>

typedef enum estados {
    paso_vehiculo,
    parpadear_vehiculo,
    detener_vehiculo,
    paso_peaton,
    parpadear_peaton,
    detener_peaton
} estados;

unsigned int volatile ciclos = 0;
unsigned int volatile boton = 0;

ISR (TIMER0_OVF_vect) {
  ciclos++;
}

ISR (INT0_vect) {
    boton = 1;
}

void retardo(float segundos) {
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
    // configurar interrupciones externas
    GIMSK |= (1<<INT0); //habilita interrupcion por INT0
    MCUCR |= (1<<ISC00)|(1<<ISC01); // Configura el tipo de interrupcion, en esta caso habilita en flanco positivo
    DDRA |= (1<<PA0)|(1<<PA1); // Configura el A0 y A1 como output
    DDRB |= (1<<PB0)|(1<<PB1); // Configra el B0 y B1 como output
    DDRD |= (0<<PD2); // Habilita el boton
    // configurar registros de timer0
    TCCR0A=0x00;  // modo normal, cuenta ascendente
    TCCR0B=0x00;  // seleccion de reloj: ninguna, detenido
    TIMSK = (1<<TOIE0); // habilitar interrupcion por overflow
}

int main() {
    setup();
    estados estado = paso_vehiculo;
    unsigned int esperar = 1;
    while (1)
    {
    switch (estado) {
        case paso_vehiculo:
            PORTA = ((0<<PA0)|(1<<PA1));
            if (esperar) {retardo(10); esperar = 0;}
            if (boton) {estado = parpadear_vehiculo; boton = 0;}
        break;

        case parpadear_vehiculo:
            PORTA = ((0<<PA0)|(1<<PA1));
            retardo(0.5);
            PORTA = ((0<<PA0)|(0<<PA1));
            retardo(0.5);   
            PORTA = ((0<<PA0)|(1<<PA1));
            retardo(0.5);   
            PORTA = ((0<<PA0)|(0<<PA1));
            retardo(0.5);
            PORTA = ((0<<PA0)|(1<<PA1));
            retardo(0.5);   
            PORTA = ((0<<PA0)|(0<<PA1));
            retardo(0.5);        
            estado = detener_vehiculo;
        break;

        case detener_vehiculo:
            PORTA = ((1<<PA0)|(0<<PA1));
            retardo(1);  
            estado = paso_peaton;
        break;   

        case paso_peaton:
            PORTB = ((0<<PB0)|(1<<PB1));
            retardo(10);   
            estado = parpadear_peaton;
            
        break;

        case parpadear_peaton:
            PORTB = ((0<<PB0)|(1<<PB1));
            retardo(0.5);
            PORTB = ((0<<PB0)|(0<<PB1));
            retardo(0.5); 
            PORTB = ((0<<PB0)|(1<<PB1));
            retardo(0.5);
            PORTB = ((0<<PB0)|(0<<PB1));
            retardo(0.5); 
            PORTB = ((0<<PB0)|(1<<PB1));
            retardo(0.5);
            PORTB = ((0<<PB0)|(0<<PB1));
            retardo(0.5);     
            estado = detener_peaton;
            
        break;

        case detener_peaton:
            PORTB = ((1<<PB0)|(0<<PB1));
            retardo(1);   
            estado = paso_vehiculo;
            esperar = 1;
        break;
    }
    }
}