/************************************************************************/
/* includes                                                             */
/************************************************************************/

#include <asf.h>
#include <string.h>
#include "ili9341.h"
#include "lvgl.h"
#include "touch/touch.h"
LV_FONT_DECLARE(dseg70);
LV_FONT_DECLARE(dseg50);
LV_FONT_DECLARE(dseg20);


// global
static  lv_obj_t * labelBtn1;
static  lv_obj_t * labelMenu;
static  lv_obj_t * labelClk;
static  lv_obj_t * labelUp;
static  lv_obj_t * labelDown;

static lv_obj_t * label_PlayPause;
static lv_obj_t * label_Restart;
static lv_obj_t * label_PassarTelaDireita;
static lv_obj_t * labelTempo;


static lv_obj_t * labelFloor;
static lv_obj_t * labelFloor50;
static lv_obj_t * labelFloor20;

// filas
QueueHandle_t xQueueAnalogicData;
QueueHandle_t xQueueRealTemp;
QueueHandle_t xQueueRTC;
// semáforos
SemaphoreHandle_t xSemaphoreClock;
SemaphoreHandle_t xSemaphore;


typedef struct {
	uint32_t year;
	uint32_t month;
	uint32_t day;
	uint32_t week;
	uint32_t hour;
	uint32_t minute;
	uint32_t second;
} calendar;

uint32_t current_hour, current_min, current_sec;
uint32_t current_year, current_month, current_day, current_week;


/************************************************************************/
/* LCD / LVGL                                                           */
/************************************************************************/

#define LV_HOR_RES_MAX          (320)
#define LV_VER_RES_MAX          (240)


/*A static or global variable to store the buffers*/
static lv_disp_draw_buf_t disp_buf;

/*Static or global buffer(s). The second buffer is optional*/
static lv_color_t buf_1[LV_HOR_RES_MAX * LV_VER_RES_MAX];
static lv_disp_drv_t disp_drv;          /*A variable to hold the drivers. Must be static or global.*/
static lv_indev_drv_t indev_drv;

/************************************************************************/
/* RTOS                                                                 */
/************************************************************************/

#define TASK_LCD_STACK_SIZE                (1024*6/sizeof(portSTACK_TYPE))
#define TASK_LCD_STACK_PRIORITY            (tskIDLE_PRIORITY)


extern void vApplicationStackOverflowHook(xTaskHandle *pxTask,  signed char *pcTaskName);
extern void vApplicationIdleHook(void);
extern void vApplicationTickHook(void);
extern void vApplicationMallocFailedHook(void);
extern void xPortSysTickHandler(void);

extern void vApplicationStackOverflowHook(xTaskHandle *pxTask, signed char *pcTaskName) {
	printf("stack overflow %x %s\r\n", pxTask, (portCHAR *)pcTaskName);
	for (;;) {	}
}

extern void vApplicationIdleHook(void) { }

extern void vApplicationTickHook(void) { }

extern void vApplicationMallocFailedHook(void) {
	configASSERT( ( volatile void * ) NULL );
}



void RTC_init(Rtc *rtc, uint32_t id_rtc, calendar t, uint32_t irq_type);


/************************************************************************/
/* lvgl                                                                 */
/************************************************************************/
// void RTC_Handler(void) {
// 	uint32_t ul_status = rtc_get_status(RTC);
// 
// 	/* seccond tick */
// 	if ((ul_status & RTC_SR_SEC) == RTC_SR_SEC) {
// 		// o c?digo para irq de segundo vem aqui
// 		// libera o semáforo para atualizar o relógio
// 		BaseType_t xHigherPriorityTaskWoken = pdTRUE;
// 		xSemaphoreGiveFromISR(xSemaphoreClock, &xHigherPriorityTaskWoken);
// 	}
// 
// 	rtc_clear_status(RTC, RTC_SCCR_SECCLR);
// 	rtc_clear_status(RTC, RTC_SCCR_ALRCLR);
// 	rtc_clear_status(RTC, RTC_SCCR_ACKCLR);
// 	rtc_clear_status(RTC, RTC_SCCR_TIMCLR);
// 	rtc_clear_status(RTC, RTC_SCCR_CALCLR);
// 	rtc_clear_status(RTC, RTC_SCCR_TDERRCLR);
// }

void RTC_Handler(void) {
	uint32_t ul_status = rtc_get_status(RTC);


	
	/* Time or date alarm */
	if ((ul_status & RTC_SR_ALARM) == RTC_SR_ALARM) {
		
	}
	

	if ((ul_status & RTC_SR_SEC) == RTC_SR_SEC) {
		
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		xSemaphoreGiveFromISR(xSemaphore, &xHigherPriorityTaskWoken);
		//rtc_clear_status(RTC, RTC_SCCR_SECCLR);
	}
	
	rtc_clear_status(RTC, RTC_SCCR_SECCLR);
	rtc_clear_status(RTC, RTC_SCCR_ALRCLR);
	rtc_clear_status(RTC, RTC_SCCR_ACKCLR);
	rtc_clear_status(RTC, RTC_SCCR_TIMCLR);
	rtc_clear_status(RTC, RTC_SCCR_CALCLR);
	rtc_clear_status(RTC, RTC_SCCR_TDERRCLR);
}

static void event_handler(lv_event_t * e) {
	lv_event_code_t code = lv_event_get_code(e);

	if(code == LV_EVENT_CLICKED) {
		LV_LOG_USER("Clicked");
	}
	else if(code == LV_EVENT_VALUE_CHANGED) {
		LV_LOG_USER("Toggled");
	}
}

static void menu_handler(lv_event_t * e) {
	lv_event_code_t code = lv_event_get_code(e);

	if(code == LV_EVENT_CLICKED) {
		LV_LOG_USER("Clicked");
	}
	else if(code == LV_EVENT_VALUE_CHANGED) {
		LV_LOG_USER("Toggled");
	}
}

static void clk_handler(lv_event_t * e) {
	lv_event_code_t code = lv_event_get_code(e);

	if(code == LV_EVENT_CLICKED) {
		LV_LOG_USER("Clicked");
	}
	else if(code == LV_EVENT_VALUE_CHANGED) {
		LV_LOG_USER("Toggled");
	}
}

static void up_handler(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    char *c;
    int temp;
    if(code == LV_EVENT_CLICKED) {
	    c = lv_label_get_text(labelFloor50);
	    temp = atoi(c);
	    lv_label_set_text_fmt(labelFloor50, "%02d", temp + 1);
    }
}

static void down_handler(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    char *c;
    int temp;
    if(code == LV_EVENT_CLICKED) {
	    c = lv_label_get_text(labelFloor50);
	    temp = atoi(c);
	    lv_label_set_text_fmt(labelFloor50, "%02d", temp + 1);
    }
}

void lv_ex_btn_1(void) {
	lv_obj_t * label;

	lv_obj_t * btn1 = lv_btn_create(lv_scr_act());
	lv_obj_add_event_cb(btn1, event_handler, LV_EVENT_ALL, NULL);
	lv_obj_align(btn1, LV_ALIGN_CENTER, 0, -40);

	label = lv_label_create(btn1);
	lv_label_set_text(label, "Corsi");
	lv_obj_center(label);

	lv_obj_t * btn2 = lv_btn_create(lv_scr_act());
	lv_obj_add_event_cb(btn2, event_handler, LV_EVENT_ALL, NULL);
	lv_obj_align(btn2, LV_ALIGN_CENTER, 0, 40);
	lv_obj_add_flag(btn2, LV_OBJ_FLAG_CHECKABLE);
	lv_obj_set_height(btn2, LV_SIZE_CONTENT);

	label = lv_label_create(btn2);
	lv_label_set_text(label, "Toggle");
	lv_obj_center(label);
}

void lv_termostato(void) {
	static lv_style_t style;
	lv_style_init(&style);
	lv_style_set_bg_color(&style, lv_color_black());
	
	// Play/Pause
	lv_obj_t * PlayPause = lv_btn_create(lv_scr_act());
	lv_obj_add_event_cb(PlayPause, event_handler, LV_EVENT_ALL, NULL);
	lv_obj_align(PlayPause, LV_ALIGN_BOTTOM_MID, -8, -20);
	lv_obj_add_style(PlayPause, &style, 0);

	label_PlayPause = lv_label_create(PlayPause);
	lv_label_set_text(label_PlayPause, LV_SYMBOL_PLAY LV_SYMBOL_PAUSE);
	lv_obj_center(label_PlayPause);
	
	// Restart
	lv_obj_t * Restart = lv_btn_create(lv_scr_act());
	lv_obj_add_event_cb(Restart, event_handler, LV_EVENT_ALL, NULL);
	lv_obj_align_to(Restart, PlayPause, LV_ALIGN_OUT_RIGHT_MID, 0, -9);
	lv_obj_add_style(Restart, &style, 0);

	label_Restart = lv_label_create(Restart);
	lv_label_set_text(label_Restart, LV_SYMBOL_REFRESH);
	lv_obj_center(label_Restart);
	
	
	// Passar tela - direita
	lv_obj_t * PassarDireita = lv_btn_create(lv_scr_act());
	lv_obj_add_event_cb(PassarDireita, event_handler, LV_EVENT_ALL, NULL);
	lv_obj_align(PassarDireita, LV_ALIGN_TOP_RIGHT, 0, 0);
	lv_obj_add_style(PassarDireita, &style, 0);

	label_PassarTelaDireita = lv_label_create(PassarDireita);
	lv_label_set_text(label_PassarTelaDireita, LV_SYMBOL_RIGHT);
	lv_obj_center(label_PassarTelaDireita);
	
	// Tempo
	labelTempo = lv_label_create(lv_scr_act());
	lv_obj_align(labelTempo, LV_ALIGN_TOP_MID, 0 , 0);
	lv_obj_set_style_text_font(labelTempo, &dseg70, LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(labelTempo, lv_color_white(), LV_STATE_DEFAULT);
// 	lv_label_set_text_fmt(labelTempo, "%02d", 23);
	
	
	// BOTÃO 1 - POWER
// 	lv_obj_t * btn1 = lv_btn_create(lv_scr_act());
// 	lv_obj_add_event_cb(btn1, event_handler, LV_EVENT_ALL, NULL);
// 	lv_obj_align(btn1, LV_ALIGN_BOTTOM_LEFT, 8, -20);
// 	lv_obj_add_style(btn1, &style, 0);
// 
// 	labelBtn1 = lv_label_create(btn1);
// 	lv_label_set_text(labelBtn1, "[  " LV_SYMBOL_POWER);
// 	lv_obj_center(labelBtn1);
// 	
// 	// BOTÃO MENU
// 	lv_obj_t * btnMenu = lv_btn_create(lv_scr_act());
// 	lv_obj_add_event_cb(btnMenu, menu_handler, LV_EVENT_ALL, NULL);
// 	lv_obj_align_to(btnMenu, btn1, LV_ALIGN_OUT_RIGHT_MID, 0, -10);
// 	lv_obj_add_style(btnMenu, &style, 0);
// 
// 	labelMenu = lv_label_create(btnMenu);
// 	lv_label_set_text(labelMenu, "|  M");
// 	lv_obj_center(labelMenu);
// 	
// 	// BOTÃO CLOCK
// 	lv_obj_t * btnClk = lv_btn_create(lv_scr_act());
// 	lv_obj_add_event_cb(btnClk, menu_handler, LV_EVENT_ALL, NULL);
// 	lv_obj_align_to(btnClk, btnMenu, LV_ALIGN_OUT_RIGHT_MID, 0, -10);
// 	lv_obj_add_style(btnClk, &style, 0);
// 
// 	labelClk = lv_label_create(btnClk);
// 	lv_label_set_text(labelClk, "|  " LV_SYMBOL_SETTINGS "  ]");
// 	lv_obj_center(labelClk);
// 	
// 	// BOTÃO UP
// 	lv_obj_t * btnuP = lv_btn_create(lv_scr_act());
// 	lv_obj_add_event_cb(btnuP, event_handler, LV_EVENT_ALL, NULL);
// 	lv_obj_align(btnuP, LV_ALIGN_BOTTOM_RIGHT, -68, -20);
// 	lv_obj_add_style(btnuP, &style, 0);
// 
// 	labelUp = lv_label_create(btnuP);
// 	lv_label_set_text(labelUp, "[  " LV_SYMBOL_UP);
// 	lv_obj_center(labelUp);
// 	
// 	// BOTÃO DOWN
// 	lv_obj_t * btnDown = lv_btn_create(lv_scr_act());
// 	lv_obj_add_event_cb(btnDown, menu_handler, LV_EVENT_ALL, NULL);
// 	lv_obj_align_to(btnDown, btnuP, LV_ALIGN_OUT_RIGHT_MID, 0, -10);
// 	lv_obj_add_style(btnDown, &style, 0);
// 
// 	labelDown = lv_label_create(btnDown);
// 	lv_label_set_text(labelDown, "|  " LV_SYMBOL_DOWN "  ]");
// 	lv_obj_center(labelDown);
// 	
// 	/////////
// 	labelFloor = lv_label_create(lv_scr_act());
// 	lv_obj_align(labelFloor, LV_ALIGN_LEFT_MID, 35 , -45);
// 	lv_obj_set_style_text_font(labelFloor, &dseg70, LV_STATE_DEFAULT);
// 	lv_obj_set_style_text_color(labelFloor, lv_color_white(), LV_STATE_DEFAULT);
// 	lv_label_set_text_fmt(labelFloor, "%02d", 23);
// 
// 	labelFloor50 = lv_label_create(lv_scr_act());
// 	lv_obj_align(labelFloor50, LV_ALIGN_RIGHT_MID, -10 , -35);
// 	lv_obj_set_style_text_font(labelFloor50, &dseg50, LV_STATE_DEFAULT);
// 	lv_obj_set_style_text_color(labelFloor50, lv_color_white(), LV_STATE_DEFAULT);
// 	lv_label_set_text_fmt(labelFloor50, "%02d", 22);
// 	
// 	labelFloor20 = lv_label_create(lv_scr_act());
// 	lv_obj_align(labelFloor20, LV_ALIGN_TOP_RIGHT, -10 , 5);
// 	lv_obj_set_style_text_font(labelFloor20, &dseg20, LV_STATE_DEFAULT);
// 	lv_obj_set_style_text_color(labelFloor20, lv_color_white(), LV_STATE_DEFAULT);
// 	lv_label_set_text_fmt(labelFloor20, "%02d:%02d", 17,46);
}
/************************************************************************/
/* TASKS                                                                */
/************************************************************************/

static void task_lcd(void *pvParameters) {
	int px, py;
	printf("cheguei\n");
	//lv_ex_btn_1();
	lv_termostato();

	for (;;)  {
		lv_tick_inc(50);
		lv_task_handler();
		vTaskDelay(50);
	}
}

static void task_RTC(void *pvParameters) {
	calendar rtc_initial = {2018, 3, 19, 12, 1, 1 ,1};
	RTC_init(RTC, ID_RTC, rtc_initial, RTC_IER_ALREN | RTC_IER_SECEN);
	uint32_t hour;
	uint32_t minute;
	uint32_t second;
	UNUSED(pvParameters);
	
	int x;
	char teste[100];
	printf("cheguei rtc\n");
	
	for (;;) {
// 		printf("1");
// 		if( xSemaphoreTake(xSemaphore, ( TickType_t ) 10 / portTICK_PERIOD_MS) == pdTRUE ) {
		rtc_get_time(RTC, &hour, &minute, &second);
		printf("1");
		lv_label_set_text_fmt(labelTempo, "%02d : %02d",hour,minute);
		delay_s(1);
// 		}

	}
}


/************************************************************************/
/* configs                                                              */
/************************************************************************/
static void configure_lcd(void) {
	/**LCD pin configure on SPI*/
	pio_configure_pin(LCD_SPI_MISO_PIO, LCD_SPI_MISO_FLAGS);  //
	pio_configure_pin(LCD_SPI_MOSI_PIO, LCD_SPI_MOSI_FLAGS);
	pio_configure_pin(LCD_SPI_SPCK_PIO, LCD_SPI_SPCK_FLAGS);
	pio_configure_pin(LCD_SPI_NPCS_PIO, LCD_SPI_NPCS_FLAGS);
	pio_configure_pin(LCD_SPI_RESET_PIO, LCD_SPI_RESET_FLAGS);
	pio_configure_pin(LCD_SPI_CDS_PIO, LCD_SPI_CDS_FLAGS);
	
	ili9341_init();
	ili9341_backlight_on();
}

static void configure_console(void) {
	const usart_serial_options_t uart_serial_options = {
		.baudrate = USART_SERIAL_EXAMPLE_BAUDRATE,
		.charlength = USART_SERIAL_CHAR_LENGTH,
		.paritytype = USART_SERIAL_PARITY,
		.stopbits = USART_SERIAL_STOP_BIT,
	};

	/* Configure console UART. */
	stdio_serial_init(CONSOLE_UART, &uart_serial_options);

	/* Specify that stdout should not be buffered. */
	setbuf(stdout, NULL);
}

void TC_init(Tc *TC, int ID_TC, int TC_CHANNEL, int freq) {
// 	uint32_t ul_div;
// 	uint32_t ul_tcclks;
// 	uint32_t ul_sysclk = sysclk_get_cpu_hz();
// 
// 	pmc_enable_periph_clk(ID_TC);
// 
// 	tc_find_mck_divisor(freq, ul_sysclk, &ul_div, &ul_tcclks, ul_sysclk);
// 	tc_init(TC, TC_CHANNEL, ul_tcclks | TC_CMR_CPCTRG);
// 	tc_write_rc(TC, TC_CHANNEL, (ul_sysclk / ul_div) / freq);
// 
// 	NVIC_SetPriority((IRQn_Type)ID_TC, 4);
// 	NVIC_EnableIRQ((IRQn_Type)ID_TC);
// 	tc_enable_interrupt(TC, TC_CHANNEL, TC_IER_CPCS);
}

void RTC_init(Rtc *rtc, uint32_t id_rtc, calendar t, uint32_t irq_type){
	/* Configura o PMC */
	pmc_enable_periph_clk(ID_RTC);

	/* Default RTC configuration, 24-hour mode */
	rtc_set_hour_mode(rtc, 0);

	/* Configura data e hora manualmente */
	rtc_set_date(rtc, t.year, t.month, t.day, t.week);
	rtc_set_time(rtc, t.hour, t.minute, t.second);

	/* Configure RTC interrupts */
	NVIC_DisableIRQ(id_rtc);
	NVIC_ClearPendingIRQ(id_rtc);
	NVIC_SetPriority(id_rtc, 4);
	NVIC_EnableIRQ(id_rtc);

	/* Ativa interrupcao via alarme */
	rtc_enable_interrupt(rtc, irq_type);
}

/************************************************************************/
/* port lvgl                                                            */
/************************************************************************/

void my_flush_cb(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p) {
	ili9341_set_top_left_limit(area->x1, area->y1);   ili9341_set_bottom_right_limit(area->x2, area->y2);
	ili9341_copy_pixels_to_screen(color_p,  (area->x2 + 1 - area->x1) * (area->y2 + 1 - area->y1));
	
	/* IMPORTANT!!!
	* Inform the graphics library that you are ready with the flushing*/
	lv_disp_flush_ready(disp_drv);
}

void my_input_read(lv_indev_drv_t * drv, lv_indev_data_t*data) {
	int px, py, pressed;
	
	if (readPoint(&px, &py))
		data->state = LV_INDEV_STATE_PRESSED;
	else
		data->state = LV_INDEV_STATE_RELEASED; 
	
	data->point.x = px;
	data->point.y = py;
}

void configure_lvgl(void) {
	lv_init();
	lv_disp_draw_buf_init(&disp_buf, buf_1, NULL, LV_HOR_RES_MAX * LV_VER_RES_MAX);
	
	lv_disp_drv_init(&disp_drv);            /*Basic initialization*/
	disp_drv.draw_buf = &disp_buf;          /*Set an initialized buffer*/
	disp_drv.flush_cb = my_flush_cb;        /*Set a flush callback to draw to the display*/
	disp_drv.hor_res = LV_HOR_RES_MAX;      /*Set the horizontal resolution in pixels*/
	disp_drv.ver_res = LV_VER_RES_MAX;      /*Set the vertical resolution in pixels*/

	lv_disp_t * disp;
	disp = lv_disp_drv_register(&disp_drv); /*Register the driver and save the created display objects*/
	
	/* Init input on LVGL */
	lv_indev_drv_init(&indev_drv);
	indev_drv.type = LV_INDEV_TYPE_POINTER;
	indev_drv.read_cb = my_input_read;
	lv_indev_t * my_indev = lv_indev_drv_register(&indev_drv);
}

/************************************************************************/
/* main                                                                 */
/************************************************************************/
int main(void) {
	/* board and sys init */
	board_init();
	sysclk_init();
	configure_console();

	/* LCd, touch and lvgl init*/
	configure_lcd();
	configure_touch();
	configure_lvgl();

	/* Create task to control oled */
	if (xTaskCreate(task_lcd, "LCD", TASK_LCD_STACK_SIZE, NULL, TASK_LCD_STACK_PRIORITY, NULL) != pdPASS) {
		printf("Failed to create lcd task\r\n");
	}
	
	if (xTaskCreate(task_RTC, "RTC", TASK_LCD_STACK_SIZE, NULL, TASK_LCD_STACK_PRIORITY, NULL) != pdPASS) {
		printf("Failed to create rtc task\r\n");
	}
	
	xQueueRTC = xQueueCreate(32, sizeof(uint32_t));
	if (xQueueRTC == NULL)
	printf("falha em criar a queue RTC\n");
	
	xSemaphoreClock = xSemaphoreCreateBinary();
	xSemaphore = xSemaphoreCreateBinary();
	
	/* Start the scheduler. */
	vTaskStartScheduler();

	while(1){ }
}