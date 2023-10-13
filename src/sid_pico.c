#include "pico/stdlib.h"
#include <stdio.h>
#include "hardware/pwm.h"
#include "hardware/gpio.h"
#include "hardware/uart.h"
#include "sid.h"
#include <time.h>

clock_t clock()
{
    return (clock_t) time_us_64() / 10000;
}

#define CS_DELAY 60000

void write_sid(uint8_t addr,uint8_t data) {

    clock_t startTime = clock();

	data &= 0xff;
	addr &= 0x1f;
	
    for(int i  = 5;i >= 0; i--) {
		gpio_put(i + ADDR_OFFSET, (addr >> i) & 1);
	}
    for(int i = 8;i >= 0; i--) {     
        gpio_put(i + DATA_OFFSET, (data >> i) & 1);
	}
    gpio_put(CS,0);
    busy_wait_us(CS_DELAY);	
    gpio_put(CS,1);
    busy_wait_us(CS_DELAY);	

    clock_t endTime = clock();

    double executionTime = (double)(endTime - startTime) / CLOCKS_PER_SEC;
    //printf("%.8f sec\n", executionTime);
	
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


}
void sid_start_clock() {
    
    gpio_set_function(CLK, GPIO_FUNC_PWM);
    
    uint slice_num = pwm_gpio_to_slice_num(CLK);

    pwm_set_clkdiv(slice_num, 64); // pwm clock should now be running at 1MHz
    pwm_set_wrap(slice_num, 1);
    pwm_set_chan_level(slice_num, PWM_CHAN_A, 1);
    pwm_set_enabled(slice_num, true);

}

void test() {
    printf("SID down \n");
    gpio_put(A4,1);
    gpio_put(A3,1);
    gpio_put(A2,0);
    gpio_put(A1,0);
    gpio_put(A0,0);
    
    gpio_put(D7,0);
    gpio_put(D6,0);
    gpio_put(D5,0);
    gpio_put(D4,0);
    gpio_put(D3,0);
    gpio_put(D2,0);
    gpio_put(D1,0);
    gpio_put(D0,0);
    
    gpio_put(CS,1);
    gpio_put(CS,0);
    busy_wait_us(CS_DELAY);
    gpio_put(CS,1);
    printf("SID up \n");

    gpio_put(D7,1);
    gpio_put(D6,1);
    gpio_put(D5,1);
    gpio_put(D4,1);
    gpio_put(D3,1);
    gpio_put(D2,1);
    gpio_put(D1,1);
    gpio_put(D0,1);
    
    gpio_put(CS,1);
    gpio_put(CS,0);
    busy_wait_us(CS_DELAY);
    gpio_put(CS,1);
    printf("SID end \n");

}
int main() {
    
    uint8_t tune[] = { 17,37,19,63,21,154,22,227,25,177,28,214,32,94,34,175 };

    stdio_init_all();

    init_pins();

    sid_start_clock();

    printf("Resetting SID\n");
    
    sid_reset();

    printf("Setting volume to max (15)\n");
    
 

    while(true) {
        for(int i=0; i < 32; i++) {
            write_sid(i,0);    
        }
        write_sid(24,15);
        write_sid(24,21);
        write_sid(5,9);
        write_sid(6,0);
        write_sid(1,48);
        write_sid(4,32);
        write_sid(4,33);
        sleep_ms(1000);
    }

}
