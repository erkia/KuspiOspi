#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

#include <stm32l4xx_hal.h>

static const char *HAL_StatusToString(HAL_StatusTypeDef status)
{
    switch (status) {
        case HAL_OK: return "HAL_OK";
        case HAL_ERROR: return "HAL_ERROR";
        case HAL_BUSY: return "HAL_BUSY";
        case HAL_TIMEOUT: return "HAL_TIMEOUT";
        default: return "UNKNOWN";
    }
}

static const char *OCTOSPI_ErrorToString(unsigned long int errcode)
{
    switch (errcode) {
        case HAL_OSPI_ERROR_NONE: return "HAL_OSPI_ERROR_NONE";
        case HAL_OSPI_ERROR_TIMEOUT: return "HAL_OSPI_ERROR_TIMEOUT";
        case HAL_OSPI_ERROR_TRANSFER: return "HAL_OSPI_ERROR_TRANSFER";
        case HAL_OSPI_ERROR_DMA: return "HAL_OSPI_ERROR_DMA";
        case HAL_OSPI_ERROR_INVALID_PARAM: return "HAL_OSPI_ERROR_INVALID_PARAM";
        case HAL_OSPI_ERROR_INVALID_SEQUENCE: return "HAL_OSPI_ERROR_INVALID_SEQUENCE";
        #if defined (HAL_OSPI_ERROR_INVALID_CALLBACK)
        case HAL_OSPI_ERROR_INVALID_CALLBACK: return "HAL_OSPI_ERROR_INVALID_CALLBACK";
        #endif
        default: return "UNKNOWN";
    }
}

void SysTick_Handler(void)
{
    HAL_IncTick();
}

void HAL_MspInit(void)
{
    __HAL_RCC_SYSCFG_CLK_ENABLE();

    HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);

    // MemoryManagement_IRQn interrupt configuration
    HAL_NVIC_SetPriority(MemoryManagement_IRQn, 0U, 0U);

    // BusFault_IRQn interrupt configuration
    HAL_NVIC_SetPriority(BusFault_IRQn, 0U, 0U);

    // UsageFault_IRQn interrupt configuration
    HAL_NVIC_SetPriority(UsageFault_IRQn, 0U, 0U);

    // DebugMonitor_IRQn interrupt configuration
    HAL_NVIC_SetPriority(DebugMonitor_IRQn, 0U, 0U);

    // SysTick_IRQn interrupt configuration
    HAL_NVIC_SetPriority(SysTick_IRQn, 0U, 0U);
}

void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    if (huart->Instance == USART1) {

        // Peripheral Clock Enable
        __HAL_RCC_USART1_CLK_ENABLE();

        // Configure GPIO pins (USART1 RX)
        __HAL_RCC_GPIOA_CLK_ENABLE();
        memset(&GPIO_InitStruct, 0, sizeof(GPIO_InitStruct));
        GPIO_InitStruct.Pin = GPIO_PIN_10;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        // Configure GPIO pins (USART1 TX)
        __HAL_RCC_GPIOB_CLK_ENABLE();
        memset(&GPIO_InitStruct, 0, sizeof(GPIO_InitStruct));
        GPIO_InitStruct.Pin = GPIO_PIN_6;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {

        // GPIO DeInit (USART1 TX)
        HAL_GPIO_DeInit(GPIOB, GPIO_PIN_6);
        __HAL_RCC_GPIOB_CLK_DISABLE();

        // GPIO DeInit (USART1 EX)
        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_10);
        __HAL_RCC_GPIOA_CLK_DISABLE();

        // Peripheral clock disable
        __HAL_RCC_USART1_CLK_DISABLE();

    }
}

static UART_HandleTypeDef DbgUartHandleData;
static UART_HandleTypeDef *DbgUartHandle = &DbgUartHandleData;
static uint8_t DbgUartBuf[8];

static int DBGUART_Init(void)
{
    memset(DbgUartHandle, 0, sizeof(*DbgUartHandle));
    DbgUartHandle->Instance = USART1;
    DbgUartHandle->Init.BaudRate = 115200;
    DbgUartHandle->Init.WordLength = UART_WORDLENGTH_8B;
    DbgUartHandle->Init.StopBits = UART_STOPBITS_1;
    DbgUartHandle->Init.Parity = UART_PARITY_NONE;
    DbgUartHandle->Init.HwFlowCtl = UART_HWCONTROL_NONE;
    DbgUartHandle->Init.Mode = UART_MODE_RX | UART_MODE_TX;
    DbgUartHandle->AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_RXOVERRUNDISABLE_INIT;
    DbgUartHandle->AdvancedInit.OverrunDisable = UART_ADVFEATURE_OVERRUN_DISABLE;

    if (HAL_UART_Init(DbgUartHandle) == HAL_OK) {
        UART_Start_Receive_IT(DbgUartHandle, DbgUartBuf, sizeof(DbgUartBuf));
        HAL_NVIC_SetPriority(USART1_IRQn, 0x0F, 0);
        HAL_NVIC_EnableIRQ(USART1_IRQn);
        return 0;
    }

    return -1;
}

void USART1_IRQHandler(void)
{
    HAL_UART_IRQHandler(DbgUartHandle);
}

int __io_putchar(int ch)
{
    uint8_t ch8 = ch;
    if (HAL_UART_Transmit(DbgUartHandle, &ch8, sizeof(ch8), -1) == HAL_OK) {
        return 1;
    } else {
        return -1;
    }
}

int __io_getchar(int ch)
{
    uint8_t ch8;
    if (HAL_UART_Receive(DbgUartHandle, &ch8, 1, -1) == HAL_OK) {
        return ch8;
    } else {
        return -1;
    }
}

#define EXTERNAL_FLASH_ADDRESS      0x90000000

#define WRITE_ENABLE_CMD            0x06
#define READ_STATUS_REG_CMD         0x05
#define READ_STATUS_REG2_CMD        0x35
#define READ_STATUS_REG3_CMD        0x15
#define WRITE_STATUS_REG_CMD        0x01
#define WRITE_STATUS_REG2_CMD       0x31
#define WRITE_STATUS_REG3_CMD       0x11

#define BLOCK_4K_ERASE_CMD          0x20    //  4k (16 pages)
#define BLOCK_32K_ERASE_CMD         0x52    // 32k (64 pages)
#define BLOCK_64K_ERASE_CMD         0xD8    // 64k (128 pages)
#define CHIP_ERASE_CMD              0xC7

#define READ_CMD                    0x03
#define FAST_READ_CMD               0x0B
#define QUAD_OUT_FAST_READ_CMD      0x6B
#define PAGE_PROG_CMD               0x02
#define QUAD_IN_FAST_PROG_CMD       0x32

#define MX25L3233_SR_WIP            ((uint8_t)(1 << 0))     // Ready/Busy Status
#define MX25L3233_SR_WREN           ((uint8_t)(1 << 1))     // Write Enable Latch
#define MX25L3233_SR_BLOCKPR        ((uint8_t)(0x0F << 2))  // Block protected against program and erase operations
#define MX25L3233_SR_QUADEN         ((uint8_t)(1 << 6))     // Quad Enable
#define MX25L3233_SR_SRP0           ((uint8_t)(1 << 7))     // Status register write enable/disable

static OSPI_HandleTypeDef OctoSpiHandleData;
static OSPI_HandleTypeDef *OctoSpiHandle = &OctoSpiHandleData;
static DMA_HandleTypeDef OctoSpiHdma;
static volatile int OctoRxCplt = 0;
static volatile int OctoTxCplt = 0;

void HAL_OSPI_MspInit(OSPI_HandleTypeDef *hospi)
{
    RCC_PeriphCLKInitTypeDef PeriphClkInit;
    GPIO_InitTypeDef GPIO_InitStruct;
    HAL_StatusTypeDef status;

    if (hospi->Instance == OCTOSPI1) {

        //Initializes the peripherals clock
        memset(&PeriphClkInit, 0, sizeof(PeriphClkInit));
        PeriphClkInit.PeriphClockSelection  = RCC_PERIPHCLK_OSPI;
        PeriphClkInit.OspiClockSelection    = RCC_OSPICLKSOURCE_SYSCLK;
        status = HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit);
        if (status != HAL_OK) {
            printf("HAL_RCCEx_PeriphCLKConfig() failed: status = %s\r\n", HAL_StatusToString(status));
            while (1);
        }

        // Peripheral clock enable
        __HAL_RCC_OSPIM_CLK_ENABLE();
        __HAL_RCC_OSPI1_CLK_ENABLE();

        // Configure GPIOA pins (OSPI1 D2, D3)
        __HAL_RCC_GPIOA_CLK_ENABLE();
        memset(&GPIO_InitStruct, 0, sizeof(GPIO_InitStruct));
        GPIO_InitStruct.Pin         = GPIO_PIN_7 | GPIO_PIN_6;
        GPIO_InitStruct.Mode        = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull        = GPIO_NOPULL;
        GPIO_InitStruct.Speed       = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate   = GPIO_AF10_OCTOSPIM_P1;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        // Configure GPIOB pins (OSPI1 D1, D2, CLK, NCS)
        __HAL_RCC_GPIOB_CLK_ENABLE();
        memset(&GPIO_InitStruct, 0, sizeof(GPIO_InitStruct));
        GPIO_InitStruct.Pin         = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_10 | GPIO_PIN_11;
        GPIO_InitStruct.Mode        = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull        = GPIO_NOPULL;
        GPIO_InitStruct.Speed       = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate   = GPIO_AF10_OCTOSPIM_P1;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

        // OCTOSPI1 DMA Init
        __HAL_RCC_DMAMUX1_CLK_ENABLE();
        __HAL_RCC_DMA1_CLK_ENABLE();

        memset(&OctoSpiHdma, 0, sizeof(OctoSpiHdma));
        __HAL_LINKDMA(hospi, hdma, OctoSpiHdma);
        hospi->hdma->Instance               = DMA1_Channel2;
        hospi->hdma->Init.Request           = DMA_REQUEST_OCTOSPI1;
        hospi->hdma->Init.Direction         = DMA_PERIPH_TO_MEMORY;
        hospi->hdma->Init.PeriphInc         = DMA_PINC_DISABLE;
        hospi->hdma->Init.MemInc            = DMA_MINC_ENABLE;
        hospi->hdma->Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hospi->hdma->Init.MemDataAlignment  = DMA_MDATAALIGN_BYTE;
        hospi->hdma->Init.Mode              = DMA_NORMAL;
        hospi->hdma->Init.Priority          = DMA_PRIORITY_LOW;
        status = HAL_DMA_Init(hospi->hdma);
        if (status != HAL_OK) {
            printf("HAL_DMA_Init() failed: status = %s\r\n", HAL_StatusToString(status));
            while (1);
        }

        HAL_NVIC_SetPriority(DMA1_Channel2_IRQn, 0x0F, 0);
        HAL_NVIC_EnableIRQ(DMA1_Channel2_IRQn);

        // OCTOSPI1 interrupt Init
        HAL_NVIC_SetPriority(OCTOSPI1_IRQn, 0x0F, 0);
        HAL_NVIC_EnableIRQ(OCTOSPI1_IRQn);
    }
}

void HAL_OSPI_MspDeInit(OSPI_HandleTypeDef *hospi)
{
    HAL_NVIC_DisableIRQ(OCTOSPI1_IRQn);
    HAL_NVIC_DisableIRQ(DMA1_Channel2_IRQn);
    __HAL_RCC_DMA1_CLK_DISABLE();
    __HAL_RCC_DMAMUX1_CLK_DISABLE();
    __HAL_RCC_OSPI1_CLK_DISABLE();
    __HAL_RCC_OSPIM_CLK_DISABLE();
}

static int OCTOSPI_Init(OSPI_HandleTypeDef *hospi)
{
    OSPIM_CfgTypeDef OSPIM_Cfg_Struct;
    HAL_StatusTypeDef status;

    memset(hospi, 0, sizeof(*hospi));
    hospi->Instance                 = OCTOSPI1;
    hospi->Init.FifoThreshold       = 1;
    hospi->Init.DualQuad            = HAL_OSPI_DUALQUAD_DISABLE;
    hospi->Init.MemoryType          = HAL_OSPI_MEMTYPE_MACRONIX;
    hospi->Init.DeviceSize          = 32;
    hospi->Init.ChipSelectHighTime  = 1;
    hospi->Init.FreeRunningClock    = HAL_OSPI_FREERUNCLK_DISABLE;
    hospi->Init.ClockMode           = HAL_OSPI_CLOCK_MODE_0;
    hospi->Init.ClockPrescaler      = 1;
    hospi->Init.SampleShifting      = HAL_OSPI_SAMPLE_SHIFTING_NONE;
    hospi->Init.DelayHoldQuarterCycle = HAL_OSPI_DHQC_DISABLE;
    hospi->Init.ChipSelectBoundary  = 0;
    hospi->Init.DelayBlockBypass    = HAL_OSPI_DELAY_BLOCK_BYPASSED;
    hospi->Init.Refresh             = 0;

    status = HAL_OSPI_Init(hospi);
    if (status != HAL_OK) {
        printf("HAL_OSPI_Init() failed: status = %s\r\n", HAL_StatusToString(status));
        return -1;
    }

    memset(&OSPIM_Cfg_Struct, 0, sizeof(OSPIM_Cfg_Struct));
    OSPIM_Cfg_Struct.ClkPort = 1;
    OSPIM_Cfg_Struct.NCSPort = 1;
    OSPIM_Cfg_Struct.IOLowPort = HAL_OSPIM_IOPORT_1_LOW;

    status = HAL_OSPIM_Config(hospi, &OSPIM_Cfg_Struct, HAL_OSPI_TIMEOUT_DEFAULT_VALUE);
    if (status != HAL_OK) {
        printf("HAL_OSPIM_Config() failed: status = %s\r\n", HAL_StatusToString(status));
        HAL_OSPI_DeInit(hospi);
        return -1;
    }

    return 0;
}

static int OCTOSPI_WriteEnable(OSPI_HandleTypeDef *hospi, uint32_t timeout)
{
    OSPI_RegularCmdTypeDef cmd;
    OSPI_AutoPollingTypeDef pollconf;
    HAL_StatusTypeDef status;

    memset(&cmd, 0, sizeof(cmd));
    cmd.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
    cmd.Instruction     = WRITE_ENABLE_CMD;
    cmd.AddressMode     = HAL_OSPI_ADDRESS_NONE;
    cmd.DataMode        = HAL_OSPI_ADDRESS_NONE;

    status = HAL_OSPI_Command(hospi, &cmd, timeout);
    if (status != HAL_OK) {
        printf("HAL_OSPI_Command() failed: status = %s, error = %s\r\n", HAL_StatusToString(status), OCTOSPI_ErrorToString(hospi->ErrorCode));
        return -1;
    }

    memset(&cmd, 0, sizeof(cmd));
    cmd.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
    cmd.Instruction     = READ_STATUS_REG_CMD;
    cmd.AddressMode     = HAL_OSPI_ADDRESS_NONE;
    cmd.DataMode        = HAL_OSPI_DATA_1_LINE;
    cmd.NbData          = 1;

    status = HAL_OSPI_Command(hospi, &cmd, timeout);
    if (status != HAL_OK) {
        printf("HAL_OSPI_Command() failed: status = %s, error = %s\r\n", HAL_StatusToString(status), OCTOSPI_ErrorToString(hospi->ErrorCode));
        return -1;
    }

    // Configure automatic polling mode to wait for write enabling
    memset(&pollconf, 0, sizeof(pollconf));
    pollconf.Match           = MX25L3233_SR_WREN;
    pollconf.Mask            = MX25L3233_SR_WREN;
    pollconf.MatchMode       = HAL_OSPI_MATCH_MODE_AND;
    pollconf.AutomaticStop   = HAL_OSPI_AUTOMATIC_STOP_ENABLE;
    pollconf.Interval        = 0x10;

    status = HAL_OSPI_AutoPolling(hospi, &pollconf, timeout);
    if (status != HAL_OK) {
        printf("HAL_OSPI_AutoPolling() failed: status = %s, error = %s\r\n", HAL_StatusToString(status), OCTOSPI_ErrorToString(hospi->ErrorCode));
        return -1;
    }

    return 0;
}

static int OCTOSPI_WhileBusy(OSPI_HandleTypeDef *hospi, uint32_t timeout)
{
    OSPI_RegularCmdTypeDef cmd;
    OSPI_AutoPollingTypeDef pollconf;
    HAL_StatusTypeDef status;

    memset(&cmd, 0, sizeof(cmd));
    cmd.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
    cmd.Instruction     = READ_STATUS_REG_CMD;
    cmd.AddressMode     = HAL_OSPI_ADDRESS_NONE;
    cmd.DataMode        = HAL_OSPI_DATA_1_LINE;
    cmd.NbData          = 1;

    status = HAL_OSPI_Command(hospi, &cmd, timeout);
    if (status != HAL_OK) {
        printf("HAL_OSPI_Command() failed: status = %s, error = %s\r\n", HAL_StatusToString(status), OCTOSPI_ErrorToString(hospi->ErrorCode));
        return -1;
    }

    // Configure automatic polling mode to wait for WIP clear
    memset(&pollconf, 0, sizeof(pollconf));
    pollconf.Match           = 0;
    pollconf.Mask            = MX25L3233_SR_WIP;
    pollconf.MatchMode       = HAL_OSPI_MATCH_MODE_AND;
    pollconf.AutomaticStop   = HAL_OSPI_AUTOMATIC_STOP_ENABLE;
    pollconf.Interval        = 0x10;

    status = HAL_OSPI_AutoPolling(hospi, &pollconf, timeout);
    if (status != HAL_OK) {
        printf("HAL_OSPI_AutoPolling() failed: status = %s, error = %s\r\n", HAL_StatusToString(status), OCTOSPI_ErrorToString(hospi->ErrorCode));
        return -1;
    }

    return 0;
}

static int OCTOSPI_Setup(OSPI_HandleTypeDef *hospi)
{
    OSPI_RegularCmdTypeDef cmd;
    HAL_StatusTypeDef status;
    int rc;
    uint8_t old_val = 0;
    uint8_t new_val = 0;

    memset(&cmd, 0, sizeof(cmd));
    cmd.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
    cmd.Instruction     = READ_STATUS_REG_CMD;
    cmd.AddressMode     = HAL_OSPI_ADDRESS_NONE;
    cmd.DataMode        = HAL_OSPI_DATA_1_LINE;
    cmd.NbData          = 1;

    status = HAL_OSPI_Command(hospi, &cmd, HAL_OSPI_TIMEOUT_DEFAULT_VALUE);
    if (status != HAL_OK) {
        printf("HAL_OSPI_Command() failed: status = %s, error = %s\r\n", HAL_StatusToString(status), OCTOSPI_ErrorToString(hospi->ErrorCode));
        return -1;
    }

    status = HAL_OSPI_Receive(hospi, &old_val, HAL_OSPI_TIMEOUT_DEFAULT_VALUE);
    if (status != HAL_OK) {
        printf("HAL_OSPI_Receive() failed: status = %s, error = %s\r\n", HAL_StatusToString(status), OCTOSPI_ErrorToString(hospi->ErrorCode));
        return -1;
    }

    printf("OLD SR = 0x%02X\r\n", old_val);

    if (!(old_val & MX25L3233_SR_QUADEN)) {

        rc = OCTOSPI_WriteEnable(hospi, HAL_OSPI_TIMEOUT_DEFAULT_VALUE);
        if (rc < 0) {
            return rc;
        }

        new_val = old_val | MX25L3233_SR_QUADEN;

        // Write status register
        memset(&cmd, 0, sizeof(cmd));
        cmd.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
        cmd.Instruction     = WRITE_STATUS_REG_CMD;
        cmd.AddressMode     = HAL_OSPI_ADDRESS_NONE;
        cmd.DataMode        = HAL_OSPI_DATA_1_LINE;
        cmd.NbData          = 1;

        status = HAL_OSPI_Command(hospi, &cmd, HAL_OSPI_TIMEOUT_DEFAULT_VALUE);
        if (status != HAL_OK) {
            printf("HAL_OSPI_Command() failed: status = %s, error = %s\r\n", HAL_StatusToString(status), OCTOSPI_ErrorToString(hospi->ErrorCode));
            return -1;
        }

        status = HAL_OSPI_Transmit(hospi, &new_val, HAL_OSPI_TIMEOUT_DEFAULT_VALUE);
        if (status != HAL_OK) {
            printf("HAL_OSPI_Transmit() failed: status = %s, error = %s\r\n", HAL_StatusToString(status), OCTOSPI_ErrorToString(hospi->ErrorCode));
            return -1;
        }

        rc = OCTOSPI_WhileBusy(hospi, HAL_OSPI_TIMEOUT_DEFAULT_VALUE);
        if (rc < 0) {
            return rc;
        }

        memset(&cmd, 0, sizeof(cmd));
        cmd.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
        cmd.Instruction     = READ_STATUS_REG_CMD;
        cmd.AddressMode     = HAL_OSPI_ADDRESS_NONE;
        cmd.DataMode        = HAL_OSPI_DATA_1_LINE;
        cmd.NbData          = 1;

        status = HAL_OSPI_Command(hospi, &cmd, HAL_OSPI_TIMEOUT_DEFAULT_VALUE);
        if (status != HAL_OK) {
            printf("HAL_OSPI_Command() failed: status = %s, error = %s\r\n", HAL_StatusToString(status), OCTOSPI_ErrorToString(hospi->ErrorCode));
            return -1;
        }

        status = HAL_OSPI_Receive(hospi, &new_val, HAL_OSPI_TIMEOUT_DEFAULT_VALUE);
        if (status != HAL_OK) {
            printf("HAL_OSPI_Receive() failed: status = %s, error = %s\r\n", HAL_StatusToString(status), OCTOSPI_ErrorToString(hospi->ErrorCode));
            return -1;
        }

        printf("NEW SR = 0x%02X\r\n", new_val);

    }

    return 0;
}

int OCTOSPI_Erase4K(OSPI_HandleTypeDef *hospi, void *dst)
{
    OSPI_RegularCmdTypeDef cmd;
    HAL_StatusTypeDef status;
    int rc;

    rc = OCTOSPI_WriteEnable(hospi, HAL_OSPI_TIMEOUT_DEFAULT_VALUE);
    if (rc < 0) {
        return rc;
    }

    memset(&cmd, 0, sizeof(cmd));
    cmd.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
    cmd.Instruction     = BLOCK_4K_ERASE_CMD;
    cmd.AddressMode     = HAL_OSPI_ADDRESS_1_LINE;
    cmd.AddressSize     = HAL_OSPI_ADDRESS_24_BITS;
    cmd.Address         = (uintptr_t)dst - EXTERNAL_FLASH_ADDRESS;
    cmd.DataMode        = HAL_OSPI_DATA_NONE;

    status = HAL_OSPI_Command(hospi, &cmd, HAL_OSPI_TIMEOUT_DEFAULT_VALUE);
    if (status != HAL_OK) {
        printf("HAL_OSPI_Command() failed: status = %s, error = %s\r\n", HAL_StatusToString(status), OCTOSPI_ErrorToString(hospi->ErrorCode));
        return -1;
    }

    rc = OCTOSPI_WhileBusy(hospi, HAL_OSPI_TIMEOUT_DEFAULT_VALUE);
    if (rc < 0) {
        return rc;
    }

    return 4096;
}

int OCTOSPI_MassErase(OSPI_HandleTypeDef *hospi)
{
    OSPI_RegularCmdTypeDef cmd;
    HAL_StatusTypeDef status;
    int rc;

    rc = OCTOSPI_WriteEnable(hospi, HAL_OSPI_TIMEOUT_DEFAULT_VALUE);
    if (rc < 0) {
        return rc;
    }

    memset(&cmd, 0, sizeof(cmd));
    cmd.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
    cmd.Instruction     = CHIP_ERASE_CMD;
    cmd.AddressMode     = HAL_OSPI_ADDRESS_NONE;
    cmd.DataMode        = HAL_OSPI_DATA_NONE;

    status = HAL_OSPI_Command(hospi, &cmd, HAL_OSPI_TIMEOUT_DEFAULT_VALUE);
    if (status != HAL_OK) {
        printf("HAL_OSPI_Command() failed: status = %s, error = %s\r\n", HAL_StatusToString(status), OCTOSPI_ErrorToString(hospi->ErrorCode));
        return -1;
    }

    rc = OCTOSPI_WhileBusy(hospi, 30000);
    if (rc < 0) {
        return rc;
    }

    return 4096;
}

int OCTOSPI_Read(OSPI_HandleTypeDef *hospi, void *dst, void *src, size_t len)
{
    OSPI_RegularCmdTypeDef cmd;
    HAL_StatusTypeDef status;

    memset(&cmd, 0, sizeof(cmd));
    cmd.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
    cmd.Instruction     = READ_CMD;
    cmd.AddressMode     = HAL_OSPI_ADDRESS_1_LINE;
    cmd.AddressSize     = HAL_OSPI_ADDRESS_24_BITS;
    cmd.Address         = (uintptr_t)src - EXTERNAL_FLASH_ADDRESS;
    cmd.DataMode        = HAL_OSPI_DATA_1_LINE;
    cmd.NbData          = len;
    cmd.DummyCycles     = 0;

    status = HAL_OSPI_Command(hospi, &cmd, HAL_OSPI_TIMEOUT_DEFAULT_VALUE);
    if (status != HAL_OK) {
        printf("HAL_OSPI_Command() failed: status = %s, error = %s\r\n", HAL_StatusToString(status), OCTOSPI_ErrorToString(hospi->ErrorCode));
        return -1;
    }

    status = HAL_OSPI_Receive(hospi, dst, HAL_OSPI_TIMEOUT_DEFAULT_VALUE);
    if (status != HAL_OK) {
        printf("HAL_OSPI_Receive() failed: status = %s, error = %s\r\n", HAL_StatusToString(status), OCTOSPI_ErrorToString(hospi->ErrorCode));
        return -1;
    }

    return len;
}

int OCTOSPI_QuadRead(OSPI_HandleTypeDef *hospi, void *dst, void *src, size_t len)
{
    OSPI_RegularCmdTypeDef cmd;
    HAL_StatusTypeDef status;

    memset(&cmd, 0, sizeof(cmd));
    cmd.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
    cmd.Instruction     = QUAD_OUT_FAST_READ_CMD;
    cmd.AddressMode     = HAL_OSPI_ADDRESS_1_LINE;
    cmd.AddressSize     = HAL_OSPI_ADDRESS_24_BITS;
    cmd.Address         = (uintptr_t)src - EXTERNAL_FLASH_ADDRESS;
    cmd.DataMode        = HAL_OSPI_DATA_4_LINES;
    cmd.NbData          = len;
    cmd.DummyCycles     = 8;

    status = HAL_OSPI_Command(hospi, &cmd, HAL_OSPI_TIMEOUT_DEFAULT_VALUE);
    if (status != HAL_OK) {
        printf("HAL_OSPI_Command() failed: status = %s, error = %s\r\n", HAL_StatusToString(status), OCTOSPI_ErrorToString(hospi->ErrorCode));
        return -1;
    }

    status = HAL_OSPI_Receive(hospi, dst, HAL_OSPI_TIMEOUT_DEFAULT_VALUE);
    if (status != HAL_OK) {
        printf("HAL_OSPI_Receive() failed: status = %s, error = %s\r\n", HAL_StatusToString(status), OCTOSPI_ErrorToString(hospi->ErrorCode));
        return -1;
    }

    return len;
}

int OCTOSPI_QuadReadDMA(OSPI_HandleTypeDef *hospi, void *dst, void *src, size_t len)
{
    OSPI_RegularCmdTypeDef cmd;
    HAL_StatusTypeDef status;

    memset(&cmd, 0, sizeof(cmd));
    cmd.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
    cmd.Instruction     = QUAD_OUT_FAST_READ_CMD;
    cmd.AddressMode     = HAL_OSPI_ADDRESS_1_LINE;
    cmd.AddressSize     = HAL_OSPI_ADDRESS_24_BITS;
    cmd.Address         = (uintptr_t)src - EXTERNAL_FLASH_ADDRESS;
    cmd.DataMode        = HAL_OSPI_DATA_4_LINES;
    cmd.NbData          = len;
    cmd.DummyCycles     = 8;

    status = HAL_OSPI_Command(hospi, &cmd, HAL_OSPI_TIMEOUT_DEFAULT_VALUE);
    if (status != HAL_OK) {
        printf("HAL_OSPI_Command() failed: status = %s, error = %s\r\n", HAL_StatusToString(status), OCTOSPI_ErrorToString(hospi->ErrorCode));
        return -1;
    }

    OctoRxCplt = 0;

    status = HAL_OSPI_Receive_DMA(hospi, dst);
    if (status != HAL_OK) {
        printf("HAL_OSPI_Receive_DMA() failed: status = %s, error = %s\r\n", HAL_StatusToString(status), OCTOSPI_ErrorToString(hospi->ErrorCode));
        return -1;
    }

    while (!OctoRxCplt);

    return len;
}

int OCTOSPI_Write(OSPI_HandleTypeDef *hospi, void *dst, void *src, size_t len)
{
    OSPI_RegularCmdTypeDef cmd;
    HAL_StatusTypeDef status;
    int rc;

    rc = OCTOSPI_WriteEnable(hospi, HAL_OSPI_TIMEOUT_DEFAULT_VALUE);
    if (rc < 0) {
        return rc;
    }

    memset(&cmd, 0, sizeof(cmd));
    cmd.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
    cmd.Instruction     = PAGE_PROG_CMD;
    cmd.AddressMode     = HAL_OSPI_ADDRESS_1_LINE;
    cmd.AddressSize     = HAL_OSPI_ADDRESS_24_BITS;
    cmd.Address         = (uintptr_t)dst - EXTERNAL_FLASH_ADDRESS;
    cmd.DataMode        = HAL_OSPI_DATA_1_LINE;
    cmd.NbData          = len;

    status = HAL_OSPI_Command(hospi, &cmd, HAL_OSPI_TIMEOUT_DEFAULT_VALUE);
    if (status != HAL_OK) {
        printf("HAL_OSPI_Command() failed: status = %s, error = %s\r\n", HAL_StatusToString(status), OCTOSPI_ErrorToString(hospi->ErrorCode));
        return -1;
    }

    status = HAL_OSPI_Transmit(hospi, src, HAL_OSPI_TIMEOUT_DEFAULT_VALUE);
    if (status != HAL_OK) {
        printf("HAL_OSPI_Transmit() failed: status = %s, error = %s\r\n", HAL_StatusToString(status), OCTOSPI_ErrorToString(hospi->ErrorCode));
        return -1;
    }

    rc = OCTOSPI_WhileBusy(hospi, HAL_OSPI_TIMEOUT_DEFAULT_VALUE);
    if (rc < 0) {
        return rc;
    }

    return len;
}

int OCTOSPI_QuadWrite(OSPI_HandleTypeDef *hospi, void *dst, void *src, size_t len)
{
    OSPI_RegularCmdTypeDef cmd;
    HAL_StatusTypeDef status;
    int rc;

    rc = OCTOSPI_WriteEnable(hospi, HAL_OSPI_TIMEOUT_DEFAULT_VALUE);
    if (rc < 0) {
        return rc;
    }

    memset(&cmd, 0, sizeof(cmd));
    cmd.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
    cmd.Instruction     = QUAD_IN_FAST_PROG_CMD;
    cmd.AddressMode     = HAL_OSPI_ADDRESS_1_LINE;
    cmd.AddressSize     = HAL_OSPI_ADDRESS_24_BITS;
    cmd.Address         = (uintptr_t)dst - EXTERNAL_FLASH_ADDRESS;
    cmd.DataMode        = HAL_OSPI_DATA_4_LINES;
    cmd.NbData          = len;

    status = HAL_OSPI_Command(hospi, &cmd, HAL_OSPI_TIMEOUT_DEFAULT_VALUE);
    if (status != HAL_OK) {
        printf("HAL_OSPI_Command() failed: status = %s, error = %s\r\n", HAL_StatusToString(status), OCTOSPI_ErrorToString(hospi->ErrorCode));
        return -1;
    }

    status = HAL_OSPI_Transmit(hospi, src, HAL_OSPI_TIMEOUT_DEFAULT_VALUE);
    if (status != HAL_OK) {
        printf("HAL_OSPI_Transmit() failed: status = %s, error = %s\r\n", HAL_StatusToString(status), OCTOSPI_ErrorToString(hospi->ErrorCode));
        return -1;
    }

    rc = OCTOSPI_WhileBusy(hospi, HAL_OSPI_TIMEOUT_DEFAULT_VALUE);
    if (rc < 0) {
        return rc;
    }

    return len;
}

int OCTOSPI_QuadWriteDMA(OSPI_HandleTypeDef *hospi, void *dst, void *src, size_t len)
{
    OSPI_RegularCmdTypeDef cmd;
    HAL_StatusTypeDef status;
    int rc;

    rc = OCTOSPI_WriteEnable(hospi, HAL_OSPI_TIMEOUT_DEFAULT_VALUE);
    if (rc < 0) {
        return rc;
    }

    memset(&cmd, 0, sizeof(cmd));
    cmd.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
    cmd.Instruction     = QUAD_IN_FAST_PROG_CMD;
    cmd.AddressMode     = HAL_OSPI_ADDRESS_1_LINE;
    cmd.AddressSize     = HAL_OSPI_ADDRESS_24_BITS;
    cmd.Address         = (uintptr_t)dst - EXTERNAL_FLASH_ADDRESS;
    cmd.DataMode        = HAL_OSPI_DATA_4_LINES;
    cmd.NbData          = len;

    status = HAL_OSPI_Command(hospi, &cmd, HAL_OSPI_TIMEOUT_DEFAULT_VALUE);
    if (status != HAL_OK) {
        printf("HAL_OSPI_Command() failed: status = %s, error = %s\r\n", HAL_StatusToString(status), OCTOSPI_ErrorToString(hospi->ErrorCode));
        return -1;
    }

    OctoTxCplt = 0;

    status = HAL_OSPI_Transmit_DMA(hospi, src);
    if (status != HAL_OK) {
        printf("HAL_OSPI_Transmit_DMA() failed: status = %s, error = %s\r\n", HAL_StatusToString(status), OCTOSPI_ErrorToString(hospi->ErrorCode));
        return -1;
    }

    while (!OctoTxCplt);

    rc = OCTOSPI_WhileBusy(hospi, HAL_OSPI_TIMEOUT_DEFAULT_VALUE);
    if (rc < 0) {
        return rc;
    }

    return len;
}

void HAL_OSPI_RxCpltCallback(OSPI_HandleTypeDef *hospi)
{
    OctoRxCplt++;
}

void HAL_OSPI_TxCpltCallback(OSPI_HandleTypeDef *hospi)
{
    OctoTxCplt++;
}

void HAL_OSPI_ErrorCallback(OSPI_HandleTypeDef *hospi)
{
    printf("HAL_OSPI_ErrorCallback\r\n");
}

void OCTOSPI1_IRQHandler(void)
{
    HAL_OSPI_IRQHandler(OctoSpiHandle);
}


void DMA1_Channel2_IRQHandler(void)
{
    HAL_DMA_IRQHandler(OctoSpiHandle->hdma);
}

static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct;
    RCC_ClkInitTypeDef RCC_ClkInitStruct;

#if 0
    // Configure the main internal regulator output voltage
    if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK) {
        while (1);
    }

    // Initializes the RCC Oscillators according to the specified parameters in the RCC_OscInitTypeDef structure.
    memset(&RCC_OscInitStruct, 0, sizeof(RCC_OscInitStruct));
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
    RCC_OscInitStruct.MSIState = RCC_MSI_ON;
    RCC_OscInitStruct.MSICalibrationValue = 0;
    RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        while (1);
    }

    // Initializes the CPU, AHB and APB buses clocks
    memset(&RCC_ClkInitStruct, 0, sizeof(RCC_ClkInitStruct));
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK) {
        while (1);
    }
#else
    // Configure the main internal regulator output voltage
    if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK) {
        while (1);
    }

    // Configure LSE Drive Capability
    //HAL_PWR_EnableBkUpAccess();
    //__HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);

    // MSI is enabled after System reset, activate PLL with MSI as source
    memset(&RCC_OscInitStruct, 0, sizeof(RCC_OscInitStruct));
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
    RCC_OscInitStruct.MSIState = RCC_MSI_ON;
    RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
    RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
    RCC_OscInitStruct.PLL.PLLM = 1;
    RCC_OscInitStruct.PLL.PLLN = 40;
    RCC_OscInitStruct.PLL.PLLR = 2;
    RCC_OscInitStruct.PLL.PLLP = 7;
    RCC_OscInitStruct.PLL.PLLQ = 4;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        // Initialization Error
        while (1);
    }

    // Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2 clocks dividers
    memset(&RCC_ClkInitStruct, 0, sizeof(RCC_ClkInitStruct));
    RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK) {
        // Initialization Error
        while (1);
    }
#endif
}

int main (int argc, char **argv)
{
    int rc;

    HAL_Init();

    SystemClock_Config();

    DBGUART_Init();

    write(STDOUT_FILENO, "\r\n", 2);

#if defined(DMAMUX1)
    printf("Has DMAMUX1\r\n");
#endif

    rc = OCTOSPI_Init(OctoSpiHandle);
    if (rc == 0) {
        printf ("OCTOSPI_Init() success\r\n");
    } else {
        printf ("OCTOSPI_Init() failed: %d\r\n", rc);
    }

    rc = OCTOSPI_Setup(OctoSpiHandle);
    if (rc == 0) {
        printf ("OCTOSPI_Setup() success\r\n");
    } else {
        printf ("OCTOSPI_Setup() failed: %d\r\n", rc);
    }

#if 0
    {
        uint32_t i = 0xAAAAAAAA;
        rc = OCTOSPI_Read(OctoSpiHandle, &i, (void *)(EXTERNAL_FLASH_ADDRESS + 0x200), sizeof(i));
        if (rc > 0) {
            printf ("OCTOSPI_Read() success: 0x%08lX\r\n", i);
        } else {
            printf ("OCTOSPI_Read() failed: %d\r\n", rc);
        }

        rc = OCTOSPI_MassErase(OctoSpiHandle);
        if (rc > 0) {
            printf ("OCTOSPI_MassErase() success\r\n");
        } else {
            printf ("OCTOSPI_MassErase() failed: %d\r\n", rc);
        }
    }
#endif

#if 1
    {
        uint32_t i = 0xAAAAAAAA;
        rc = OCTOSPI_Read(OctoSpiHandle, &i, (void *)(EXTERNAL_FLASH_ADDRESS + 0x200), sizeof(i));
        if (rc > 0) {
            printf ("OCTOSPI_Read() success: 0x%08lX\r\n", i);
        } else {
            printf ("OCTOSPI_Read() failed: %d\r\n", rc);
        }
    }
#endif

#if 1
    {
        uint8_t buf[256];
        memset(buf, 0xFF, sizeof(buf));
        *(uint32_t *)buf = 0xDEADBEEF;
        *(uint32_t *)(buf + 4) = 0xDEADBEEF;
        rc = OCTOSPI_QuadWrite(OctoSpiHandle, (void *)(EXTERNAL_FLASH_ADDRESS + 0x200), buf, sizeof(buf));
        if (rc > 0) {
            printf ("OCTOSPI_QuadWrite() success\r\n");
        } else {
            printf ("OCTOSPI_QuadWrite() failed: %d\r\n", rc);
        }
    }
#endif

#if 1
    {
        uint32_t i = 0xAAAAAAAA;
        rc = OCTOSPI_QuadRead(OctoSpiHandle, &i, (void *)(EXTERNAL_FLASH_ADDRESS + 0x200), sizeof(i));
        if (rc > 0) {
            printf ("OCTOSPI_QuadRead() success: 0x%08lX\r\n", i);
        } else {
            printf ("OCTOSPI_QuadRead() failed: %d\r\n", rc);
        }
    }
#endif

#if 1
    {
        rc = OCTOSPI_Erase4K(OctoSpiHandle, (void *)EXTERNAL_FLASH_ADDRESS);
        if (rc > 0) {
            printf ("OCTOSPI_Erase4K() success\r\n");
        } else {
            printf ("OCTOSPI_Erase4K() failed: %d\r\n", rc);
        }
    }
#endif

#if 1
    {
        uint32_t i = 0xAAAAAAAA;
        rc = OCTOSPI_Read(OctoSpiHandle, &i, (void *)(EXTERNAL_FLASH_ADDRESS + 0x200), sizeof(i));
        if (rc > 0) {
            printf ("OCTOSPI_Read() success: 0x%08lX\r\n", i);
        } else {
            printf ("OCTOSPI_Read() failed: %d\r\n", rc);
        }
    }
#endif

#if 1
    {
        uint8_t buf[256];
        memset(buf, 0xFF, sizeof(buf));
        *(uint32_t *)buf = 0xDEADBEEF;
        *(uint32_t *)(buf + 4) = 0xDEADBEEF;
        rc = OCTOSPI_QuadWriteDMA(OctoSpiHandle, (void *)(EXTERNAL_FLASH_ADDRESS + 0x200), buf, sizeof(buf));
        if (rc > 0) {
            printf ("OCTOSPI_QuadWriteDMA() success\r\n");
        } else {
            printf ("OCTOSPI_QuadWriteDMA() failed: %d\r\n", rc);
        }
    }
#endif

#if 1
    {
        uint32_t i = 0xAAAAAAAA;
        rc = OCTOSPI_QuadReadDMA(OctoSpiHandle, &i, (void *)(EXTERNAL_FLASH_ADDRESS + 0x200), sizeof(i));
        if (rc > 0) {
            printf ("OCTOSPI_QuadReadDMA() success: 0x%08lX\r\n", i);
        } else {
            printf ("OCTOSPI_QuadReadDMA() failed: %d\r\n", rc);
        }
    }
#endif

#if 1
    {
        uint8_t buf[256];
        memset(buf, 0xFF, sizeof(buf));
        *(uint32_t *)buf = 0xDEADBEEF;
        *(uint32_t *)(buf + 4) = 0xDEADBEEF;
        rc = OCTOSPI_QuadWriteDMA(OctoSpiHandle, (void *)EXTERNAL_FLASH_ADDRESS, buf, sizeof(buf));
        if (rc > 0) {
            printf ("OCTOSPI_QuadWriteDMA() success\r\n");
        } else {
            printf ("OCTOSPI_QuadWriteDMA() failed: %d\r\n", rc);
        }
    }
#endif

#if 1
    {
        uint64_t i = 0xAAAAAAAA;
        rc = OCTOSPI_QuadReadDMA(OctoSpiHandle, &i, (void *)EXTERNAL_FLASH_ADDRESS, sizeof(i));
        if (rc > 0) {
            printf ("OCTOSPI_QuadReadDMA() success: 0x%08lX%08lX\r\n", (uint32_t)(i >> 32), (uint32_t)i);
        } else {
            printf ("OCTOSPI_QuadReadDMA() failed: %d\r\n", rc);
        }
    }
#endif

    while(1) {
        write(STDOUT_FILENO, ".", 1);
        for (volatile int _wait = 0; _wait < 10000000; _wait++);
    }

    return -1;
}
