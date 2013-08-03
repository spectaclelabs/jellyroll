#ifndef JELLYROLL_CODEC_WM8731
#define JELLYROLL_CODEC_WM8731

#include "mbed.h"

#include "thelonious/types.h"
#include "thelonious/constants/sizes.h"

#include "codec.h"
#include "i2s.h"

namespace jellyroll {

template <size_t M, size_t N>
class WM8731Codec : public Codec<M, N> {
public:
    WM8731Codec(PinName i2c_sda, PinName i2c_scl, PinName i2s_sd,
                PinName i2s_ws, PinName i2s_ck, PinName i2s_mck,
                PinName i2s_ext_sd=NC) : 
            i2s(i2s_sd, i2s_ws, i2s_ck, i2s_mck, i2s_ext_sd),
            i2c(i2c_sda, i2c_scl) {
        i2s.setCallback(&WM8731Codec::callback, (void *) this);

        // Shoddy hack - should happen in hardware.  Enable pull ups on the
        // I2C lines
        GPIOB->PUPDR |= (1 << 20) | (1 << 22);

        i2c.frequency(10000);

        wait(0.1);

        // Power on, and enable Line In, ADC and DAC
        writeRegister(0x0C, 0x72);

        // Send DAC to outputs, disable bypass
        writeRegister(0x08, 0x12);

        // Unmute DAC
        writeRegister(0x0A, 0x00);

        // Unmute line in
        writeRegister(0x00, 0x17);

        // Set sample rate
        writeRegister(0x10, 0x20);

        // Set format
        writeRegister(0x0E, 0x02);

        // Set active
        writeRegister(0x12, 0x01);

        // Enable outputs
        writeRegister(0x0C, 0x62);
    }

    void start() {
        i2s.start();
    }

    void tickOut(thelonious::Block<M> &samples) {
        for (uint32_t i=0; i<thelonious::constants::BLOCK_SIZE; i++) {
            for (uint32_t j=0; j<2; j++) {
                if (j >= M) {
                    continue;
                }
                samples[j][i] = ((float) inputBuffer[i * 2 + j]) / 0x7FFF;
            }
        }
    }

    void tickIn(thelonious::Block<N> &samples) {
        for (uint32_t i=0; i<thelonious::constants::BLOCK_SIZE; i++) {
            for (uint32_t j=0; j<2; j++) {
                if (j >= N) {
                    continue;
                }
                float sample = samples[j][i];
                sample = sample > 1.f ? 1.f : sample;
                sample = sample < -1.f ? -1.f : sample;
                outputBuffer[i * 2 + j] = sample * 0x7FFF;
            }
        }
   }

private:
    void writeRegister(char reg, char value) {
        char data[2] = {reg, value};
        i2c.write(0x34, data, 2);
    }

    static void callback(int16_t *inputSamples, int16_t *outputSamples,
                         uint32_t numberOfSamples, void *device) {
        WM8731Codec *castDevice = (WM8731Codec *) device;
        castDevice->setInputBuffer(inputSamples);
        castDevice->setOutputBuffer(outputSamples);
        if (castDevice->onAudioCallback != nullptr) {
            (*(castDevice->onAudioCallback))();
        }
    }

    void setInputBuffer(int16_t *buffer) {
        inputBuffer = buffer;
    }

    void setOutputBuffer(int16_t *buffer) {
        outputBuffer = buffer;
    }


    I2S i2s;
    I2C i2c;

    int16_t *inputBuffer;
    int16_t *outputBuffer;
};

} // namespace jellyroll


#endif
