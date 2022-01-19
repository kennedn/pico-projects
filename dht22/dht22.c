#include "hardware/pio.h"
#include "dht22.pio.h"
#include "dht22.h"

static PIO s_pio;
static uint s_sm;
static uint s_offset;
static bool s_enabled;

// Fifo's / OSR likely dirty from sensor disconnect, reset everything for a clean slate
static void dht_reset_sm() {
    pio_sm_set_enabled(s_pio, s_sm, false);
    pio_sm_clear_fifos(s_pio,s_sm);
    pio_sm_restart(s_pio, s_sm);
    pio_sm_set_enabled(s_pio, s_sm, true);
}

bool dht_get(DHTReading *reading_buffer) {
    static uint32_t data[5];
    
    // Restart execution of dht pio block, causing a read from dht21
    if(s_enabled) {
        pio_sm_exec(s_pio, s_sm, pio_encode_jmp(s_offset));
    } else {
        pio_sm_set_enabled(s_pio, s_sm, true);
        s_enabled = true;
    }

    for(int i=0; i < count_of(data); i++) {
        data[i] = pio_sm_get(s_pio, s_sm);
    }

    if(((data[0] + data[1] + data[2] + data[3]) & 0xFF) != data[4] || data[4] == 0) {
        dht_reset_sm();
        return false;
    }

    reading_buffer->humidity = (data[0] << 8 | data[1]) * 0.1;
        // First bit of data[2] must be treated as a positive / negative flag
    reading_buffer->temperature = ((data[2] & 0x80) ? -1 : 1) * ((data[2] & 0x7F) << 8 | data[3]) * 0.1;
    // Checksum not ok
    return true;
}

void dht_init() {
    s_pio = pio0;
    s_offset = pio_add_program(s_pio, &dht22_program);
    s_sm = pio_claim_unused_sm(s_pio, true);
    s_enabled = false;
    dht22_program_init(s_pio, s_sm, s_offset, 6);
}