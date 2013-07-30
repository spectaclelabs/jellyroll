#ifndef JELLYROLL_I2S 
#define JELLYROLL_I2S

#include <map>

#include "mbed.h"
#include "pinmap.h"

#include "thelonious/constants/sizes.h"

namespace jellyroll {

FunctionPointer i2sirq;


typedef void (*I2SCallback) (int16_t *, uint32_t, void *data);

extern "C" void i2s_dma_irq_handler() {
    i2sirq.call();
}

typedef enum {
    I2S_2 = (int)SPI2_BASE,
    I2S_3 = (int)SPI3_BASE,
    I2SE_2 = (int)I2S2ext_BASE,
    I2SE_3 = (int)I2S3ext_BASE
} I2SName;

static const PinMap PinMap_I2S_CK[] = {
    {PB_3, I2S_3, STM_PIN_DATA(2, 6)},
    {PB_10, I2S_2, STM_PIN_DATA(2, 5)},
    {PB_13, I2S_2, STM_PIN_DATA(2, 5)},
    {PC_10, I2S_3, STM_PIN_DATA(2, 6)},
    {NC   , NC   , 0}
};

static const PinMap PinMap_I2S_SD[] = {
    {PB_5, I2S_3, STM_PIN_DATA(2, 6)},
    {PB_15, I2S_2, STM_PIN_DATA(2, 5)},
    {PC_3, I2S_2, STM_PIN_DATA(2, 5)},
    {PC_12, I2S_3, STM_PIN_DATA(2, 6)},
    {NC   , NC   , 0}
};

static const PinMap PinMap_I2S_ext_SD[] = {
    {PB_14, I2SE_2, STM_PIN_DATA(2, 6)},
    {PC_2, I2SE_2, STM_PIN_DATA(2, 6)},
    {PC_11, I2SE_3, STM_PIN_DATA(2, 5)},
    {NC   , NC   , 0}
};

static const PinMap PinMap_I2S_WS[] = {
    {PA_4, I2S_3, STM_PIN_DATA(2, 6)},
    {PA_15, I2S_3, STM_PIN_DATA(2, 6)},
    {PB_9, I2S_2, STM_PIN_DATA(2, 5)},
    {PB_12, I2S_2, STM_PIN_DATA(2, 5)},
    {NC   , NC   , STM_PIN_DATA(0, 0)}
};

static const PinMap PinMap_I2S_MCK[] = {
    {PC_6, I2S_2, STM_PIN_DATA(2, 5)},
    {PC_7, I2S_3, STM_PIN_DATA(2, 6)}, 
    {NC   , NC   , 0}
};

enum I2SMode {INPUT, OUTPUT, DUPLEX};

static const std::map<int, std::map<I2SMode, int>>
DMAChannelMap {
    {I2S_2, {
        {INPUT, 3},
        {OUTPUT, 4},
        {DUPLEX, 4}
    }},
    {I2S_3, {
        {INPUT, 2},
        {OUTPUT, 5},
        {DUPLEX, 5}
    }},
    {I2SE_2, {
        {DUPLEX, 2}
    }},
    {I2SE_3, {
        {DUPLEX, 0}
    }}
};

class I2S {
public:
    I2S(PinName sd, PinName ws, PinName ck, PinName mck, PinName ext_sd=NC) {
        I2SName i2s_sd = (I2SName) pinmap_peripheral(sd, PinMap_I2S_SD);
        I2SName i2s_ws = (I2SName) pinmap_peripheral(ws, PinMap_I2S_WS);
        I2SName i2s_ck = (I2SName) pinmap_peripheral(ck, PinMap_I2S_CK);
        I2SName i2s_mck = (I2SName) pinmap_peripheral(mck, PinMap_I2S_MCK);
        I2SName i2s_ext_sd = (I2SName) pinmap_peripheral(ext_sd,
                                                         PinMap_I2S_ext_SD);

        I2SName merged = (I2SName) pinmap_merge(i2s_sd, i2s_ws);
        merged = (I2SName) pinmap_merge(merged, i2s_ck);
        merged = (I2SName) pinmap_merge(merged, i2s_mck);



        i2s = (SPI_TypeDef *)merged;
        i2s_ext = (SPI_TypeDef *) i2s_ext_sd;
        if ((int)i2s == NC ||
            ((int) i2s_ext != NC &&  (((int)i2s == I2S_2 &&
                                      (int)i2s_ext != I2SE_2) ||
                                     ((int)i2s == I2S_3 &&
                                      (int)i2s_ext != I2SE_3)))) {
            error("I2S pinout mapping failed");
        }

        haveExt = (int) i2s_ext != NC;

        enablePowerClocking();

        // Pin out the pins
        pinmap_pinout(sd, PinMap_I2S_SD);
        pinmap_pinout(ws, PinMap_I2S_WS);
        pinmap_pinout(ck, PinMap_I2S_CK);
        pinmap_pinout(mck, PinMap_I2S_MCK);

        if (haveExt) {
            pinmap_pinout(ext_sd, PinMap_I2S_ext_SD);
        }

        i2sirq.attach(this, &I2S::handleDMAInterrupts);

        enableVector();

        setupI2S();
        setupDMA();

        enableDMA();
        enableI2S();
    }

    void start() {
        enableInterrupt();
    }

    void bitDepth(int bitDepth) {
        disableI2S();
        i2s->I2SCFGR &= ~(0x3 << 1);
        i2s->I2SCFGR |= (bitDepth / 8 - 2) << 1;

        if ((int) i2s_ext != NC) {
            i2s_ext->I2SCFGR &= ~(0x3 << 1);
            i2s_ext->I2SCFGR |= (bitDepth / 8 - 2) << 1;
        }
        enableI2S();
    }

    void frequency(uint32_t frequency) {
        disableI2S();

        uint32_t plli2s_n = (RCC->PLLI2SCFGR >> 6) & 0x1FF;
        uint32_t plli2s_r = (RCC->PLLI2SCFGR >> 28) & 0x7;
        uint32_t pll_m = RCC->PLLCFGR & RCC_PLLCFGR_PLLM;
        uint32_t clock = ((HSE_VALUE / pll_m) * plli2s_n) / plli2s_r;
        uint32_t divider = 10 * clock / (256 * frequency);
        divider /= 10;
        uint32_t odd = divider & 0x1;
        divider = divider >> 1;
        i2s->I2SPR |= divider | (odd << 8);

        enableI2S();
    }

    // Set transmit or receive move (0 = tx, 1=rx)
    void mode(int mode) {
        currentMode = (I2SMode) mode;

        disableI2S();
        enableVector();

        i2s->I2SCFGR &= ~(0x3 << 8);
        switch (currentMode) {
            case INPUT:
                i2s->I2SCFGR |= (0x3 << 8); // Master RX
                break;
            case OUTPUT:
                i2s->I2SCFGR |= (0x2 << 8); // Master TX
                break;
            case DUPLEX:
                i2s->I2SCFGR |= (0x2 << 8); // Master TX
                i2s_ext->I2SCFGR &= ~(0x3 << 8);
                i2s_ext->I2SCFGR |= (0x1 << 8); // Slave RX
                break;
        }

        enableI2S();
    }

    void setRxCallback(I2SCallback callback, void *data=nullptr) {
        this->rxCallback = callback;
        this->rxData = data;
    }

    void setTxCallback(I2SCallback callback, void *data=nullptr) {
        this->txCallback = callback;
        this->txData = data;
    }
 

private:
    void handleDMAInterrupts() {
        int channel = DMAChannelMap.at((int)i2s).at(currentMode);

        I2SCallback callback = nullptr;
        int16_t *pointer = nullptr;
        void *data = nullptr;
        

        if (halfTransfer(channel)) {
            clearHalfTransfer(channel);
            switch (currentMode) {
                case INPUT:
                    callback = rxCallback;
                    pointer = inputBuffer;
                    data = rxData;
                    break;
                case OUTPUT:
                case DUPLEX:
                    callback = txCallback;
                    pointer = outputBuffer;
                    data = txData;
                    break;
            }
        }
        else if (transferComplete(channel)) {
            clearTransferComplete(channel);
            switch (currentMode) {
                case INPUT:
                    callback = rxCallback;
                    pointer = inputBuffer +
                              2 * thelonious::constants::BLOCK_SIZE;
                    data = rxData;
                case OUTPUT:
                case DUPLEX:
                    callback = txCallback;
                    pointer = outputBuffer +
                              2 * thelonious::constants::BLOCK_SIZE;
                    data = txData;
            }
        }

        if (currentMode == DUPLEX) {
            int channel_ext = DMAChannelMap.at((int)i2s_ext).at(currentMode);
            if (halfTransfer(channel_ext)) {
                clearHalfTransfer(channel_ext);
                callback =  rxCallback;
                pointer = inputBuffer;
                data = rxData;
            }

            else if (transferComplete(channel_ext)) {
                clearTransferComplete(channel_ext);
                callback = rxCallback;
                pointer = inputBuffer + 2 * thelonious::constants::BLOCK_SIZE;
                data = rxData;
            }
        }

        if (callback != nullptr) {
            callback(pointer, thelonious::constants::BLOCK_SIZE, data);
        }
    }

    void enablePowerClocking() {
        // Enable power and clocking
        RCC->AHB1ENR |=  RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN |
                         RCC_AHB1ENR_GPIOCEN | RCC_AHB1ENR_DMA1EN;
        switch ((int) i2s) {
            case I2S_2:
                RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;
                break;
            case I2S_3:
                RCC->APB1ENR |= RCC_APB1ENR_SPI3EN;
                break;
        }
    }

    void setupI2S() {
        // Enable I2S mode
        i2s->I2SCFGR |= SPI_I2SCFGR_I2SMOD;

        // Enable master clock output
        i2s->I2SPR |= SPI_I2SPR_MCKOE;

        if (haveExt) {
            i2s_ext->I2SCFGR |= SPI_I2SCFGR_I2SMOD;

            i2s->I2SPR = 0x2;
        }

        bitDepth(16);
        frequency(44100);
        mode(haveExt ? DUPLEX : OUTPUT);
    }

    void setupDMA() {
        int channel = DMAChannelMap.at((int)i2s).at(currentMode);
        dma = dmaChannelToStream(channel);

        int direction = (currentMode == OUTPUT ||
                         currentMode == DUPLEX) ? 1 : 0;
        uint32_t buffer = direction ? (uint32_t) outputBuffer :
                                      (uint32_t) inputBuffer;

        dma->PAR = (uint32_t) &(i2s->DR);
        dma->M0AR = buffer;
        dma->NDTR = 4 * thelonious::constants::BLOCK_SIZE;
        dma->CR = (0x3 << 16) | // Very high priority
                  (0x1 << 13) | // Half-word memory data size
                  (0x1 << 11) | // Half-word peripheral data size
                  (direction << 6)  | // P2M or M2P
                  DMA_SxCR_MINC | // Increment memory address pointer
                  DMA_SxCR_CIRC | // Circular buffer
                  DMA_SxCR_TCIE | // Transfer complete interrupt
                  DMA_SxCR_HTIE;  // Transfer half-complete interrupt

        if (currentMode == DUPLEX) {
            channel = DMAChannelMap.at((int)i2s_ext).at(currentMode);
            dma_ext = dmaChannelToStream(channel);
            dma_ext->PAR = (uint32_t) &(i2s_ext->DR);
            dma_ext->M0AR = (uint32_t) inputBuffer;
            dma_ext->NDTR = 4 * thelonious::constants::BLOCK_SIZE;
            dma_ext->CR = (0x3 << 16) | // Very high priority
                          (0x1 << 13) | // Half-word memory data size
                          (0x1 << 11) | // Half-word peripheral data size
                          DMA_SxCR_MINC | // Increment memory address pointer
                          DMA_SxCR_CIRC | // Circular buffer
                          DMA_SxCR_TCIE | // Transfer complete interrupt
                          DMA_SxCR_HTIE;  // Transfer half-complete interrupt
        }
    }

    void enableI2S() {
        i2s->I2SCFGR |= SPI_I2SCFGR_I2SE;
    }

    void disableI2S() {
        i2s->I2SCFGR &= ~SPI_I2SCFGR_I2SE;
    }

    void enableDMA() {
        dma->CR |= DMA_SxCR_EN;
    }

    void disableDMA() {
        dma->CR &= ~DMA_SxCR_EN;
    }

    void enableVector() {
        int channel = DMAChannelMap.at((int)i2s).at(currentMode);
        IRQn irqn = dmaChannelToIRQn(channel);

        NVIC_SetVector(irqn, (uint32_t)i2s_dma_irq_handler);
        NVIC_EnableIRQ(irqn);

        if (currentMode == DUPLEX) {
            channel = DMAChannelMap.at((int)i2s_ext).at(currentMode);
            irqn = dmaChannelToIRQn(channel);

            NVIC_SetVector(irqn, (uint32_t)i2s_dma_irq_handler);
            NVIC_EnableIRQ(irqn);
        }
    }

    void enableInterrupt() {
        switch (currentMode) {
            case INPUT:
                i2s->CR2 |= SPI_CR2_RXDMAEN;
                break;
            case OUTPUT:
            case DUPLEX:
                i2s->CR2 |= SPI_CR2_TXDMAEN;
                break;
        }
    }

    void disableInterrupt() {
        switch (currentMode) {
            case INPUT:
                i2s->CR2 &= ~SPI_CR2_RXDMAEN;
                break;
            case OUTPUT:
            case DUPLEX:
                i2s->CR2 &= ~SPI_CR2_TXDMAEN;
                break;
        }
    }

    DMA_Stream_TypeDef* dmaChannelToStream(int channel) {
        return (DMA_Stream_TypeDef *)(DMA1_Stream0_BASE + 0x18 * channel);
    }

    IRQn dmaChannelToIRQn(int channel) {
        return static_cast<IRQn>(DMA1_Stream0_IRQn + channel);
    }


    int TCIF(int channel) {
        return 1 << (channel * 6 + (channel / 2) * 4 + 5);
    }

    int HTIF(int channel) {
        return 1 << (channel * 6 + (channel / 2) * 4 + 4);
    }

    bool transferComplete(int channel) {
        if (channel < 4) {
            return DMA1->LISR & TCIF(channel);
        }
        else if (channel < 8) {
             return DMA1->HISR & TCIF(channel - 4);
        }
        return false;
    }

    void clearTransferComplete(int channel) {
        if (channel < 4) {
            DMA1->LIFCR |= TCIF(channel);
        }
        else if (channel < 8) {
             DMA1->HIFCR |= TCIF(channel - 4);
        }   
    }

    bool halfTransfer(int channel) {
        if (channel < 4) {
            return DMA1->LISR & HTIF(channel);
        }
        else if (channel < 8) {
             return DMA1->HISR & HTIF(channel - 4);
        }
        return false;
    }

    void clearHalfTransfer(int channel) {
        if (channel < 4) {
            DMA1->LIFCR |= HTIF(channel);
        }
        else if (channel < 8) {
             DMA1->HIFCR |= HTIF(channel - 4);
        }
    }

    SPI_TypeDef *i2s;
    SPI_TypeDef *i2s_ext;
    DMA_Stream_TypeDef *dma;
    DMA_Stream_TypeDef *dma_ext;
    I2SMode currentMode;

    // Four times buffer size - two channels, and double buffered
    int16_t inputBuffer[thelonious::constants::BLOCK_SIZE * 4];
    int16_t outputBuffer[thelonious::constants::BLOCK_SIZE * 4];

    I2SCallback rxCallback = nullptr;
    I2SCallback txCallback = nullptr;

    void *rxData;
    void *txData;

    bool haveExt;
};

} // namespace jellyroll

#endif
