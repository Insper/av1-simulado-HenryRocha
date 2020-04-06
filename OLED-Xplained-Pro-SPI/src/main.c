#include <asf.h>

#include "gfx_mono_ug_2832hsweg04.h"
#include "gfx_mono_text.h"
#include "sysfont.h"

// LED PLACA
#define LED_PLACA_PIO_ID ID_PIOC
#define LED_PLACA_PIO PIOC
#define LED_PLACA_PIN 8
#define LED_PLACA_IDX_MASK (1 << LED_PLACA_PIN)

// LED1 OLED
#define LED1_PIO PIOA
#define LED1_PIO_ID ID_PIOA
#define LED1_IDX 0
#define LED1_IDX_MASK (1 << LED1_IDX)

// LED2 OLED
#define LED2_PIO PIOC
#define LED2_PIO_ID ID_PIOC
#define LED2_IDX 30
#define LED2_IDX_MASK (1 << LED2_IDX)

// LED3 OLED
#define LED3_PIO PIOB
#define LED3_PIO_ID ID_PIOB
#define LED3_IDX 2
#define LED3_IDX_MASK (1 << LED3_IDX)

// BUT1 OLED
#define BUT1_PIO PIOD
#define BUT1_PIO_ID 16
#define BUT1_PIO_IDX 28
#define BUT1_PIO_IDX_MASK (1u << BUT1_PIO_IDX)

// BUT2 OLED
#define BUT2_PIO PIOC
#define BUT2_PIO_ID ID_PIOC
#define BUT2_PIO_IDX 31
#define BUT2_PIO_IDX_MASK (1 << BUT2_PIO_IDX)

// BUT3 OLED
#define BUT3_PIO PIOA
#define BUT3_PIO_ID ID_PIOA
#define BUT3_PIO_IDX 19
#define BUT3_PIO_IDX_MASK (1 << BUT3_PIO_IDX)

// Global variables
volatile int but1_flag = 0;
volatile int but2_flag = 0;
volatile int but3_flag = 0;
volatile char tc0_flag = 0;
volatile char tc1_flag = 0;
volatile char tc2_flag = 0;

// Prototypes
void setup_io();
void pin_toggle(Pio *pio, uint32_t mask);
void but1_callback();
void but2_callback();
void but3_callback();
void TC_init(Tc * TC, int ID_TC, int TC_CHANNEL, int freq);
void TC0_Handler(void);
void TC1_Handler(void);
void TC2_Handler(void);

// Functions
void setup_io() {
	// LED PLACA
	pmc_enable_periph_clk(LED_PLACA_PIO_ID);
	pio_configure(LED_PLACA_PIO, PIO_OUTPUT_0, LED_PLACA_IDX_MASK, PIO_DEFAULT);
	pio_set(LED_PLACA_PIO, LED_PLACA_IDX_MASK);
	
	// LED1 OLED
	pmc_enable_periph_clk(LED1_PIO_ID);
	pio_configure(LED1_PIO, PIO_OUTPUT_0, LED1_IDX_MASK, PIO_DEFAULT);
	pio_clear(LED1_PIO, LED1_IDX_MASK);
	
	// LED2 OLED
	pmc_enable_periph_clk(LED2_PIO_ID);
	pio_configure(LED2_PIO, PIO_OUTPUT_0, LED2_IDX_MASK, PIO_DEFAULT);
	pio_set(LED2_PIO, LED2_IDX_MASK);
	
	// LED3 OLED
	pmc_enable_periph_clk(LED3_PIO_ID);
	pio_configure(LED3_PIO, PIO_OUTPUT_0, LED3_IDX_MASK, PIO_DEFAULT);
	pio_clear(LED3_PIO, LED3_IDX_MASK);
		
	// BUT1 OLED
	pmc_enable_periph_clk(BUT1_PIO_ID);
	pio_configure(BUT1_PIO, PIO_INPUT, BUT1_PIO_IDX_MASK, PIO_PULLUP);
	pio_handler_set(BUT1_PIO, BUT1_PIO_ID, BUT1_PIO_IDX_MASK, PIO_IT_RISE_EDGE, but1_callback);
	pio_enable_interrupt(BUT1_PIO, BUT1_PIO_IDX_MASK);
	NVIC_EnableIRQ(BUT1_PIO_ID);
	NVIC_SetPriority(BUT1_PIO_ID, 4);
	
	// BUT2 OLED
	pmc_enable_periph_clk(BUT2_PIO_ID);
	pio_configure(BUT2_PIO, PIO_INPUT, BUT2_PIO_IDX_MASK, PIO_PULLUP);
	pio_handler_set(BUT2_PIO, BUT2_PIO_ID, BUT2_PIO_IDX_MASK, PIO_IT_FALL_EDGE, but2_callback);
	pio_enable_interrupt(BUT2_PIO, BUT2_PIO_IDX_MASK);
	NVIC_EnableIRQ(BUT2_PIO_ID);
	NVIC_SetPriority(BUT2_PIO_ID, 4);
	
	// BUT3 OLED
	pmc_enable_periph_clk(BUT3_PIO_ID);
	pio_configure(BUT3_PIO, PIO_INPUT, BUT3_PIO_IDX_MASK, PIO_PULLUP);
	pio_handler_set(BUT3_PIO, BUT3_PIO_ID, BUT3_PIO_IDX_MASK, PIO_IT_RISE_EDGE, but3_callback);
	pio_enable_interrupt(BUT3_PIO, BUT3_PIO_IDX_MASK);
	NVIC_EnableIRQ(BUT3_PIO_ID);
	NVIC_SetPriority(BUT3_PIO_ID, 4);
}

void pin_toggle(Pio *pio, uint32_t mask) {
	if (pio_get_output_data_status(pio, mask)) {
		pio_clear(pio, mask);
	} else {
		pio_set(pio, mask);
	}
}

void but1_callback() {
	but1_flag = 1;
}

void but2_callback() {
	but2_flag = 1;
}

void but3_callback() {
	but3_flag = 1;
}

void TC_init(Tc *TC, int ID_TC, int TC_CHANNEL, int freq) {
	uint32_t ul_div;
	uint32_t ul_tcclks;
	uint32_t ul_sysclk = sysclk_get_cpu_hz();

	/* Configura o PMC */
	/* O TimerCounter � meio confuso
	o uC possui 3 TCs, cada TC possui 3 canais
	TC0 : ID_TC0, ID_TC1, ID_TC2
	TC1 : ID_TC3, ID_TC4, ID_TC5
	TC2 : ID_TC6, ID_TC7, ID_TC8
	*/
	pmc_enable_periph_clk(ID_TC);

	/** Configura o TC para operar em  4Mhz e interrup�c�o no RC compare */
	tc_find_mck_divisor(freq, ul_sysclk, &ul_div, &ul_tcclks, ul_sysclk);
	tc_init(TC, TC_CHANNEL, ul_tcclks | TC_CMR_CPCTRG);
	tc_write_rc(TC, TC_CHANNEL, (ul_sysclk / ul_div) / freq);

	/* Configura e ativa interrup�c�o no TC canal 0 */
	/* Interrup��o no C */
	NVIC_EnableIRQ((IRQn_Type) ID_TC);
	tc_enable_interrupt(TC, TC_CHANNEL, TC_IER_CPCS);

	/* Inicializa o canal dado do TC */
	tc_start(TC, TC_CHANNEL);
}

void TC0_Handler(void) {
	volatile uint32_t ul_dummy;

	/****************************************************************
	* Devemos indicar ao TC que a interrup��o foi satisfeita.
	******************************************************************/
	ul_dummy = tc_get_status(TC0, 0);

	/* Avoid compiler warning */
	UNUSED(ul_dummy);

	/** Muda o estado do LED */
	tc0_flag = 1;
}

void TC1_Handler(void) {
	volatile uint32_t ul_dummy;

	/****************************************************************
	* Devemos indicar ao TC que a interrup��o foi satisfeita.
	******************************************************************/
	ul_dummy = tc_get_status(TC0, 1);

	/* Avoid compiler warning */
	UNUSED(ul_dummy);

	/** Muda o estado do LED */
	tc1_flag = 1;
}

void TC2_Handler(void) {
	volatile uint32_t ul_dummy;

	/****************************************************************
	* Devemos indicar ao TC que a interrup��o foi satisfeita.
	******************************************************************/
	ul_dummy = tc_get_status(TC0, 2);

	/* Avoid compiler warning */
	UNUSED(ul_dummy);

	/** Muda o estado do LED */
	tc2_flag = 1;
}

// Main
int main (void) {
	// Init...
	board_init();
	sysclk_init();
	delay_init();
	gfx_mono_ssd1306_init();

	// Self created inits...
	setup_io();
	
	// Configure TC's...
	TC_init(TC0, ID_TC0, 0, 5);
	TC_init(TC0, ID_TC1, 1, 10);
	TC_init(TC0, ID_TC2, 2, 1);
	
	// Variables
	int flash_LED1 = 0;
	int flash_LED2 = 0;
	int flash_LED3 = 0;
  
	//gfx_mono_draw_filled_circle(20, 16, 16, GFX_PIXEL_SET, GFX_WHOLE);
	//gfx_mono_draw_string("mundo", 50,16, &sysfont);
	
	while (1) {
		if (tc0_flag && flash_LED1) {
			pin_toggle(LED1_PIO, LED1_IDX_MASK);
			tc0_flag = 0;
		}
		
		if (tc1_flag && flash_LED2) {
			pin_toggle(LED2_PIO, LED2_IDX_MASK);
			tc1_flag = 0;
		}
		
		if (tc2_flag && flash_LED3) {
			pin_toggle(LED3_PIO, LED3_IDX_MASK);
			tc2_flag = 0;
		}
		
		if (but1_flag) {
			if (flash_LED1 == 0) {
				flash_LED1 = 1;
			} else {
				flash_LED1 = 0;
			}
			
			but1_flag = 0;
		}
		
		if (but2_flag) {
			if (flash_LED2 == 0) {
				flash_LED2 = 1;
			} else {
				flash_LED2 = 0;
			}
			
			but2_flag = 0;
		}
		
		if (but3_flag) {
			if (flash_LED3 == 0) {
				flash_LED3 = 1;
			} else {
				flash_LED3 = 0;
			}
			
			but3_flag = 0;
		}
	}
}
