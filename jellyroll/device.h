#ifndef JELLYROLL_DEVICE
#define JELLYROLL_DEVICE

#include "thelonious/types.h"
#include "thelonious/duplex.h"

#include "io_types.h"

namespace jellyroll {

template <typename CodecType, uint32_t M, uint32_t N>
class Device : public CodecType {
public:
    // Constructor simply forward arguments to codec
    template<typename... Args>
    Device(Args&&... args) : CodecType(std::forward<Args>(args)...) {}

    virtual void setInputGain(float gain) {
    }

    virtual void setOutputGain(float gain) {
    }

    virtual void setInputType(InputType type) {
    }

    virtual void setOutputType(OutputType type) {
    }
};

} // namespace jellyroll

#endif
