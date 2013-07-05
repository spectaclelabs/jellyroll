#ifndef JELLYROLL_IO_TYPES
#define JELLYROLL_IO_TYPES

namespace jellyroll {

enum InputType {
    LINE_IN,
    INSTRUMENT_IN,
    MICROPHONE_IN
};

enum OutputType {
    LINE_OUT,
    HEADPHONE_OUT,
    SPEAKER_OUT
};

} // namespace jellyroll

#endif
