#include <driver/dac.h>
#include <Arduino.h>
#include <WebServer.h>
#include "btAudio.h"

void analog_out_sink_callback(const uint8_t *data, uint32_t len);

/* audio device name */
btAudio audio = btAudio("Fieldphone");

/* timer0 configuration */
hw_timer_t *timer0_config = nullptr;

/* ringbuffer */
uint8_t ringbuffer[2048 * 2];
uint32_t ringbuffer_head = 0;
uint32_t ringbuffer_tail = 0;

/* timer0 ISR function */
void IRAM_ATTR timer0_ISR() {
    /* write from ringbuffer */
    if (ringbuffer_head != ringbuffer_tail) {
        dac_output_voltage(DAC_CHANNEL_1, ringbuffer[ringbuffer_tail]);
        ringbuffer_tail++;
        if (ringbuffer_tail == sizeof(ringbuffer)) {
            ringbuffer_tail = 0;
        }
    } else {
        dac_output_voltage(DAC_CHANNEL_1, 0);
    }
}

void setup() {
    /* stream audio data to the esp32 */
    audio.begin();

    /* re-connect to last connected device */
    audio.reconnect();

    /* register custom callback */
    audio.setSinkCallback(analog_out_sink_callback);

    /* configure timer0 interrupt */
    timer0_config = timerBegin(0, 8, true);
    timerAttachInterrupt(timer0_config, &timer0_ISR, true);
    timerAlarmWrite(timer0_config, 227, true); // 230
    timerAlarmEnable(timer0_config);

    /* enable DAC channel 1 */
    dac_output_enable(DAC_CHANNEL_1);
}

void loop() {
}

void analog_out_sink_callback(const uint8_t *data, uint32_t len) {
    /* number of 16 bit samples */
    uint32_t n = len / 2;

    /* point to a 16 bit sample */
    int16_t *data16 = (int16_t *) data;

    for (int i = 0; i < n; i++) {
        /* write data to ringbuffer */
        ringbuffer[ringbuffer_head] = *data16 / 128;
        ringbuffer[ringbuffer_head] = ringbuffer[ringbuffer_head] - 128;
        ringbuffer_head++;
        if (ringbuffer_head == sizeof(ringbuffer)) {
            ringbuffer_head = 0;
        }

        /* move to next memory address */
        data16++;
    }
}
