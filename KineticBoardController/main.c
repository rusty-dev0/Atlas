#include <stdio.h>

#include "board.h"
#include "motor.h"

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


int main() {
    setup();

    sleep_ms(1000);

    struct motor m;
    motor_init(&m, M1_PIN, M2_PIN);

    set_motor_speed(&m, 0.0f);

    sleep_ms(1000);

    while (true) {
        uint8_t incoming_byte = 0;
        
        uart_read_blocking(UART_ID, &incoming_byte, 1);   
        
        printf("Received byte: 0x%02X\n", incoming_byte);

        uint8_t upper = (incoming_byte & 0xF0) >> 4;
        float speed = (float)upper / 0x0F;

        if ((incoming_byte & 0x0F) == 0x0A) {

            set_motor_speed(&m, speed);  // Command: forward
        } 
        else if ((incoming_byte & 0x0F) == 0x0B) {
            set_motor_speed(&m, -speed);  // Command: backward
        }
        else if ((incoming_byte & 0x0F) == 0x0C) {
            set_motor_speed(&m, 0.0f);  // Command: stop
        }
    }
}