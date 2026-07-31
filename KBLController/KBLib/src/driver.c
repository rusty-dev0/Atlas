#include <stdlib.h>
#include "driver.h"

void gpio_test() {
    stdio_init_all();

#ifdef PIN_A
    gpio_init(PIN_A);
    gpio_set_dir(PIN_A, GPIO_OUT);
    
    bool state = 0;

    while (true) {
        state = !state;
        gpio_put(PIN_A, state);
        sleep_ms(1000);
    }
#endif
}

struct pwm *motor_pwm_init(uint pin) {
    gpio_set_function(pin, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(pin);
    uint channel = pwm_gpio_to_channel(pin);
    pwm_set_enabled(slice_num, true);
    pwm_set_wrap(slice_num, WRAP);
    struct pwm *p = malloc(sizeof(*p));
    if (!p) return NULL;
    p->slice_num = slice_num;
    p->channel = channel;
    return p;
}

void motor_pwm_set_speed(struct pwm *pwm, float speed) {
    if (!pwm) return;
    if (speed < 0.0f) {
        speed = 0.0f;
    } else if (speed > 1.0f) {
        speed = 1.0f;
    }
    uint level = (uint)(speed * WRAP);
    pwm_set_chan_level(pwm->slice_num, pwm->channel, level);
}

void motor_pwm_deinit(struct pwm *pwm) {
    if (!pwm) return;
    pwm_set_enabled(pwm->slice_num, false);
    free(pwm);
}

struct motor *motor_init(uint in1, uint in2, uint pwm_pin) {
    gpio_init(in1);
    gpio_init(in2);
    gpio_set_dir(in1, GPIO_OUT);
    gpio_set_dir(in2, GPIO_OUT);
    struct pwm *speed = motor_pwm_init(pwm_pin);
    struct motor *m = malloc(sizeof(*m));
    if (!m || !speed) {
        if (speed) {
            motor_pwm_deinit(speed);
        }
        else if (m) {
            free(m);
        }
        return NULL;
    }
    m->in1 = in1;
    m->in2 = in2;
    m->speed = NULL;

    m->speed = speed;
    return m;
}

void motor_set_direction(struct motor *m, enum motor_direction dir) {
    if (!m) return;
    switch (dir) {
        case MOTOR_CW:
            gpio_put(m->in1, GPIO_HIGH);
            gpio_put(m->in2, GPIO_LOW);
            break;
        case MOTOR_CCW:
            gpio_put(m->in1, GPIO_LOW);
            gpio_put(m->in2, GPIO_HIGH);
            break;
        case MOTOR_STOP:
            gpio_put(m->in1, GPIO_LOW);
            gpio_put(m->in2, GPIO_LOW);
            break;
        case MOTOR_BRAKE:
            gpio_put(m->in1, GPIO_HIGH);
            gpio_put(m->in2, GPIO_HIGH);
            break;
    }
}

void motor_spin(struct motor *m, float speed, enum motor_direction dir) {
    if (!m) return;
    motor_set_direction(m, dir);
    motor_pwm_set_speed(m->speed, speed);
}

void motor_deinit(struct motor *m) {
    if (!m) return;
    motor_pwm_deinit(m->speed);
    gpio_deinit(m->in1);
    gpio_deinit(m->in2);
    free(m);
}