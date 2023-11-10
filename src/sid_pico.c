#include "pico/stdlib.h"
#include <stdio.h>
#include "hardware/pwm.h"
#include "hardware/gpio.h"
#include "hardware/uart.h"
#include "pico/multicore.h"
#include "sid.h"
#include <time.h>
#include "sid_pico.h"
#include "sid_server.h"
#include <string.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "lwip/pbuf.h"
#include "lwip/tcp.h"

#include "hardware/pio.h"
#include "sidpio.pio.h"

#include "hardware/clocks.h"

#define TCP_PORT 6581
#define DEBUG_printf printf
#define BUF_SIZE 2048
#define TEST_ITERATIONS 10
#define POLL_TIME_S 5

#define CS_DELAY 1

PIO pio;
uint sm;
uint offset;

void write_sid_pio(uint8_t addr,uint8_t data, uint16_t delay) {
    uint command = delay << 16 | ((addr << 8) | data);
    pio_sm_put_blocking(pio, sm, command);

}

void write_sid(uint8_t addr,uint8_t data, uint16_t delay) {

	data &= 0xff;
	addr &= 0x1f;
	
    for(int i  = 0;i < 5; i++) {
		gpio_put(ADDR_OFFSET - i, (addr >> (4 - i)) & 1);
	}
    for(int i = 0;i < 8; i++) {     
        gpio_put(DATA_OFFSET - i, (data >> i) & 1);
	}
    gpio_put(CS,0);
    sleep_us(CS_DELAY);	
    gpio_put(CS,1);
    sleep_us(delay);	
	
}

void init_pins() {

    // gpio_init(CS);
    // gpio_set_dir(CS, GPIO_OUT);

    gpio_init(RES);
    gpio_set_dir(RES, GPIO_OUT);
    gpio_put(RES,0);

    // gpio_init(A0);
    // gpio_set_dir(A0, GPIO_OUT);
    // gpio_init(A1);
    // gpio_set_dir(A1, GPIO_OUT);
    // gpio_init(A2);
    // gpio_set_dir(A2, GPIO_OUT);
    // gpio_init(A3);
    // gpio_set_dir(A3, GPIO_OUT);
    // gpio_init(A4);
    // gpio_set_dir(A4, GPIO_OUT);
    // gpio_init(D0);
    // gpio_set_dir(D0, GPIO_OUT);
    // gpio_init(D1);
    // gpio_set_dir(D1, GPIO_OUT);
    // gpio_init(D2);
    // gpio_set_dir(D2, GPIO_OUT);
    // gpio_init(D3);
    // gpio_set_dir(D3, GPIO_OUT);
    // gpio_init(D4);
    // gpio_set_dir(D4, GPIO_OUT);
    // gpio_init(D5);
    // gpio_set_dir(D5, GPIO_OUT);
    // gpio_init(D6);
    // gpio_set_dir(D6, GPIO_OUT);
    // gpio_init(D7);
    // gpio_set_dir(D7, GPIO_OUT);

    
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
        write_sid_pio(i,0,0);    
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

    printf("Test Beep\n");
    write_sid_pio(24,15,20);
  
    while(true) {

        printf("Beep\n");
        write_sid_pio(24,21,20);
        write_sid_pio(5,9,20);
        write_sid_pio(6,0,20);
        write_sid_pio(1,48,20);
        write_sid_pio(4,32,20);
        write_sid_pio(4,33,20);
        sleep_ms(2000); 
    }
}
void sid_command() {

    while(true) {
        uint32_t command = multicore_fifo_pop_blocking();
        uint8_t delayHigh = command >> 24;
        uint8_t delayLow = command >> 16;
        uint8_t reg = command >> 8;
        uint8_t value = command & 0xff;
        write_sid(reg,value, (delayHigh << 8) | delayLow);
    }
}

void sid_command_pio() {

    while(true) {
        uint32_t command = multicore_fifo_pop_blocking();
        pio_sm_put_blocking(pio, sm, command);
        
    }
}

#define BUFFER_LENGTH 100
static sid_server_t* tcp_server_init(void) {
    sid_server_t *state = calloc(1, sizeof(sid_server_t));
    if (!state) {
        DEBUG_printf("failed to allocate state\n");
        return NULL;
    }
    return state;
}

static err_t tcp_server_close(void *arg) {
    sid_server_t *state = (sid_server_t*)arg;
    err_t err = ERR_OK;
    if (state->client_pcb != NULL) {
        tcp_arg(state->client_pcb, NULL);
        tcp_poll(state->client_pcb, NULL, 0);
        tcp_sent(state->client_pcb, NULL);
        tcp_recv(state->client_pcb, NULL);
        tcp_err(state->client_pcb, NULL);
        err = tcp_close(state->client_pcb);
        if (err != ERR_OK) {
            DEBUG_printf("close failed %d, calling abort\n", err);
            tcp_abort(state->client_pcb);
            err = ERR_ABRT;
        }
        state->client_pcb = NULL;
    }
    if (state->server_pcb) {
        tcp_arg(state->server_pcb, NULL);
        tcp_close(state->server_pcb);
        state->server_pcb = NULL;
    }
    return err;
}

static err_t tcp_server_sent(void *arg, struct tcp_pcb *tpcb, u16_t len) {
    sid_server_t *state = (sid_server_t*)arg;

    if (state->sent_len >= BUF_SIZE) {

        // We should get the data back from the client
        state->recv_len = 0;
        DEBUG_printf("Waiting for buffer from client\n");
    }

    return ERR_OK;
}

err_t tcp_server_send_data(void *arg, struct tcp_pcb *tpcb, int length)
{
    sid_server_t *state = (sid_server_t*)arg;
    
    state->sent_len = 0;
    //DEBUG_printf("Writing %ld bytes to client\n", length);
    // this method is callback from lwIP, so cyw43_arch_lwip_begin is not required, however you
    // can use this method to cause an assertion in debug mode, if this method is called when
    // cyw43_arch_lwip_begin IS needed
    cyw43_arch_lwip_check();
    err_t err = tcp_write(tpcb, state->buffer_sent, length, TCP_WRITE_FLAG_COPY);
    if (err != ERR_OK) {
        DEBUG_printf("Failed to write data %d\n", err);
        return err; //tcp_server_result(arg, -1);
    }
    return ERR_OK;
}

err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    sid_server_t *state = (sid_server_t*)arg;
    if (!p) {
        return ERR_CLSD; //(arg, -1);
    }
    state->recv_len = 0;
    // this method is callback from lwIP, so cyw43_arch_lwip_begin is not required, however you
    // can use this method to cause an assertion in debug mode, if this method is called when
    // cyw43_arch_lwip_begin IS needed
    cyw43_arch_lwip_check();
    if (p->tot_len > 0) {
        //DEBUG_printf("tcp_server_recv %d/%d err %d\n", p->tot_len, state->recv_len, err);

        pbuf_copy_partial(p, state->buffer_recv, p->tot_len, 0);
        tcp_recved(tpcb, p->tot_len);
        processReadBuffer(state);

        tcp_server_send_data(arg, tpcb, state->sent_len);
        
        state->sent_len = 0;
        
    }
    pbuf_free(p);

    // Have we have received the whole buffer
    return ERR_OK;
}

static err_t tcp_server_poll(void *arg, struct tcp_pcb *tpcb) {
 
    return ERR_TIMEOUT; //(arg, -1); // no response is an error?
}

static void tcp_server_err(void *arg, err_t err) {
    if (err != ERR_ABRT) {
        DEBUG_printf("tcp_client_err_fn %d\n", err);
        //tcp_server_result(arg, err);
    }
}

static err_t tcp_server_accept(void *arg, struct tcp_pcb *client_pcb, err_t err) {
    sid_server_t *state = (sid_server_t*)arg;
    if (err != ERR_OK || client_pcb == NULL) {
        DEBUG_printf("Failure in accept\n");
        return ERR_VAL;
    }
    DEBUG_printf("Client connected\n");

    state->client_pcb = client_pcb;
    tcp_arg(client_pcb, state);
    tcp_sent(client_pcb, tcp_server_sent);
    tcp_recv(client_pcb, tcp_server_recv);
    tcp_poll(client_pcb, tcp_server_poll, POLL_TIME_S * 2);
    tcp_err(client_pcb, tcp_server_err);

    return ERR_OK; // tcp_server_send_data(arg, state->client_pcb);
}

static bool tcp_server_open(void *arg) {
    sid_server_t *state = (sid_server_t*)arg;
    DEBUG_printf("Starting Sid Pico server at %s on port %u\n", ip4addr_ntoa(netif_ip4_addr(netif_list)), TCP_PORT);

    struct tcp_pcb *pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
    if (!pcb) {
        DEBUG_printf("failed to create pcb\n");
        return false;
    }

    err_t err = tcp_bind(pcb, NULL, TCP_PORT);
    if (err) {
        DEBUG_printf("failed to bind to port %u\n", TCP_PORT);
        return false;
    }

    state->server_pcb = tcp_listen_with_backlog(pcb, 1);
    if (!state->server_pcb) {
        DEBUG_printf("failed to listen\n");
        if (pcb) {
            tcp_close(pcb);
        }
        return false;
    }

    tcp_arg(state->server_pcb, state);
    tcp_accept(state->server_pcb, tcp_server_accept);

    return true;
}

void run_sid_server_test(void) {
    sid_server_t *state = tcp_server_init();
    if (!state) {
        return;
    }
    if (!tcp_server_open(state)) {
        return;
    }
    while(true) {
        cyw43_arch_poll();
        cyw43_arch_wait_for_work_until(make_timeout_time_ms(1000));
    }
    free(state);
}

void test_addr() {
    
     printf("Test\n");
    while(true) {

        for(int i=0;i<8;i++) {
            printf("Addr %02x Data %02x\n",i % 5,i);
            write_sid_pio(1 << i,1 << i,0xffff);
            sleep_ms(500);        
        }
    }
}

int main() {

    static const float pio_freq = 125;
    
    pio = pio0;

    offset = pio_add_program(pio, &sidpio_program);

    sm = pio_claim_unused_sm(pio, true);
    
    stdio_init_all();

    printf("Connecting to network\n");
    if (cyw43_arch_init()) {
            printf("failed to initialise\n");
        return 1;
    }

    cyw43_arch_enable_sta_mode();

    printf("Connecting to Wi-Fi... %s\n",WIFI_SSID);
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("failed to connect.\n");
        return 1;
    } else {
        printf("Connected.\n");
    }

    multicore_launch_core1(sid_command_pio);

    init_pins();

//    sid_start_clock();

    sidpio_program_init(pio, sm, offset, BASE_PIN, CS, CLK);

    printf("Resetting SID\n");
    
    sid_reset();

    //test_beep();

    //test_addr();

    while(1) {
        run_sid_server_test();
    }
    
}

