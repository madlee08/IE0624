/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2014 Karl Palsson <karlp@tweak.net.au>
 * Copyright (C) 2015 Piotr Esden-Tempski <piotr@esden.nety>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

// -------------------------------------------------------------------
// se agradece a las personas de libopencm3 que proporcionaron los
// ejemplos para desarrollar este laboratorio. créditos a Karl
// Palsson & Piotr Esden-Tempski por el ejemplo de adc-dac-printf.c
// y a Chuck McManis por el resto de ejemplos utilizados para este
// laboratorio (spi-mems.c, clock.c, console.c, lcd-spi.c, gfx.c,
// font-7x12.c ysdram.c).
// -------------------------------------------------------------------
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <math.h>
#include "clock.h"
#include "console.h"
#include "sdram.h"
#include "lcd-spi.h"
#include "gfx.h"
#include "spi-mems.h"
#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/usart.h>

#define LED_DISCO_GREEN_PORT GPIOG
#define LED_DISCO_GREEN_PIN GPIO13
#define LED_DISCO_RED_PIN GPIO14

#define USART_CONSOLE USART1

int _write(int file, char *ptr, int len)
{
	int i;

	if (file == STDOUT_FILENO || file == STDERR_FILENO) {
		for (i = 0; i < len; i++) {
			if (ptr[i] == '\n') {
				usart_send_blocking(USART_CONSOLE, '\r');
			}
			usart_send_blocking(USART_CONSOLE, ptr[i]);
		}
		return i;
	}
	errno = EIO;
	return -1;
}

static void adc_setup(void)
{
	rcc_periph_clock_enable(RCC_ADC1);
	gpio_mode_setup(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO0);
	gpio_mode_setup(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO2);

	adc_power_off(ADC1);
	adc_disable_scan_mode(ADC1);
	adc_set_sample_time_on_all_channels(ADC1, ADC_SMPR_SMP_3CYC);

	adc_power_on(ADC1);

}

static uint16_t read_adc_naiive(uint8_t channel)
{
	uint8_t channel_array[16];
	channel_array[0] = channel;
	adc_set_regular_sequence(ADC1, 1, channel_array);
	adc_start_conversion_regular(ADC1);
	while (!adc_eoc(ADC1));
	uint16_t reg16 = adc_read_regular(ADC1);
	return reg16;
}

char *axes[] = { "X: ", "Y: ", "Z: " };

int main(void)
{
	int16_t vecs[3];
	int16_t baseline[3];

	clock_setup();
	console_setup(115200);
	sdram_init();
	lcd_spi_init();
	adc_setup();
	spi_setup();

	/* green led for ticking */
	gpio_mode_setup(LED_DISCO_GREEN_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
			LED_DISCO_GREEN_PIN);
	gpio_mode_setup(LED_DISCO_GREEN_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
		LED_DISCO_RED_PIN);

	baseline[0] = 0;
	baseline[1] = 0;
	baseline[2] = 0;

	gfx_init(lcd_draw_pixel, 240, 320);
	gfx_setTextColor(LCD_YELLOW, LCD_BLACK);
	gfx_setTextSize(2);

	int16_t abel;
	char txt[20];
	char txt2[20];
	char *txt3;
	while (1) {
		gfx_fillScreen(LCD_BLACK);
		gfx_setCursor(0, 36);
		read_xyz(vecs);
		uint16_t input_adc0 = read_adc_naiive(0);
		uint16_t USART = read_adc_naiive(2);
		float volt = 9*input_adc0/4095.0;

		if (volt < 8) txt3 = "si";
		else txt3 = "no";

		if (volt > 8) gpio_clear(LED_DISCO_GREEN_PORT, LED_DISCO_RED_PIN);
		else gpio_toggle(LED_DISCO_GREEN_PORT, LED_DISCO_RED_PIN);

		if (USART >= 2048) printf("Bateria: %f\t", volt);
		if (USART >= 2048) printf("Bajo: %s\t", txt3);
		sprintf(txt2, "%.2f", volt);
		gfx_puts("Bateria: ");
		gfx_puts(txt2);
		gfx_puts("\n");
		gfx_puts("USART: ");

		if (USART >= 2048) gfx_puts("ON");
		else gfx_puts("OFF");

		gfx_puts("\n");
		if (USART >= 2048) gpio_toggle(LED_DISCO_GREEN_PORT, LED_DISCO_GREEN_PIN);
		else gpio_clear(LED_DISCO_GREEN_PORT, LED_DISCO_GREEN_PIN);
		for (int i = 0; i < 3; i++) {
			abel = vecs[i] - baseline[i];
			if (USART >= 2048) printf("%s%d\t", axes[i], abel);
			gfx_puts(axes[i]);
			sprintf(txt, "%d", abel);
			gfx_puts(txt);
			gfx_puts("\n");

		}
		if (USART >= 2048) printf("\n");

		gfx_puts("\r");

		baseline[0] = vecs[0];
		baseline[1] = vecs[1];
		baseline[2] = vecs[2];

		msleep(10);
		lcd_show_frame();
	}

	return 0;
}
