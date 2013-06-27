#ifndef JELLYROLL_DEVICE_STM32F4DISCOVERY
#define JELLYROLL_DEVICE_STM32F4DISCOVERY

#include "mbed.h"

#include "device.h"
#include "codec.cs43l22.h"

template <size_t N>
class STM32F4DiscoveryDevice : public Device<CS43L22Codec<N>, N> {
public:
    STM32F4DiscoveryDevice() :
            Device<CS43L22Codec<N>, N>(PC_12, PA_4, PC_10, PC_7, PB_9, PB_6,
                                       PD_4) {
    }
};

template <size_t N>
using BabadooN = STM32F4DiscoveryDevice<N>;

#endif
