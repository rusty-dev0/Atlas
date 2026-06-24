#include <stdio.h>

#include "board.h"
#include "motor.h"

#define IDONTHAVETHEREALTHINGYET
#define FIXEDROTATIONSPEED 0.75f

void setup() {
    stdio_init_all();

    uart_init(UART_ID, BAUD_RATE);
    
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    
    for(int i = 0; i < 2; i++) {
        gpio_put(LED_PIN, 1);
        sleep_ms(100);
        gpio_put(LED_PIN, 0);
        sleep_ms(100);
    }
}

// TODO: For rotation, convert the speed to a degree and use encoders to determine when the rotation is complete.
// Controls the robot's motion based on the recieved command
// @author Satej Gandre
// @param byte the command byte
// @param m1 pointer to the first motor struct
// @param m2 pointer to the second motor struct
// @param old_cmd the previous command byte
void interpret_cmd_blocking(char byte, struct motor *m1, struct motor *m2, char old_byte) {
    char upper = (byte & 0xF0) >> 4;
    char cmd = byte & 0x0F;
    float angle = 0.0f;
    float speed = (float)upper / 0x0F;
    switch (cmd) {
        case 0x0A: // forward
            set_motor_speed(m1, speed);
            set_motor_speed(m2, -speed);
            break;
        case 0x0B: // backward
            set_motor_speed(m1, -speed);
            set_motor_speed(m2, speed);
            break;
        case 0x0C: // stop
            set_motor_speed(m1, 0.0f);
            set_motor_speed(m2, 0.0f);
            break;
        case 0x0D: // left
            angle = ((upper >> 2) + 1) * 45.0f;
            set_motor_speed(m1, -FIXEDROTATIONSPEED);
            set_motor_speed(m2, -FIXEDROTATIONSPEED);
            #ifdef IDONTHAVETHEREALTHINGYET
            printf("Turning left by %.2f degrees\n", angle);
            sleep_ms(10 * (int)angle);
            #endif
            interpret_cmd_blocking(0x000C, m1, m2, old_byte);
            sleep_ms(100);
            interpret_cmd_blocking(old_byte, m1, m2, old_byte);
            break;
        case 0x0E: // right
            angle = ((upper >> 2) + 1) * 45.0f;
            set_motor_speed(m1, FIXEDROTATIONSPEED);
            set_motor_speed(m2, FIXEDROTATIONSPEED);
            #ifdef IDONTHAVETHEREALTHINGYET
            printf("Turning right by %.2f degrees\n", angle);
            sleep_ms(10 * (int)angle);
            #endif
            interpret_cmd_blocking(0x000C, m1, m2, old_byte);
            sleep_ms(100);
            interpret_cmd_blocking(old_byte, m1, m2, old_byte);
            break;
        default:
            break;
    }
}

int main() {
    setup();

    sleep_ms(1000);

    struct motor m1, m2;
    motor_init(&m1, M1_PIN, M2_PIN);
    motor_init(&m2, M3_PIN, M4_PIN);

    set_motor_speed(&m1, 0.0f);
    set_motor_speed(&m2, 0.0f);

    sleep_ms(1000);

    char old_cmd = 0x000C;

    while (true) {
        char incoming_byte = 0;
        
        uart_read_blocking(UART_ID, &incoming_byte, 1);   
        
        printf("Received byte: 0x%02X\n", incoming_byte);
        
        interpret_cmd_blocking(incoming_byte, &m1, &m2, old_cmd);
        
        // to prevent infinite recursion
        if (((incoming_byte & 0x0F) != 0x0D) && ((incoming_byte & 0x0F) != 0x0E)) old_cmd = incoming_byte;
    }
}