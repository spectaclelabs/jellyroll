#ifndef AUDIO_DEVICE_H
#define AUDIO_DEVICE_H

#ifdef TARGET_LPC1768
#include "audio_device.lpc1768.h"
#endif

#ifdef TARGET_DESKTOP
#include "audio_device.desktop.h"
#endif

#endif
