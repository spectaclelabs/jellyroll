#ifndef JELLYROLL_CODEC
#define JELLYROLL_CODEC

#include "thelonious/duplex.h"
#include "thelonious/types.h"

namespace jellyroll {

template <uint32_t M, uint32_t N>
class Codec : public thelonious::Duplex<M, N> {
public:
    virtual void start() = 0;
    virtual void wait() {
        while (1);
    }


    virtual void onAudio(void (*onAudioCallback)(void)) {
        this->onAudioCallback = onAudioCallback;
    }

protected:
    void (*onAudioCallback)();
};

} // namespace jellyroll

#endif
