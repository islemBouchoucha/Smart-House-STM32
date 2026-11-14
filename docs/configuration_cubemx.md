# Configuration STM32CubeMX (STM32F103C8T6)

1. **Nouveau projet** → carte/MCU `STM32F103C8Tx`.
2. **SYS** : Debug = Serial Wire.
3. **RCC** : High Speed Clock = Crystal/Ceramic Resonator. **Clock Configuration** : HCLK = 72 MHz.
4. **ADC1** : IN0 (PA0) et IN1 (PA1) activés. Mode indépendant, pas de scan, pas de mode continu, déclenchement logiciel, 1 conversion.
5. **TIM3** : Channel 1 = PWM Generation CH1 (PA6). Prescaler = `71`, Counter Period = `19999` (soit 1 µs par tick, période de 20 ms).
6. **USART1** : Asynchronous, 115200 bauds, 8 bits, pas de parité, 1 stop (PA9 TX / PA10 RX). Onglet NVIC : activer **USART1 global interrupt**.
7. **GPIO** : PB0, PB1, PB5 en `GPIO_Output` (niveau initial bas) ; PA4 en `GPIO_Input` avec pull-down.
8. Générer le code, puis copier `app.h` et `app.c` dans `Core/Inc` et `Core/Src`.
9. Dans `main.c` :

```c
/* USER CODE BEGIN Includes */
#include "app.h"
/* USER CODE END Includes */

/* après MX_USART1_UART_Init(); */
/* USER CODE BEGIN 2 */
app_init();
/* USER CODE END 2 */

while (1)
{
    /* USER CODE BEGIN 3 */
    app_loop();
    /* USER CODE END 3 */
}
```
