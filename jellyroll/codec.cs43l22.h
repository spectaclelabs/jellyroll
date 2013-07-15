#ifndef JELLYROLL_CODEC_CS43L22
#define JELLYROLL_CODEC_CS43L22

#include "mbed.h"

#include "thelonious/types.h"
#include "thelonious/constants/sizes.h"

#include "codec.h"
#include "i2s.h"

namespace jellyroll {

template <size_t M, size_t N>
class CS43L22Codec : public Codec<M, N> {
public:
    CS43L22Codec(PinName i2s_sd, PinName i2s_ws, PinName i2s_ck,
                 PinName i2s_mck, PinName i2c_sda, PinName i2c_scl,
                 PinName reset) : 
            i2s(i2s_sd, i2s_ws, i2s_ck, i2s_mck), i2c(i2c_sda, i2c_scl),
            reset(reset) {
        i2s.setCallback(&CS43L22Codec::callback, (void *) this);

        this->reset = 0;
        wait(0.1);
        this->reset = 1;

        // Initialization
        writeRegister(0x00, 0x99);
        writeRegister(0x47, 0x80);
        writeRegister(0x32, (1 << 7));
        writeRegister(0x32, (0 << 7));
        writeRegister(0x00, 0x00);

        // Unmute
        writeRegister(0x04, 0xAF);

        // Auto detect clock
        writeRegister(0x05, 0x80);

        // I2S Slave Mode, 16 bit
        writeRegister(0x06, 0x07);

        // Power up
        writeRegister(0x02, 0x9E);
    }

    void start() {
        i2s.start();
    }

    void tickOut(thelonious::Block<N> &samples) {
    }

    void tickIn(thelonious::Block<M> &samples) {
        for (uint32_t i=0; i<thelonious::constants::BLOCK_SIZE; i++) {
            for (uint32_t j=0; j<2; j++) {
                if (j >= M) {
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
        i2c.write(0x94, data, 2);
    }

    char readRegister(char reg) {
        i2c.write(0x94, &reg, 1);
        char out[1]; 
        i2c.read(0x94, out, 1);
        return out[0];
    }

    static void callback(int16_t *outputSamples, uint32_t numberOfSamples,
                         void *device) {
        CS43L22Codec *castDevice = (CS43L22Codec *) device;
        castDevice->setBuffer(outputSamples);
        if (castDevice->onAudioCallback != nullptr) {
            (*(castDevice->onAudioCallback))();
        }
    }

    void setBuffer(int16_t *buffer) {
        outputBuffer = buffer;
    }

    I2S i2s;
    I2C i2c;
    DigitalOut reset;

    int16_t *outputBuffer;
};

} // namespace jellyroll

#endif
