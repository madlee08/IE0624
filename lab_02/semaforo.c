// define para syntax highlighting de los registros
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
estados volatile estado = paso_vehiculo;

ISR(TIMER0_OVF_vect) {
  ciclos++;
}

ISR(INT0_vect) {
  if (estado == paso_vehiculo) boton = 1;
}

void retardo(float segundos) {
	ciclos = 0;
	TCNT0 = 0; // empezar el timer0 en cero
	TCCR0B = (1<<CS00)|(1<<CS02); // empezar a contar

	// la funcion se encicla aquí y eventualmente
	// sale del while por ISR(TIMER0_OVF_vect)
	while (ciclos<61.2745*segundos) {}

	TCCR0B = 0; // detener timer0
}

void setup() {
	// habilitar interrupciones (global)
	sei();

	// configurar interrupciones externas
	GIMSK = 1<<INT0; // habilita interrupcion por INT0 y ...
	MCUCR = (1<<ISC00)|(1<<ISC01); // ...configura tipo de interrupcion

	// confugurar direccion de datos
	DDRA = (1<<PA0)|(1<<PA1); // configura el A0 y A1 como output
	DDRB = (1<<PB0)|(1<<PB1); // configra el B0 y B1 como output
	DDRD = 0<<PD2;            // configura D2 como input

	// configurar registros de timer0
	TCCR0A = 0;  // modo normal, cuenta ascendente
	TCCR0B = 0;  // seleccion de reloj: ninguna, detenido
	TIMSK = 1<<TOIE0; // habilitar interrupcion por overflow
}

int main() {
	setup();
	unsigned int esperar = 1;

	while (1) {
		switch (estado) {
			case paso_vehiculo:
				PORTA = 1<<PA1;
				PORTB = 1<<PB0;

				if (esperar) {retardo(10); esperar = 0;}
				if (boton) {estado = parpadear_vehiculo; boton = 0;}
			break;

			case parpadear_vehiculo:
				PORTB = 1<<PB0;
				PORTA = 0<<PA1; retardo(0.5);
				PORTA = 1<<PA1; retardo(0.5);
				PORTA = 0<<PA1; retardo(0.5);
				PORTA = 1<<PA1; retardo(0.5);
				PORTA = 0<<PA1; retardo(0.5);
				PORTA = 1<<PA1; retardo(0.5);

				estado = detener_vehiculo;
			break;

			case detener_vehiculo:
				PORTA = 1<<PA0;
				PORTB = 1<<PB0;
				retardo(1);

				estado = paso_peaton;
			break;

			case paso_peaton:
				PORTA = 1<<PA0;
				PORTB = 1<<PB1;
				retardo(10);

				estado = parpadear_peaton;
			break;

			case parpadear_peaton:
				PORTA = 1<<PA0;
				PORTB = 0<<PB1; retardo(0.5);
				PORTB = 1<<PB1; retardo(0.5);
				PORTB = 0<<PB1; retardo(0.5);
				PORTB = 1<<PB1; retardo(0.5);
				PORTB = 0<<PB1; retardo(0.5);
				PORTB = 1<<PB1; retardo(0.5);

				estado = detener_peaton;
			break;

			case detener_peaton:
				PORTA = 1<<PA0;
				PORTB = 1<<PB0;
				retardo(1);

				esperar = 1;
				estado = paso_vehiculo;
			break;
		}
	}
}