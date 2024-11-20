#include <stdbool.h>
#include <stdint.h>

#include "nrf.h"
#include "nrf52840.h"
#include "nrf_delay.h"
#include "nrf_drv_uart.h"
#include "nrf_gpio.h"

#define SERVO_PIN 13      // Пин, к которому подключен SG90 (например, P0.13)
#define PWM_PERIOD 20000  // Период PWM в мкс (20 мс = 50 Гц)
#define MIN_PULSE 500     // Минимальная длительность импульса в мкс (0.5 мс)
#define MAX_PULSE 2500    // Максимальная длительность импульса в мкс (2.5 мс)

void pwm_init(void) {
    // Настраиваем GPIO как выход
    nrf_gpio_cfg_output(SERVO_PIN);

    // Включаем таймер
    NRF_TIMER2->MODE = TIMER_MODE_MODE_Timer;                                        // Режим таймера
    NRF_TIMER2->BITMODE = TIMER_BITMODE_BITMODE_32Bit << TIMER_BITMODE_BITMODE_Pos;  // 32-битный режим
    NRF_TIMER2->PRESCALER = 4;  // Предделитель (1 МГц, 1 тик = 1 мкс)

    // Настраиваем CC[0] для импульса и CC[1] для периода
    NRF_TIMER2->CC[1] = PWM_PERIOD;  // Устанавливаем период (20 мс)
    NRF_TIMER2->SHORTS = TIMER_SHORTS_COMPARE1_CLEAR_Enabled << TIMER_SHORTS_COMPARE1_CLEAR_Pos;

    // Включаем прерывания для управления GPIO
    NRF_TIMER2->INTENSET = TIMER_INTENSET_COMPARE0_Set << TIMER_INTENSET_COMPARE0_Pos;

    // Настраиваем NVIC
    NVIC_EnableIRQ(TIMER2_IRQn);

    // Запускаем таймер
    NRF_TIMER2->TASKS_START = 1;
}

void set_servo_angle(uint8_t angle) {
    if (angle > 180) angle = 180;  // Ограничиваем угол

    // Рассчитываем длительность импульса
    uint32_t pulse_width = MIN_PULSE + (angle * (MAX_PULSE - MIN_PULSE)) / 180;

    // Устанавливаем длительность импульса в CC[0]
    NRF_TIMER2->CC[0] = pulse_width;
}

void TIMER2_IRQHandler(void) {
    if (NRF_TIMER2->EVENTS_COMPARE[0]) {
        NRF_TIMER2->EVENTS_COMPARE[0] = 0;

        // Включаем сигнал на пине
        nrf_gpio_pin_set(SERVO_PIN);
    }

    if (NRF_TIMER2->EVENTS_COMPARE[1]) {
        NRF_TIMER2->EVENTS_COMPARE[1] = 0;

        // Отключаем сигнал на пине
        nrf_gpio_pin_clear(SERVO_PIN);
    }
}

int main(void) {
    pwm_init();

    while (1) {
        // Пример: двигаем сервопривод из 0° в 180° и обратно
        for (uint8_t angle = 0; angle <= 180; angle++) {
            set_servo_angle(angle);
            nrf_delay_ms(20);  // Задержка для плавного движения
        }

        for (uint8_t angle = 180; angle > 0; angle--) {
            set_servo_angle(angle);
            nrf_delay_ms(20);
        }
    }
}
