#ifndef JELLYROLL_CODEC_RTAUDIO
#define JELLYROLL_CODEC_RTAUDIO

#include <iostream>

#include "thelonious/audio_device.h"
#include "thelonious/types.h"

#include "codec.h"

namespace jellyroll {

template <uint32_t M, uint32_t N>
class RtAudioCodec : public Codec<M, N> {
public:
    RtAudioCodec(int inputDevice=-1, int outputDevice=-1) :
                 device(inputDevice, outputDevice) {
    }

    void tickOut(thelonious::Block<N> & block) {
        device.tickOut(block);
    }

    void tickIn(thelonious::Block<M> & block) {
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
    thelonious::AudioDeviceN<M, N> device;
};

} // namespace jellyroll

#endif
