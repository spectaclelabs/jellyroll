#ifndef AUDIO_DEVICE_TESTING_H
#define AUDIO_DEVICE_TESTING_H

#include <cstddef>
#include <functional>

#include "RtAudio.h"

#include "thelonious/types.h"
#include "thelonious/sizes.h"
#include "thelonious/rates.h"

using namespace thelonious;

namespace jellyroll {

template <size_t inputChannels, size_t outputChannels, size_t blocksPerBuffer=8>
class AudioDevice {
public:
    AudioDevice() : bufferSize(BLOCK_SIZE * blocksPerBuffer) {
        RtAudio::StreamParameters inputParameters;
        inputParameters.deviceId = TESTING_DEVICE;
        inputParameters.nChannels = inputChannels;

        RtAudio::StreamParameters outputParameters;
        outputParameters.deviceId = TESTING_DEVICE;
        outputParameters.nChannels = outputChannels;

        RtAudio::StreamOptions options;
        options.flags = RTAUDIO_NONINTERLEAVED;

        try {
            device.openStream(outputChannels > 0 ? &outputParameters: NULL,
                              inputChannels > 0 ? &inputParameters : NULL,
                              RTAUDIO_FLOAT32, SAMPLE_RATE, &bufferSize,
                              &AudioDevice::callback, (void *) this, &options);
            device.startStream();
        }
        catch (RtError& error) {
            error.printMessage();
            exit(0);
        }
    }

    ~AudioDevice() {
        if (device.isStreamRunning()) {
            device.stopStream();
        }
        if (device.isStreamOpen()) {
            device.closeStream();
        }
    }

    static int callback(void *outputSamples, void *inputSamples,
                 uint32_t nBufferFrames, double streamTime,
                 RtAudioStreamStatus status, void *device) {
        AudioDevice *castDevice = (AudioDevice *) device;
        castDevice->inputSamples = (Sample *) inputSamples;
        castDevice->outputSamples = (Sample *) outputSamples;
        
        for (uint32_t i=0; i<blocksPerBuffer; i++) {
            castDevice->runCallback();
        }
        return 0;
    }


    Block<inputChannels> read() {
        int i=0;
        for (auto it=inputBlock.begin(); it!=inputBlock.end(); it++, i++) {
            Chock chock = *it;
            Sample *channelStart = inputSamples + i * bufferSize;
            Sample *blockStart = channelStart + inputBlockIndex * BLOCK_SIZE;
            Sample *blockEnd = blockStart + BLOCK_SIZE;
            std::copy(blockStart, blockEnd, chock.begin());
        }

        inputBlockIndex++;
        if (inputBlockIndex == blocksPerBuffer) {
            inputBlockIndex = 0;
        }
    }

    void write(Block<outputChannels> outputBlock) {
        int i=0;
        for (auto it=outputBlock.begin(); it!=outputBlock.end(); it++, i++) {
            Chock chock = *it;
            Sample *channelStart = outputSamples + i * bufferSize;
            Sample *blockStart = channelStart + outputBlockIndex * BLOCK_SIZE;
            std::copy(chock.begin(), chock.end(), blockStart);
        }

        outputBlockIndex++;
        if (outputBlockIndex == blocksPerBuffer) {
            outputBlockIndex = 0;
        }
         
    };

    void onTick(const std::function<void()> &callback) {
        tickCallback = callback;
    }

    void runCallback() {
        if (tickCallback != nullptr) {
            tickCallback();
        }
    }

private:
    RtAudio device;
    std::function<void()> tickCallback;
    uint32_t bufferSize;

    Sample *inputSamples;
    Sample *outputSamples;

    Block<inputChannels> inputBlock;

    uint32_t inputBlockIndex;
    uint32_t outputBlockIndex;
};

}

#endif
