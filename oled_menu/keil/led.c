#include "led.h"
void led_on(){
	DL_GPIO_setPins(LED_PORT,LED_LIGHT_PIN);
}
void led_off(){
	DL_GPIO_clearPins(LED_PORT,LED_LIGHT_PIN);
}

