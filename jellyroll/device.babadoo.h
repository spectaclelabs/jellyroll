#ifndef JELLYROLL_BABADOO_BABADOO
#define JELLYROLL_BABADOO_BABADOO

#include "thelonious.h"

template <uint32_t N>
class BabadooDevice : Device<WM8731Codec, N> {
public:
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
};

#endif
