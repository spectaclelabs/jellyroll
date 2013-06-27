#ifndef JELLYROLL_CODEC
#define JELLYROLL_CODEC

#include "thelonious/duplex.h"
#include "thelonious/types.h"

using namespace thelonious;

template <uint32_t N>
class Codec : public Duplex<N> {
public:
    virtual void tick(Block<N> & block) = 0;
    virtual void tickIn(Block<N> & block) = 0;
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

#endif
