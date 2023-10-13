#include "pico/stdlib.h"
#include <stdio.h>
#include "hardware/pwm.h"
#include "hardware/gpio.h"
#include "hardware/uart.h"
#include "pico/multicore.h"
#include "sid.h"
#include <time.h>
#include "sid_pico.h"

#define STARTED_FLAG 999
#define CS_DELAY 1

void write_sid(uint8_t addr,uint8_t data) {

	data &= 0xff;
	addr &= 0x1f;
	
    for(int i  = 5;i >= 0; i--) {
		gpio_put(i + ADDR_OFFSET, (addr >> i) & 1);
	}
    for(int i = 8;i >= 0; i--) {     
        gpio_put(i + DATA_OFFSET, (data >> i) & 1);
	}
    gpio_put(CS,0);
    sleep_us(CS_DELAY);	
    gpio_put(CS,1);
    sleep_us(5);	
	
}

void init_pins() {

    gpio_init(CS);
    gpio_set_dir(CS, GPIO_OUT);

    gpio_init(RES);
    gpio_set_dir(RES, GPIO_OUT);

    gpio_init(RW);
    gpio_set_dir(RW, GPIO_OUT);

    gpio_init(A0);
    gpio_set_dir(A0, GPIO_OUT);
    gpio_init(A1);
    gpio_set_dir(A1, GPIO_OUT);
    gpio_init(A2);
    gpio_set_dir(A2, GPIO_OUT);
    gpio_init(A3);
    gpio_set_dir(A3, GPIO_OUT);
    gpio_init(A4);
    gpio_set_dir(A4, GPIO_OUT);
    gpio_init(D0);
    gpio_set_dir(D0, GPIO_OUT);
    gpio_init(D1);
    gpio_set_dir(D1, GPIO_OUT);
    gpio_init(D2);
    gpio_set_dir(D2, GPIO_OUT);
    gpio_init(D3);
    gpio_set_dir(D3, GPIO_OUT);
    gpio_init(D4);
    gpio_set_dir(D4, GPIO_OUT);
    gpio_init(D5);
    gpio_set_dir(D5, GPIO_OUT);
    gpio_init(D6);
    gpio_set_dir(D6, GPIO_OUT);
    gpio_init(D7);
    gpio_set_dir(D7, GPIO_OUT);

    
}
void clock_count(int count) {

    for(int i = 0; i < count; i++) {
        while(gpio_get(CLK) > 0);
        while(gpio_get(CLK) == 0);
    }

}
void sid_reset() {
    gpio_put(RES,0);
    sleep_ms(20);
    gpio_put(RES,1);
    for(int i=0; i < 32; i++) {
        write_sid(i,0);    
    }  


}
void sid_start_clock() {
    
    gpio_set_function(CLK, GPIO_FUNC_PWM);
    
    uint slice_num = pwm_gpio_to_slice_num(CLK);

    pwm_set_clkdiv(slice_num, 64); // pwm clock should now be running at 1MHz
    pwm_set_wrap(slice_num, 1);
    pwm_set_chan_level(slice_num, PWM_CHAN_A, 1);
    pwm_set_enabled(slice_num, true);

}

void test_beep() {
    
    write_sid(24,15);
  
    while(true) {

        write_sid(24,21);
        write_sid(5,9);
        write_sid(6,0);
        write_sid(1,48);
        write_sid(4,32);
        write_sid(4,33);
        sleep_ms(2000); 
    }
}
int main() {
    
    stdio_init_all();

    multicore_launch_core1(test_beep);

    init_pins();

    sid_start_clock();

    printf("Resetting SID\n");
    
    sid_reset();

    while(true) {
        printf("Hello\n");
        sleep_ms(1000);
    }
    

}
