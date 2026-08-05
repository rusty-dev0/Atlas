#include "driver.h"
#include "board.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"

// encoder state shared between ISR and main
volatile int encoder_count = 0;
volatile bool last_state_a = false;
volatile bool last_state_b = false;
volatile bool encoder_initialized = false;

void update_encoder_count(uint gpio, uint32_t events) {
    bool state_a = gpio_get(AENC1);
    bool state_b = gpio_get(AENC2);

    if (!encoder_initialized) {
        last_state_a = state_a;
        last_state_b = state_b;
        encoder_initialized = true;
        return;
    }

    if (state_a != last_state_a || state_b != last_state_b) {
        if (state_a == state_b) {
            encoder_count++;
        } else {
            encoder_count--;
        }
    }

    last_state_a = state_a;
    last_state_b = state_b;
}

int main() {
    stdio_init_all();

    struct motor *m1 = motor_init(AIN1, AIN2, APWM);
    struct motor *m2 = motor_init(BIN1, BIN2, BPWM);

    // // set up encoder pins
    // gpio_init(AENC1);
    // gpio_init(AENC2);

    // gpio_set_dir(AENC1, GPIO_IN);
    // gpio_set_dir(AENC2, GPIO_IN);

    // gpio_pull_up(AENC1);
    // gpio_pull_up(AENC2);

    // // read and store initial encoder state before enabling IRQs
    // last_state_a = gpio_get(AENC1);
    // last_state_b = gpio_get(AENC2);
    // encoder_initialized = true;

    // gpio_set_irq_enabled_with_callback(AENC1, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, update_encoder_count);
    // gpio_set_irq_enabled_with_callback(AENC2, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, update_encoder_count);

    if (!m1 || !m2) {
        return 1;
    }

    // this results in forward movement because I swapped the polarity on the second motor
    motor_spin(m1, 0.5f, MOTOR_CW);
    motor_spin(m2, 0.5f, MOTOR_CW);

    

    while (true) {
        // printf("Encoder Count: %d\n", encoder_count);
        // sleep_ms(500);
        tight_loop_contents();
    }

    motor_deinit(m1);
    motor_deinit(m2);

    return 0;
}