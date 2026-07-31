#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"

// TODO: remove this later as the board is confirmed to be working
#ifndef PIN_A
#define PIN_A 0
#endif

// TODO: find a better place to keep these so they can be shared across different files
#ifndef GPIO_HIGH
#define GPIO_HIGH 1
#endif

#ifndef GPIO_LOW
#define GPIO_LOW 0
#endif

#ifndef WRAP
#define WRAP 6250
#endif

struct pwm {
    uint slice_num;
    uint channel;
};

struct motor {
    uint in1;
    uint in2;
    struct pwm *speed;
};

enum motor_direction {
    MOTOR_CW,
    MOTOR_CCW,
    MOTOR_STOP,
    MOTOR_BRAKE
};

void gpio_test();

struct pwm *motor_pwm_init(uint pin);

void motor_pwm_set_speed(struct pwm *pwm, float speed);

void motor_pwm_deinit(struct pwm *pwm);

void motor_deinit(struct motor *m);

struct motor *motor_init(uint in1, uint in2, uint pwm_pin);

void motor_set_direction(struct motor *m, enum motor_direction dir);

void motor_spin(struct motor *m, float speed, enum motor_direction dir);

void motor_deinit(struct motor *m);