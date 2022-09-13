#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "nec.pio.h"
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "lwip/pbuf.h"
#include "lwip/tcp.h"

#define TCP_PORT 8080
#define DEBUG_printf printf
#define BUF_SIZE 2048
#define POLL_TIME_S 5
#define LED_PIN 16

#define max(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })
// Macros to generate the lookup table (at compile-time)
#define R2(n) n, n + 2*64, n + 1*64, n + 3*64
#define R4(n) R2(n), R2(n + 2*16), R2(n + 1*16), R2(n + 3*16)
#define R6(n) R4(n), R4(n + 2*4 ), R4(n + 1*4 ), R4(n + 3*4 )
#define REVERSE_BITS R6(0), R6(2), R6(1), R6(3)
 
typedef enum HTTP_METHOD_T_ {
    HTTP_METHOD_GET,
    HTTP_METHOD_POST,
    HTTP_METHOD_PUT
} HTTP_METHOD_T;

typedef enum HTTP_VERSION_T_ {
    HTTP_VERSION_1,
    HTTP_VERSION_1_1,
    HTTP_VERSION_2,
    HTTP_VERSION_3,
} HTTP_VERSION_T;

typedef struct HTTP_MESSAGE_BODY_T_ {
    HTTP_METHOD_T method;
    HTTP_VERSION_T version;
    char url[20];
    long long code;
} HTTP_MESSAGE_BODY_T;

typedef struct TCP_SERVER_T_ {
    struct tcp_pcb *server_pcb;
    struct tcp_pcb *client_pcb;
    uint8_t buffer_recv[BUF_SIZE];
    uint8_t buffer_send[BUF_SIZE];
    int recv_len;
    int send_len;
    int payload_len;
    int run_count;
    HTTP_MESSAGE_BODY_T *message_body;
} TCP_SERVER_T;

PIO pio;

// lookup table to store the reverse of each index of the table.
// The macro `REVERSE_BITS` generates the table
unsigned int lookup[256] = { REVERSE_BITS };
uint32_t rgb_mask = 1 << 17 | 1 << 18 | 1 << 19;
uint32_t rgb_base_pin = 17;
// Function to reverse bits of `n` using a lookup table
int reverseBits(int n) {
    return lookup[n & 0xff] << 24 |                // consider the first 8 bits
           lookup[(n >> 8) & 0xff] << 16 |         // consider the next 8 bits
           lookup[(n >> 16) & 0xff] << 8 |         // consider the next 8 bits
           lookup[(n >> 24) & 0xff];               // consider last 8 bits
}

static TCP_SERVER_T* tcp_server_init(void) {
    TCP_SERVER_T *state = calloc(1, sizeof(TCP_SERVER_T));
    state->message_body = (HTTP_MESSAGE_BODY_T*)calloc(1, sizeof(HTTP_MESSAGE_BODY_T));
    if (!state) {
        DEBUG_printf("failed to allocate state\n");
        return NULL;
    }
    return state;
}

static err_t tcp_client_close(void *arg) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    err_t err = ERR_OK;
    if (state->client_pcb == NULL) { return err; }
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
    return err;
}

static void tcp_server_close(void *arg) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    if (state->server_pcb == NULL) { return; }
    tcp_arg(state->server_pcb, NULL);
    tcp_close(state->server_pcb);
    state->server_pcb = NULL;
}

static err_t tcp_server_send(void *arg, struct tcp_pcb *tpcb, u16_t len) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    DEBUG_printf("tcp_server_send %u\n", len);
    state->send_len += len;

    if (state->send_len == state->payload_len) {
        state->recv_len = 0;
        state->payload_len = 0;
        DEBUG_printf("tcp_server_send buffer ok\n");
        return tcp_client_close(arg);
    }
    return ERR_OK;
}

err_t tcp_server_send_data(void *arg, struct tcp_pcb *tpcb)
{
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;

    state->send_len = 0;
    DEBUG_printf("Writing %ld bytes to client\n", state->payload_len);
    // this method is callback from lwIP, so cyw43_arch_lwip_begin is not required, however you
    // can use this method to cause an assertion in debug mode, if this method is called when
    // cyw43_arch_lwip_begin IS needed
    cyw43_arch_lwip_check();
    err_t err = tcp_write(tpcb, state->buffer_send, state->payload_len, TCP_WRITE_FLAG_COPY);
    if (err != ERR_OK) {
        DEBUG_printf("Failed to write data %d\n", err);
        return tcp_client_close(arg);
    }
    return ERR_OK;
}

static void http_message_body_parse(void *arg) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    if (state->buffer_recv == NULL) { return; }
    state->message_body->method = HTTP_METHOD_GET;
    state->message_body->url[0] = '\0';
    state->message_body->version = HTTP_VERSION_1;
    state->message_body->code = -1;

    char *message_body = (char*)state->buffer_recv;
    char *delim = " ?=&\r";
    char next_delim;
    char current_delim = '\0';
    char *key;
    char *token;
    while(1) {
        if (*message_body == '\0') { break; }
        token = message_body;
        message_body = strpbrk(message_body, delim);
        if (message_body == NULL) { break; }
        next_delim = *message_body;
        *message_body = '\0';

        switch(current_delim) {
            case '\0':
                DEBUG_printf("http_message_body_parse method: %s\n", token);
                if(!strcmp(token, "GET")) { state->message_body->method = HTTP_METHOD_GET; }
                else if(!strcmp(token, "PUT")) { state->message_body->method = HTTP_METHOD_PUT; }
                else if(!strcmp(token, "POST")) { state->message_body->method = HTTP_METHOD_POST; }
                break;
            case ' ':
                if (next_delim != '\r') {
                    DEBUG_printf("http_message_body_parse url: %s\n", token);
                    strncpy(state->message_body->url, token, count_of(state->message_body->url));
                } else {
                    DEBUG_printf("http_message_body_parse http_ver: %s\n", token);
                    if(!strcmp(token, "HTTP/1")) { state->message_body->version = HTTP_VERSION_1; }
                    else if(!strcmp(token, "HTTP/1.1")) { state->message_body->version = HTTP_VERSION_1_1; }
                    else if(!strcmp(token, "HTTP/2")) { state->message_body->version = HTTP_VERSION_2; }
                    else if(!strcmp(token, "HTTP/3")) { state->message_body->version = HTTP_VERSION_3; }
                }
                break;
            case '?':
            case '&':
                    DEBUG_printf("http_message_body_parse key: %s\n", token);
                    key = token;
                break;
            case '=':
                    DEBUG_printf("http_message_body_parse value: %s\n", token);
                    if (!strcmp(key, "code")) {
                        state->message_body->code = strtoll(token, NULL, 16); 
                        DEBUG_printf("http_message_body_parse code: %llu\n", state->message_body->code);
                    }
                break; 
        }
        current_delim = next_delim;
        message_body++;
        if (current_delim == '\r') { break; }
    }
    if (state->message_body->code != -1) { return; }
    if (strstr(message_body, "Content-Type: application/json") == NULL) { return; }
    message_body = strrchr(message_body, '\n');
    message_body++;  // message_body = '{"code": "00F7C03F"}'
    DEBUG_printf("http_message_body_parse message_body: %s\n", message_body);


    // Lazy man's JSON parser, iterate over key-value pairs from JSON string, ignoring non string values. Treats JSON as a flat file.

    // Seek past first occurance of '{'
    message_body = strpbrk(message_body, "{");
    message_body ++;  // message_body = '"code": "00F7C03F"}'
    key = NULL;
    while(1) {
        // Seek past first occurance of '\"'
        message_body = strpbrk(message_body, "\"");
        if (message_body == NULL) { break; }
        message_body++; // message_body = 'code": "00F7C03F"}'

        // Null terminate the next occurance of '\"' and seek past it, store reference to substring in token
        token = message_body;  
        message_body = strpbrk(message_body, "\"");
        if (message_body == NULL) { break; }
        *message_body = '\0';  // token = 'code'
        message_body++; // message_body = ': "00F7C03F"}'
        DEBUG_printf("http_message_body_parse key: %s\n", token);

        // Store token in key for later validation
        key = token;

        // Null terminate the next occurance of '\"', store reference to substring in token
        token = message_body;
        message_body = strpbrk(message_body, "\"");
        if (message_body == NULL) { break; }
        *message_body = '\0';  // token = ': '

        // If the token does not appear to be a valid key-value seperator, this means the next value is not a string, 
        // to ignore it we must rewind the last strpbrk / null termination and continue on to next key.
        for(int i=0; token[i]; i++) {
            if (token[i] == ' ' || token[i] == ':') { continue; }
            DEBUG_printf("None string value detected, skipping\n"); 
            *message_body = '\"';
            break;
        }
        if (*message_body == '\"') { 
            message_body--;
            continue; 
        }

        // Value looks valid, seek past '\"', null terminate the next occurance of '\"', store reference to value in token
        message_body++;  // message_body = '00F7C03F"}'
        token = message_body;
        message_body = strpbrk(message_body, "\"");
        if (message_body == NULL) { break; }
        *message_body = '\0';  // token = '00F7C03F'
        DEBUG_printf("http_message_body_parse value: %s\n", token);

        // Validate that the key associated with this value is the one we want. If so, capture the value and return
        if (!strcmp(key, "code")) { 
            state->message_body->code = strtoll(token, NULL, 16); 
            DEBUG_printf("http_message_body_parse code: %llu\n", state->message_body->code);
            return; 
        }
        message_body++;
    }
}

void tcp_server_process_recv_data(void *arg, struct tcp_pcb *tpcb) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    char json_body[1024];
    http_message_body_parse(arg);
    if (state->message_body->method == HTTP_METHOD_PUT && 
        state->message_body->version == HTTP_VERSION_1_1 &&
        !strcmp(state->message_body->url, "/ir")) {
        if (state->message_body->code != -1) {
            int json_body_len = sprintf(json_body, "{\"status\": \"OK\"}\n");
            state->payload_len = sprintf((char*)state->buffer_send, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\n\r\n%s", json_body_len, json_body);
            
            pio_sm_put_blocking(pio, 0, reverseBits(state->message_body->code));
        } else {
            int json_body_len = sprintf(json_body, "{\"message\": \"Code variable required\"}\n");
            state->payload_len = sprintf((char*)state->buffer_send, "HTTP/1.1 400 Bad Request\r\nContent-Length: %d\r\n\r\n%s", json_body_len, json_body);
        }
    } else if (state->message_body->method == HTTP_METHOD_GET && 
        state->message_body->version == HTTP_VERSION_1_1 &&
        !strcmp(state->message_body->url, "/status")) {
        int json_body_len = 0;
        while (json_body_len == 0) {
            uint32_t gpio = (gpio_get_all() & rgb_mask) >> rgb_base_pin;

            switch(gpio) {
                case 0b110: // red
                    json_body_len = sprintf(json_body, "{\"status\": \"off\"}\n");
                    break;
                case 0b100: // yellow
                    json_body_len = sprintf(json_body, "{\"status\": \"optical\"}\n");
                    break;
                case 0b000: // white
                    json_body_len = sprintf(json_body, "{\"status\": \"aux\"}\n");
                    break;
                case 0b101: // green
                    json_body_len = sprintf(json_body, "{\"status\": \"line-in\"}\n");
                    break;
                case 0b011: // blue
                    json_body_len = sprintf(json_body, "{\"status\": \"bluetooth\"}\n");
                    break;
                case 0b111: // off
                    continue;
            }
            busy_wait_ms(100);
        }
        state->payload_len = sprintf((char*)state->buffer_send, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\n\r\n%s", json_body_len, json_body);
    } else {
        int json_body_len = sprintf(json_body, "{\"status\": \"NG\"}\n");
        state->payload_len = sprintf((char*)state->buffer_send, "HTTP/1.1 400 Bad Request\r\nContent-Length: %d\r\n\r\n%s", json_body_len, json_body);
    }

    tcp_server_send_data(arg, tpcb);
}

err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    if (!p) {
        return tcp_client_close(arg);
    }
    // this method is callback from lwIP, so cyw43_arch_lwip_begin is not required, however you
    // can use this method to cause an assertion in debug mode, if this method is called when
    // cyw43_arch_lwip_begin IS needed
    cyw43_arch_lwip_check();
    if (p->tot_len > 0) {
        DEBUG_printf("tcp_server_recv %d/%d err %d\n", p->tot_len, state->recv_len, err);

        // Receive the buffer
        const uint16_t buffer_left = BUF_SIZE - state->recv_len;
        state->recv_len += pbuf_copy_partial(p, state->buffer_recv + state->recv_len,
                                             p->tot_len > buffer_left ? buffer_left : p->tot_len, 0);
        tcp_recved(tpcb, p->tot_len);
    }

    // Have we have received the whole buffer
    if (state->recv_len == p->tot_len) {
        DEBUG_printf("tcp_server_recv buffer ok: %s\n", state->buffer_recv);
        tcp_server_process_recv_data(arg, tpcb);
    }
    pbuf_free(p);
    return ERR_OK;
}

static err_t tcp_server_poll(void *arg, struct tcp_pcb *tpcb) {
    DEBUG_printf("tcp_server_poll_fn\n");
    return tcp_client_close(arg);
}

static void tcp_server_err(void *arg, err_t err) {
    if (err != ERR_ABRT) {
        DEBUG_printf("tcp_client_err_fn %d\n", err);
        tcp_client_close(arg);
    }
}

static err_t tcp_server_accept(void *arg, struct tcp_pcb *client_pcb, err_t err) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    if (err != ERR_OK || client_pcb == NULL) {
        DEBUG_printf("Failure in accept\n");
        tcp_client_close(arg);
        return ERR_VAL;
    }
    DEBUG_printf("----------------\n");
    DEBUG_printf("Client connected\n");

    state->client_pcb = client_pcb;
    tcp_arg(client_pcb, state);
    tcp_sent(client_pcb, tcp_server_send);
    tcp_recv(client_pcb, tcp_server_recv);
    tcp_poll(client_pcb, tcp_server_poll, POLL_TIME_S * 2);
    tcp_err(client_pcb, tcp_server_err);

    return ERR_OK;
}

static bool tcp_server_open(void *arg) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    DEBUG_printf("Starting server at %s on port %u\n", ip4addr_ntoa(netif_ip4_addr(netif_list)), TCP_PORT);

    struct tcp_pcb *pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
    if (!pcb) {
        DEBUG_printf("failed to create pcb\n");
        return false;
    }

    err_t err = tcp_bind(pcb, NULL, TCP_PORT);
    if (err) {
        DEBUG_printf("failed to bind to port %d\n");
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

void run_tcp_server(void) {
    TCP_SERVER_T *state = tcp_server_init();
    if (!state) {
        return;
    }
    if (!tcp_server_open(state)) {
        tcp_client_close(state);
        tcp_server_close(state);
        free(state->message_body);
        free(state);
        return;
    }

    while(1) {
        tight_loop_contents();
    }

    tcp_client_close(state);
    tcp_server_close(state);
    free(state->message_body);
    free(state);
}

int main() {
    stdio_init_all();
    gpio_init_mask(rgb_mask);
    pio = pio0;
    uint nec_offset = pio_add_program(pio, &nec_program);
    nec_program_init(pio, 0 , nec_offset, LED_PIN);

    if (cyw43_arch_init()) {
        printf("failed to initialise\n");
        return 1;
    }

    cyw43_arch_enable_sta_mode();

    printf("Connecting to WiFi...\n");
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("failed to connect.\n");
        return 1;
    } else {
        printf("Connected.\n");
    }
    run_tcp_server();
    cyw43_arch_deinit();
    return 0;
}