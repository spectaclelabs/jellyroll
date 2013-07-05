#ifndef JELLYROLL_I2S 
#define JELLYROLL_I2S

#include "mbed.h"
#include "pinmap.h"

#include "thelonious/constants/sizes.h"

FunctionPointer i2sirq;

typedef void (*I2SCallback) (int16_t *, uint32_t, void *data);

extern "C" void i2s_dma_irq_handler() {
    i2sirq.call();
}

typedef enum {
    I2S_2 = (int)SPI2_BASE,
    I2S_3 = (int)SPI3_BASE
} I2SName;

typedef enum {
    I2SE_2 = (int)I2S2ext_BASE,
    I2SE_3 = (int)I2S3ext_BASE
} I2SEName;

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

class I2S {
public:
    I2S(PinName sd, PinName ws, PinName ck, PinName mck) {
        I2SName i2s_sd = (I2SName) pinmap_peripheral(sd, PinMap_I2S_SD);
        I2SName i2s_ws = (I2SName) pinmap_peripheral(ws, PinMap_I2S_WS);
        I2SName i2s_ck = (I2SName) pinmap_peripheral(ck, PinMap_I2S_CK);
        I2SName i2s_mck = (I2SName) pinmap_peripheral(mck, PinMap_I2S_MCK);

        I2SName merged = (I2SName) pinmap_merge(i2s_sd, i2s_ws);
        merged = (I2SName) pinmap_merge(merged, i2s_ck);
        merged = (I2SName) pinmap_merge(merged, i2s_mck);

        i2s = (SPI_TypeDef *)merged;
        if ((int)i2s == NC) {
            error("I2S pinout mapping failed");
        }


        enablePowerClocking();

        // Pin out the pins
        pinmap_pinout(sd, PinMap_I2S_SD);
        pinmap_pinout(ws, PinMap_I2S_WS);
        pinmap_pinout(ck, PinMap_I2S_CK);
        pinmap_pinout(mck, PinMap_I2S_MCK);

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
        switch (bitDepth) {
            case 16: i2s->I2SCFGR |= (0x0 << 1); break;
            case 24: i2s->I2SCFGR |= (0x1 << 1); break;
            case 32: i2s->I2SCFGR |= (0x2 << 1); break;
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
        currentMode = mode;

        disableI2S();

        i2s->I2SCFGR &= ~(0x3 << 8);
        switch (mode) {
            case 0: i2s->I2SCFGR |= (0x2 << 8); break; // Master TX
            case 1: i2s->I2SCFGR |= (0x3 << 8); break; // Master RX
        }

        enableI2S();
    }

    void setCallback(I2SCallback callback, void *data=nullptr) {
        this->callback = callback;
        this->data = data;
    }

private:
    void handleDMAInterrupts() {
        if ((int) i2s == I2S_2) {
            if (DMA1->HISR & DMA_HISR_HTIF4) {
                // Half done
                // Clear the interrupt
                DMA1->HIFCR |= DMA_HIFCR_CHTIF4;
                pointer = buffer;
            }
            if (DMA1->HISR & DMA_HISR_TCIF4) {
                // Completely done
                // Clear the interrupt
                DMA1->HIFCR |= DMA_HIFCR_CTCIF4;
                pointer = buffer + 2 * thelonious::constants::BLOCK_SIZE;
            }
        }
        else if ((int) i2s == I2S_3) {
            if (DMA1->HISR & DMA_HISR_HTIF5) {
                // Half done
                // Clear the interrupt
                DMA1->HIFCR |= DMA_HIFCR_CHTIF5;
                pointer = buffer;
            }
            if (DMA1->HISR & DMA_HISR_TCIF5) {
                // Completely done
                // Clear the interrupt  
                DMA1->HIFCR |= DMA_HIFCR_CTCIF5;
                pointer = buffer + 2 * thelonious::constants::BLOCK_SIZE;
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

        bitDepth(16);
        frequency(44100);
        mode(0);
    }

    void setupDMA() {
        if ((int) i2s == I2S_2) {
            switch (currentMode) {
                case 0: dma = DMA1_Stream4; break;
                case 1: dma = DMA1_Stream3; break;
            }
        }
        else if ((int) i2s == I2S_3) {
            switch (currentMode) {
                case 0: dma = DMA1_Stream5; break;
                case 1: dma = DMA1_Stream2; break;
            }  
        }

        dma->PAR = (uint32_t) &(i2s->DR);
        dma->M0AR = (uint32_t) buffer;
        dma->NDTR = 4 * thelonious::constants::BLOCK_SIZE;
        dma->CR |= (0x3 << 16) | // Very high priority
                   (0x1 << 13) | // Half-word memory data size
                   (0x1 << 11) | // Half-word peripheral data size
                   (0x1 << 6)  | // Memory to peripheral
                   DMA_SxCR_MINC | // Increment memory address pointer
                   DMA_SxCR_CIRC | // Circular buffer
                   DMA_SxCR_TCIE | // Transfer complete interrupt
                   DMA_SxCR_HTIE;  // Transfer half-complete interrupt
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
        switch ((int) i2s) {
            case I2S_2:
                NVIC_SetVector(DMA1_Stream4_IRQn,
                               (uint32_t)i2s_dma_irq_handler);
                NVIC_EnableIRQ(DMA1_Stream4_IRQn);
                break;
            case I2S_3:
                NVIC_SetVector(DMA1_Stream5_IRQn,
                               (uint32_t)i2s_dma_irq_handler);
                NVIC_EnableIRQ(DMA1_Stream5_IRQn);
                break;
        }
    }

    void enableInterrupt() {
        if (currentMode == 0) {
            // Transmit
            i2s->CR2 |= SPI_CR2_TXDMAEN;
        }
        else {
            // Receive
            i2s->CR2 |= SPI_CR2_RXDMAEN;
        }
    }

    void disableInterrupt() {
        if (currentMode == 0) {
            // Transmit
            i2s->CR2 &= ~SPI_CR2_TXDMAEN;
        }   
        else {
            // Receive
            i2s->CR2 &= ~SPI_CR2_RXDMAEN;
        }
    }

    SPI_TypeDef *i2s;
    DMA_Stream_TypeDef *dma;
    int currentMode;

    // Four times buffer size - two channels, and double buffered
    int16_t buffer[thelonious::constants::BLOCK_SIZE * 4];
    int16_t *pointer;

    I2SCallback callback = nullptr;
    void *data;
};

#endif
