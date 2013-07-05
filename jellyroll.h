#ifdef DEVICE_BABADOO
#include "jellyroll/device.babadoo.h"
#elif DEVICE_STM32F4DISCOVERY
#include "jellyroll/device.stm32f4discovery.h"
#elif DEVICE_LPCXPRESSO_1769
#include "jellyroll/device.lpcxpresso_lpc1769.h"
#elif DEVICE_DESKTOP
#include "jellyroll/device.desktop.h"
#endif

namespace jellyroll {

typedef BabadooN<1> Babadoo;

} // namespace jellyroll
