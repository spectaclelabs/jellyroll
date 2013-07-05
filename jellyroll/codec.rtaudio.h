#ifndef JELLYROLL_CODEC_RTAUDIO
#define JELLYROLL_CODEC_RTAUDIO

#include <iostream>

#include "thelonious/audio_device.h"
#include "thelonious/types.h"

#include "codec.h"

namespace jellyroll {

template <uint32_t N>
class RtAudioCodec : public Codec<N> {
public:
    RtAudioCodec(int device=-1) : device(device, device) {
    }

    void tick(thelonious::Block<N> & block) {
        device.tick(block);
    }

    void tickIn(thelonious::Block<N> & block) {
        device.tickIn(block);
    }

    void start() {
        device.start();
    }

    void wait() {
        std::cin.get();
    }

    void onAudio(void (*onAudioCallback)(void)) {
        device.onAudio(onAudioCallback);
    }

private:
    thelonious::AudioDeviceN<N> device;
};

} // namespace jellyroll

#endif
