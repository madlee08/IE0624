#include <pic14/pic12f675.h>

// registro de configuración. créditos a Raphael Neider:
// https://sourceforge.net/p/sdcc/discussion/1865/thread/06684379/
// los detalles de los configwords se pueden ver en el propio
// header pic12f675.h.
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;

__code uint16_t __at(0x2007) __configword =
	_CPD_OFF & _CP_OFF & _BODEN_ON & _MCLRE_ON &
	_PWRTE_OFF & _WDT_OFF & _INTRC_OSC_NOCLKOUT;

// codigo del ejemplo hola PIC proporcionado por el profesor
void delay(unsigned int tiempo)
{
	unsigned int i;
	unsigned int j;

	for(i=0; i<tiempo; i++)
	  for(j=0; j<1275; j++);
}

void main(void)
{
    TRISIO = 0b00100000; // setup del I/O (GP5 como entrada)
	GPIO = 0x00;         // valor inicial de GPIO

    unsigned int time = 10;
	unsigned int dice_value = 1;

    while (1) {
		if (GP5 != 1) {
			dice_value = dice_value + 1;

			if (dice_value > 6) {
				dice_value = 1;
			}
		}

		GPIO = dice_value;
		delay(time);
    }
}