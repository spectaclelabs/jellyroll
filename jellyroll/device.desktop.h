#ifndef JELLYROLL_DEVICE_DESKTOP
#define JELLYROLL_DEVICE_DESKTOP

#include "device.h"
#include "codec.rtaudio.h"

#ifndef AUDIO_DEVICE
#define AUDIO_DEVICE -1
#endif

namespace jellyroll {

template <size_t M, size_t N>
class DesktopDevice : public Device<RtAudioCodec<M, N>> {
public:
    DesktopDevice() : Device<RtAudioCodec<M, N>>(AUDIO_DEVICE, AUDIO_DEVICE) {}
};

template <size_t M, size_t N>
using BabadooN = DesktopDevice<M, N>;

} // namespace jellyroll

#endif
