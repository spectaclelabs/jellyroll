#ifndef JELLYROLL_DEVICE_DESKTOP
#define JELLYROLL_DEVICE_DESKTOP

#include "device.h"
#include "codec.rtaudio.h"

#ifndef AUDIO_DEVICE
#define AUDIO_DEVICE -1
#endif

template <size_t N>
class DesktopDevice : public Device<RtAudioCodec<N>, N> {
public:
    DesktopDevice() : Device<RtAudioCodec<N>, N>(AUDIO_DEVICE) {}
};

template <size_t N>
using BabadooN = DesktopDevice<N>;

#endif
