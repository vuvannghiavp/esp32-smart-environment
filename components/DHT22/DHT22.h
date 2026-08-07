/* 

	DHT22 temperature sensor driver

*/

#ifndef DHT22_H_  
#define DHT22_H_

#define DHT_OK 0
#define DHT_CHECKSUM_ERROR -1
#define DHT_TIMEOUT_ERROR -2

#define RELAY1_GPIO        GPIO_NUM_25
#define RELAY2_GPIO        GPIO_NUM_26
#define PWM_GPIO           GPIO_NUM_27

#define IN1_GPIO   			GPIO_NUM_18
#define IN2_GPIO   			GPIO_NUM_19

#define SDA_GPIO   			GPIO_NUM_21
#define SCL_GPIO   			GPIO_NUM_22

#define PWM_TIMER          LEDC_TIMER_0
#define PWM_MODE           LEDC_LOW_SPEED_MODE
#define PWM_CHANNEL        LEDC_CHANNEL_0
#define PWM_RESOLUTION     LEDC_TIMER_10_BIT   /* 0..1023 */
#define PWM_FREQ_HZ        5000
// == function prototypes =======================================

void 	setDHTgpio(int gpio);
void 	errorHandler(int response);
int 	readDHT();
float 	getHumidity();
float 	getTemperature();
// PWM for Motor
void 	hardware_init();
#endif
