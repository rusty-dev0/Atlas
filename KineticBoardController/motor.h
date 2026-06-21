#include "board.h"

struct motor {
    uint8_t m1;
    uint8_t m2;
    uint s1;
    uint s2;
};


// initialize a motor
// @author Satej Gandre
// @param m: pointer to the motor struct
// @param m1_pin: the pin for motor 1
// @param m2_pin: the pin for motor 2
void motor_init(struct motor *m, uint8_t m1_pin, uint8_t m2_pin) {
    m->m1 = m1_pin;
    m->m2 = m2_pin;

    gpio_set_function(m->m1, GPIO_FUNC_PWM);
    gpio_set_function(m->m2, GPIO_FUNC_PWM);

    m->s1 = pwm_gpio_to_slice_num(m->m1);
    m->s2 = pwm_gpio_to_slice_num(m->m2);

    pwm_set_wrap(m->s1, 65535);
    pwm_set_wrap(m->s2, 65535);

    pwm_set_enabled(m->s1, true);
    pwm_set_enabled(m->s2, true);
}


// set the speed of a motor
// @author Satej Gandre
// @param m: pointer to the motor struct
// @param speed: the speed to set (-1.0 to 1.0)
void set_motor_speed(struct motor *m, float speed) {
    if (speed < -1.0f) speed = -1.0;
    if (speed > 1.0f) speed = 1.0;

    if (speed > 0.0f) {
        pwm_set_gpio_level(m->m1, (uint16_t)(speed * 65535));
        pwm_set_gpio_level(m->m2, 0);
    } else if (speed < 0.0f) {
        pwm_set_gpio_level(m->m1, 0);
        pwm_set_gpio_level(m->m2, (uint16_t)(-speed * 65535));
    } else {
        pwm_set_gpio_level(m->m1, 0);
        pwm_set_gpio_level(m->m2, 0);
    }
}