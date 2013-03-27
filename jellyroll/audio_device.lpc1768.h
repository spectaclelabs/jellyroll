#ifndef AUDIO_DEVICE_LPC1768_H
#define AUDIO_DEVICE_LPC1768_H

#include <cstddef>
#include <utility>

#include "mbed.h"

#include "thelonious/types.h"
#include "thelonious/util.h"
#include "thelonious/constants/sizes.h"
#include "thelonious/constants/rates.h"

using namespace thelonious;

namespace jellyroll {

template <size_t inputChannels, size_t outputChannels, size_t blocksPerBuffer=2>
class AudioDevice {
public:
    AudioDevice() :
            out(P0_26), dout(P0_22),
            bufferSize(thelonious::constants::BLOCK_SIZE * blocksPerBuffer),
            readPosition(0), writePosition(bufferSize - 1) {
        readTicker.attach(this, &AudioDevice::readTask,
                          1.0 * thelonious::constants::INV_SAMPLE_RATE);
    }

    void readTask() {
        if (readPosition == writePosition) {
            // Buffer is empty, so signal underflow 
            dout = 1;
            return;
        }

        dout = 0;

        out.write(buffer[0][readPosition] * 0.5f + 0.5f);

        readPosition = (readPosition + 1) % bufferSize;
    }

    Block<inputChannels> read() {
    }


    void write(Block<outputChannels> outputBlock) {
        Chock & chock = outputBlock[0];
        for (uint32_t i=0; i<thelonious::constants::BLOCK_SIZE; i++) {
            while (writePosition + 1 % bufferSize == readPosition) {
                // Block while buffer is full
            }
            buffer[0][writePosition] = chock[i];
            writePosition = (writePosition + 1) % bufferSize;
        }
    }

private:
    Ticker readTicker;
    AnalogOut out;
    DigitalOut dout;

    const volatile uint32_t bufferSize;
    volatile uint32_t readPosition;
    volatile uint32_t writePosition;

    volatile float buffer[outputChannels][thelonious::constants::BLOCK_SIZE *
                                          blocksPerBuffer];
};

}

#endif
