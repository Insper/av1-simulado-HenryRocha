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
volatile Bool f_rtt_alarme = false;
volatile char rtt_pause = 1;
volatile char rtc_second = 0;

// Structs
typedef struct  {
	uint32_t year;
	uint32_t month;
	uint32_t day;
	uint32_t week;
	uint32_t hour;
	uint32_t minute;
	uint32_t seccond;
} calendar;

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
static void RTT_init(uint16_t pllPreScale, uint32_t IrqNPulses);

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

static void RTT_init(uint16_t pllPreScale, uint32_t IrqNPulses) {
	uint32_t ul_previous_time;

	/* Configure RTT for a 1 second tick interrupt */
	rtt_sel_source(RTT, false);
	rtt_init(RTT, pllPreScale);
	
	ul_previous_time = rtt_read_timer_value(RTT);
	while (ul_previous_time == rtt_read_timer_value(RTT));
	
	rtt_write_alarm_time(RTT, IrqNPulses+ul_previous_time);

	/* Enable RTT interrupt */
	NVIC_DisableIRQ(RTT_IRQn);
	NVIC_ClearPendingIRQ(RTT_IRQn);
	NVIC_SetPriority(RTT_IRQn, 0);
	NVIC_EnableIRQ(RTT_IRQn);
	rtt_enable_interrupt(RTT, RTT_MR_ALMIEN);
}

void RTT_Handler(void) {
	uint32_t ul_status;

	/* Get RTT status - ACK */
	ul_status = rtt_get_status(RTT);

	/* IRQ due to Time has changed */
	if ((ul_status & RTT_SR_RTTINC) == RTT_SR_RTTINC) {
	}

	/* IRQ due to Alarm */
	if ((ul_status & RTT_SR_ALMS) == RTT_SR_ALMS) {
		f_rtt_alarme = true;
	}
}

void RTC_init(Rtc *rtc, uint32_t id_rtc, calendar t, uint32_t irq_type) {
	/* Configura o PMC */
	pmc_enable_periph_clk(ID_RTC);

	/* Default RTC configuration, 24-hour mode */
	rtc_set_hour_mode(rtc, 0);

	/* Configura data e hora manualmente */
	rtc_set_date(rtc, t.year, t.month, t.day, t.week);
	rtc_set_time(rtc, t.hour, t.minute, t.seccond);

	/* Configure RTC interrupts */
	NVIC_DisableIRQ(id_rtc);
	NVIC_ClearPendingIRQ(id_rtc);
	NVIC_SetPriority(id_rtc, 0);
	NVIC_EnableIRQ(id_rtc);

	/* Ativa interrupcao via alarme */
	rtc_enable_interrupt(rtc,  irq_type);
}

void RTC_Handler(void) {
	uint32_t ul_status = rtc_get_status(RTC);

	/*
	*  Verifica por qual motivo entrou
	*  na interrupcao, se foi por segundo
	*  ou Alarm
	*/
	if ((ul_status & RTC_SR_SEC) == RTC_SR_SEC) {
		rtc_clear_status(RTC, RTC_SCCR_SECCLR);
	}
	
	/* Time or date alarm */
	if ((ul_status & RTC_SR_ALARM) == RTC_SR_ALARM) {
		
	}
	
	if ((ul_status & RTC_SR_SEC) == RTC_SR_SEC) {
		rtc_clear_status(RTC, RTC_SCCR_SECCLR);
		rtc_second = 1;
	}
	
	rtc_clear_status(RTC, RTC_SCCR_ACKCLR);
	rtc_clear_status(RTC, RTC_SCCR_TIMCLR);
	rtc_clear_status(RTC, RTC_SCCR_CALCLR);
	rtc_clear_status(RTC, RTC_SCCR_TDERRCLR);
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
	
	// Configure RTT...
	f_rtt_alarme = true;
	
	/** Configura RTC */
	calendar rtc_initial = {2020, 4, 6, 1, 18, 30, 1};
	RTC_init(RTC, ID_RTC, rtc_initial, RTC_IER_ALREN | RTC_IER_SECEN);
	rtc_set_hour_mode(RTC, 0);
	int counter = 0;
	char stringCounter[5][6] = {"*----", "**---", "***--", "****-", "*****"};

	// Variables
	int flash_LED1 = 0;
	int flash_LED2 = 0;
	int flash_LED3 = 0;
  
	// Print LED's frequencies...
	gfx_mono_draw_string("5", 0, 0, &sysfont);
	gfx_mono_draw_string("10", 30, 0, &sysfont);
	gfx_mono_draw_string("1", 70, 0, &sysfont);
	
	// Time stuff
	uint32_t hour;
	uint32_t minute;
	uint32_t second;
	char timeBuffer[512];
	
	while (1) {
		pmc_sleep(SAM_PM_SMODE_SLEEP_WFI);
		
		if (tc0_flag && flash_LED1 && !rtt_pause) {
			pin_toggle(LED1_PIO, LED1_IDX_MASK);
			tc0_flag = 0;
		}
		
		if (tc1_flag && flash_LED2 && !rtt_pause) {
			pin_toggle(LED2_PIO, LED2_IDX_MASK);
			tc1_flag = 0;
		}
		
		if (tc2_flag && flash_LED3 && !rtt_pause) {
			pin_toggle(LED3_PIO, LED3_IDX_MASK);
			tc2_flag = 0;
		}
		
		if (but1_flag) {
			flash_LED1 = !flash_LED1;			
			but1_flag = 0;
		}
		
		if (but2_flag) {
			flash_LED2 = !flash_LED2;			
			but2_flag = 0;
		}
		
		if (but3_flag) {
			flash_LED3 = !flash_LED3;			
			but3_flag = 0;
		}
		
		if (f_rtt_alarme) {
			// IRQ apos 5s -> 10 * 0.5
			uint16_t pllPreScale = (int) (((float) 32768) / 4.0);
			uint32_t irqRTTvalue = 19;
      
			// Reinicia RTT para gerar um novo IRQ
			RTT_init(pllPreScale, irqRTTvalue);         
      
			f_rtt_alarme = false;
			rtt_pause = !rtt_pause;
		}
		
		if (rtc_second) {
			gfx_mono_draw_string(stringCounter[counter], 0, 16, &sysfont);
				
			counter += 1;
			if (counter > 4) {
				counter = 0;
			}
			
			rtc_get_time(RTC, &hour, &minute, &second);
			sprintf(timeBuffer, "%2d:%2d:%2d", hour, minute, second);
			gfx_mono_draw_string(timeBuffer, 50, 16, &sysfont);
			
			rtc_second = 0;
		}
	}
}
