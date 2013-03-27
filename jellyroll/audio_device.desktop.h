#ifndef AUDIO_DEVICE_DESKTOP_H
#define AUDIO_DEVICE_DESKTOP_H

#include <cstddef>
#include <functional>
#include <thread>
#include <chrono>

#include "RtAudio.h"

#include "thelonious/types.h"
#include "thelonious/constants/sizes.h"
#include "thelonious/constants/rates.h"

#include "desktop.h"

using namespace thelonious;

namespace jellyroll {

template <size_t inputChannels, size_t outputChannels, size_t blocksPerBuffer=8>
class AudioDevice {
public:
    AudioDevice() :
            bufferSize(thelonious::constants::BLOCK_SIZE * blocksPerBuffer),
            inputBlockIndex(blocksPerBuffer),
            outputBlockIndex(blocksPerBuffer) {
        RtAudio::StreamParameters inputParameters;
        inputParameters.deviceId = DESKTOP_DEVICE;
        inputParameters.nChannels = inputChannels;

        RtAudio::StreamParameters outputParameters;
        outputParameters.deviceId = DESKTOP_DEVICE;
        outputParameters.nChannels = outputChannels;

        RtAudio::StreamOptions options;
        options.flags = RTAUDIO_NONINTERLEAVED;

        try {
            device.openStream(outputChannels > 0 ? &outputParameters: NULL,
                              inputChannels > 0 ? &inputParameters : NULL,
                              RTAUDIO_FLOAT32,
                              thelonious::constants::SAMPLE_RATE, &bufferSize,
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
        castDevice->inputSamples = (float *) inputSamples;
        castDevice->outputSamples = (float *) outputSamples;

        castDevice->inputBlockIndex = 0;
        castDevice->outputBlockIndex = 0;
        return 0;
    }


    Block<inputChannels> read() {
        while (inputBlockIndex == blocksPerBuffer) {
            std::chrono::milliseconds sleepTime(1);
            std::this_thread::sleep_for(sleepTime);
        }

        Block<inputChannels> block;
        for (uint32_t i=0; i<inputChannels; i++) {
            Chock chock = block[i];
            // The start of the channel in the outputSamples buffer
            float *channelStart = inputSamples + i * bufferSize;
            // The start of the block we are interested in
            float *blockStart = channelStart + inputBlockIndex *
                                thelonious::constants::BLOCK_SIZE;
            float *blockEnd = blockStart + thelonious::constants::BLOCK_SIZE;
            std::copy(blockStart, blockEnd, chock.begin());
        }

        inputBlockIndex++;
        return block;
    }

    void write(Block<outputChannels> outputBlock) {
        while (outputBlockIndex == blocksPerBuffer) {
            std::chrono::milliseconds sleepTime(1);
            std::this_thread::sleep_for(sleepTime);
        }

        for (uint32_t i=0; i<outputChannels; i++) {
            Chock chock = outputBlock[i];
            // The start of the channel in the outputSamples buffer
            float *channelStart = outputSamples + i * bufferSize;
            // The start of the block we are interested in
            float *blockStart = channelStart + outputBlockIndex *
                                thelonious::constants::BLOCK_SIZE;
            std::copy(chock.begin(), chock.end(), blockStart);
        }

        outputBlockIndex++;
    };

private:
    RtAudio device;
    uint32_t bufferSize;

    float *inputSamples;
    float *outputSamples;

    uint32_t inputBlockIndex;
    uint32_t outputBlockIndex;
};

}

#endif
