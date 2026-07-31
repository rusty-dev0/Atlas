#include "driver.h"
#include "board.h"

int main() {
    struct motor *m1 = motor_init(AIN1, AIN2, APWM);
    struct motor *m2 = motor_init(BIN1, BIN2, BPWM);

    if (!m1 || !m2) {
        return 1;
    }

    // this results in forward movement because I swapped the polarity on the second motor
    motor_spin(m1, 0.5f, MOTOR_CW);
    motor_spin(m2, 0.5f, MOTOR_CW); 

    while (true) {
        tight_loop_contents();
    }

    motor_deinit(m1);
    motor_deinit(m2);

    return 0;
}