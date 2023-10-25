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


static void spi_setup(void) {
	/* Enable the GPIO ports whose pins we are using */
	rcc_periph_clock_enable(RCC_GPIOF | RCC_GPIOC);

	gpio_mode_setup(GPIOF, GPIO_MODE_AF, GPIO_PUPD_PULLDOWN,
			GPIO7 | GPIO8 | GPIO9);
	gpio_set_af(GPIOF, GPIO_AF5, GPIO7 | GPIO8 | GPIO9);
	gpio_set_output_options(GPIOF, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ,
				GPIO7 | GPIO9);

	/* Chip select line */
	gpio_set(GPIOC, GPIO1);
	gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO1);

	rcc_periph_clock_enable(RCC_SPI5);

	uint32_t cr_tmp = SPI_CR1_BAUDRATE_FPCLK_DIV_8 |
		 SPI_CR1_MSTR |
		 SPI_CR1_SPE |
		 SPI_CR1_CPHA |
		 SPI_CR1_CPOL_CLK_TO_1_WHEN_IDLE;

	
	SPI_CR2(SPI5) |= SPI_CR2_SSOE;
	SPI_CR1(SPI5) = cr_tmp;
	
	/*
	 * These parameters are sort of random, clearly I need
	 * set something. Based on the app note I reset the 'baseline'
	 * values after 100 samples. But don't see a lot of change
	 * when I move the board around. Z doesn't move at all but the
	 * temperature reading is correct and the ID code returned is
	 * as expected so the SPI code at least is working.
	 */
	write_reg(0x20, 0xcf);  /* Normal mode */
	write_reg(0x21, 0x07);  /* standard filters */
	write_reg(0x23, 0xb0);  /* 250 dps */
}

static void adc_setup(void)
{
	rcc_periph_clock_enable(RCC_ADC1);
	gpio_mode_setup(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO0);

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

	baseline[0] = 0;
	baseline[1] = 0;
	baseline[2] = 0;

	gfx_init(lcd_draw_pixel, 240, 320);
	gfx_setTextColor(LCD_YELLOW, LCD_BLACK);
	gfx_setTextSize(2);

	int16_t abel;
	char txt[20];
	char txt2[20];
	while (1) {
		gfx_fillScreen(LCD_BLACK);
		gfx_setCursor(0, 36);
		read_xyz(vecs);
		uint16_t input_adc0 = read_adc_naiive(0);
		float volt = 3.0*input_adc0/4095.0;
		printf("Bateria: %f\t", volt);
		sprintf(txt2, "%.2f", volt);
		gfx_puts("Bateria: ");
		gfx_puts(txt2);
		gfx_puts("\n");

		gpio_toggle(LED_DISCO_GREEN_PORT, LED_DISCO_GREEN_PIN);
		for (int i = 0; i < 3; i++) {
			abel = vecs[i] - baseline[i];
			printf("%s%d\t", axes[i], abel);
			gfx_puts(axes[i]);
			sprintf(txt, "%d", abel);
			gfx_puts(txt);
			gfx_puts("\n");

			gpio_toggle(LED_DISCO_GREEN_PORT, LED_DISCO_GREEN_PIN);
		}
		printf("\n");
		gfx_puts("\r");

		baseline[0] = vecs[0];
		baseline[1] = vecs[1];
		baseline[2] = vecs[2];

		msleep(10);
		lcd_show_frame();
	}

	return 0;
}
