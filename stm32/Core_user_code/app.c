#include "app.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

extern ADC_HandleTypeDef  hadc1;
extern TIM_HandleTypeDef  htim3;
extern UART_HandleTypeDef huart1;

/* ---------- État du système ---------- */
typedef enum { FAN_OFF = 0, FAN_ON = 1, FAN_AUTO = 2 } FanMode;

static int      temp_x10   = 0;      /* température * 10 (ex. 253 = 25,3 °C) */
static uint16_t gas_raw    = 0;
static int      pir        = 0;
static int      light      = 0;
static int      fan        = 0;
static FanMode  fan_mode   = FAN_AUTO;
static int      door_open  = 0;
static int      alarm      = 0;
static int      mute       = 0;

/* ---------- Réception UART (ligne terminée par '\n') ---------- */
#define RX_LINE_MAX 64
static uint8_t  rx_byte;
static char     rx_buf[RX_LINE_MAX];
static uint8_t  rx_idx = 0;
static char     cmd_line[RX_LINE_MAX];
static volatile uint8_t cmd_ready = 0;

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        if (rx_byte == '\n') {
            if (!cmd_ready) {                 /* n'écrase pas une commande non traitée */
                rx_buf[rx_idx] = '\0';
                memcpy(cmd_line, rx_buf, (size_t)rx_idx + 1);
                cmd_ready = 1;
            }
            rx_idx = 0;
        } else if (rx_byte != '\r') {
            if (rx_idx < RX_LINE_MAX - 1) rx_buf[rx_idx++] = (char)rx_byte;
            else rx_idx = 0;                  /* ligne trop longue : on jette */
        }
        HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
    }
}

/* ---------- Utilitaires ---------- */
static void write_output(GPIO_TypeDef *port, uint16_t pin, int on)
{
    int level = on ? OUTPUT_ACTIVE_LEVEL : !OUTPUT_ACTIVE_LEVEL;
    HAL_GPIO_WritePin(port, pin, level ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static uint16_t adc_read(uint32_t channel)
{
    ADC_ChannelConfTypeDef cfg = {0};
    cfg.Channel      = channel;
    cfg.Rank         = ADC_REGULAR_RANK_1;
    cfg.SamplingTime = ADC_SAMPLETIME_55CYCLES_5;
    HAL_ADC_ConfigChannel(&hadc1, &cfg);
    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, 10);
    uint16_t v = (uint16_t)HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);
    return v;
}

static uint16_t adc_read_avg(uint32_t channel, int n)
{
    uint32_t sum = 0;
    for (int i = 0; i < n; i++) sum += adc_read(channel);
    return (uint16_t)(sum / (uint32_t)n);
}

static void apply_outputs(void)
{
    write_output(LIGHT_PORT, LIGHT_PIN, light);
    write_output(FAN_PORT, FAN_PIN, fan);
    write_output(BUZZER_PORT, BUZZER_PIN, alarm && !mute);
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1,
                          door_open ? SERVO_OPEN_US : SERVO_CLOSED_US);
}

/* ---------- Logique métier ---------- */
static void read_sensors(void)
{
    /* LM35 : 10 mV/°C -> mV = °C * 10 = température * 10 */
    uint16_t raw_t = adc_read_avg(ADC_CH_TEMP, 8);
    temp_x10 = (int)((uint32_t)raw_t * 3300u / 4095u);

    gas_raw = adc_read_avg(ADC_CH_GAS, 4);
    pir = (HAL_GPIO_ReadPin(PIR_PORT, PIR_PIN) == GPIO_PIN_SET) ? 1 : 0;
}

static void update_logic(void)
{
    /* Ventilateur */
    if (fan_mode == FAN_ON)       fan = 1;
    else if (fan_mode == FAN_OFF) fan = 0;
    else {                        /* AUTO avec hystérésis */
        if (temp_x10 >= TEMP_FAN_ON_X10)       fan = 1;
        else if (temp_x10 <= TEMP_FAN_OFF_X10) fan = 0;
    }

    /* Alarme gaz avec hystérésis */
    if (gas_raw >= GAS_ALARM_ON)       alarm = 1;
    else if (gas_raw <= GAS_ALARM_OFF) alarm = 0;

    apply_outputs();
}

static void process_command(const char *line)
{
    /* Format : "CMD;CLE=VALEUR" */
    if (strncmp(line, "CMD;", 4) != 0) return;
    const char *kv = line + 4;
    const char *eq = strchr(kv, '=');
    if (!eq) return;

    size_t klen = (size_t)(eq - kv);
    int v = atoi(eq + 1);

    if      (klen == 5 && strncmp(kv, "LIGHT", 5) == 0)   light = (v != 0);
    else if (klen == 7 && strncmp(kv, "FANMODE", 7) == 0) { if (v >= 0 && v <= 2) fan_mode = (FanMode)v; }
    else if (klen == 4 && strncmp(kv, "DOOR", 4) == 0)    door_open = (v != 0);
    else if (klen == 4 && strncmp(kv, "MUTE", 4) == 0)    mute = (v != 0);

    update_logic();
}

static void send_state(void)
{
    char buf[160];
    int n = snprintf(buf, sizeof buf,
        "STATE;T=%d.%d;GAS=%u;PIR=%d;LIGHT=%d;FAN=%d;FANMODE=%d;DOOR=%d;ALARM=%d;MUTE=%d\n",
        temp_x10 / 10, temp_x10 % 10, (unsigned)gas_raw, pir, light, fan,
        (int)fan_mode, door_open, alarm, mute);
    HAL_UART_Transmit(&huart1, (uint8_t *)buf, (uint16_t)n, 50);
}

/* ---------- API publique ---------- */
void app_init(void)
{
    HAL_ADCEx_Calibration_Start(&hadc1);
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
    apply_outputs();
    HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
}

void app_loop(void)
{
    static uint32_t last_sample = 0, last_tx = 0;
    uint32_t now = HAL_GetTick();

    if (cmd_ready) {
        process_command(cmd_line);
        cmd_ready = 0;
    }
    if (now - last_sample >= SAMPLE_PERIOD_MS) {
        last_sample = now;
        read_sensors();
        update_logic();
    }
    if (now - last_tx >= TX_PERIOD_MS) {
        last_tx = now;
        send_state();
    }
}
