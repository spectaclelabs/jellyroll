#ifndef JELLYROLL_DEVICE_BABADOO
#define JELLYROLL_DEVICE_BABADOO

#include "mbed.h"

#include "device.h"
#include "codec.wm8731.h"

namespace jellyroll {

template <uint32_t M, uint32_t N>
class BabadooDevice : public Device<WM8731Codec<M, N>, M, N> {
public:
    BabadooDevice() :
            Device<WM8731Codec<M, N>, M, N>(PB_15, PB_12, PB_13, PC_6, PB_11,
                                            PB_10),
            outputSwitch(PC_9) {
        outputSwitch = 1;
    }
    /*
    void setInputGain(float gain) {
        codec.setInputGain(gain);
    }

    void setOutputGain(float gain) {
        codec.setOutputGain(gain);
    }

    void setInputType(InputType type) {
        codec.setInputType(type);
    }

    void setOutputType(OutputType type) {
        codec.setOutputType(type);
    }
    */
private:
    DigitalOut outputSwitch;
};

template <size_t M, size_t N>
using BabadooN = BabadooDevice<M, N>;

} // namespace jellyroll

#endif
