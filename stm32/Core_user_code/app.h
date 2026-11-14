#ifndef APP_H
#define APP_H

#include "main.h"

/* ---- Affectation des broches (STM32F103C8T6 "Blue Pill") -------------
 * PA0  ADC1_IN0   capteur de température LM35 (10 mV/°C)
 * PA1  ADC1_IN1   capteur de gaz MQ-2 (sortie analogique)
 * PA4  GPIO IN    capteur PIR (HC-SR501)
 * PA6  TIM3_CH1   servomoteur de la porte (PWM 50 Hz)
 * PA9  USART1_TX  vers ESP32 RX (GPIO16)
 * PA10 USART1_RX  depuis ESP32 TX (GPIO17)
 * PB0  GPIO OUT   lumière (module relais / LED)
 * PB1  GPIO OUT   ventilateur (module relais)
 * PB5  GPIO OUT   buzzer actif
 * Modifie les lignes ci-dessous si ton câblage est différent.
 * ---------------------------------------------------------------------- */
#define LIGHT_PORT   GPIOB
#define LIGHT_PIN    GPIO_PIN_0
#define FAN_PORT     GPIOB
#define FAN_PIN      GPIO_PIN_1
#define BUZZER_PORT  GPIOB
#define BUZZER_PIN   GPIO_PIN_5
#define PIR_PORT     GPIOA
#define PIR_PIN      GPIO_PIN_4

#define ADC_CH_TEMP  ADC_CHANNEL_0
#define ADC_CH_GAS   ADC_CHANNEL_1

/* Niveau actif des sorties (1 = actif à l'état haut, 0 = actif à l'état bas, ex. relais) */
#define OUTPUT_ACTIVE_LEVEL 1

/* Seuils */
#define TEMP_FAN_ON_X10   300   /* 30,0 °C : ventilateur ON en mode AUTO  */
#define TEMP_FAN_OFF_X10  280   /* 28,0 °C : ventilateur OFF (hystérésis) */
#define GAS_ALARM_ON      2000  /* valeur ADC brute (0-4095)              */
#define GAS_ALARM_OFF     1800

/* Servo porte : largeur d'impulsion en microsecondes */
#define SERVO_CLOSED_US   1000
#define SERVO_OPEN_US     2000

/* Périodes */
#define SAMPLE_PERIOD_MS  200
#define TX_PERIOD_MS      500

void app_init(void);
void app_loop(void);

#endif
