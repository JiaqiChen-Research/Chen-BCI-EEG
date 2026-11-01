/* USER CODE BEGIN Header */

/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : ADS1299 configuration test
  ******************************************************************************
  */

/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usb_device.h"
#include <stdio.h>
#include <string.h>

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi1;
UART_HandleTypeDef huart2;

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
static void MX_USART2_UART_Init(void);

/* USER CODE BEGIN 0 */

/* USER CODE BEGIN 0 */

/*
 * ADS1299 connection
 *
 * PA4 -> CS
 * PB0 -> DRDY
 * PB1 -> RESET
 *
 * SPI1:
 * PA5 -> SCLK
 * PA6 -> MISO / DOUT
 * PA7 -> MOSI / DIN
 *
 * USART2:
 * PA2 -> TX
 * PA3 -> RX
 */


/* ============================================================
 * ADS1299 Write Register
 * ============================================================ */
static HAL_StatusTypeDef ADS1299_WriteReg(uint8_t reg, uint8_t data)
{
    uint8_t cmd[2];

    /*
     * WREG:
     * 010r rrrr
     */
    cmd[0] = 0x40 | (reg & 0x1F);

    /*
     * Write 1 register:
     * N-1 = 0
     */
    cmd[1] = 0x00;

    /* CS LOW */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);

    /* Send WREG command */
    if (HAL_SPI_Transmit(&hspi1, cmd, 2, 100) != HAL_OK)
    {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
        return HAL_ERROR;
    }

    /* Send register data */
    if (HAL_SPI_Transmit(&hspi1, &data, 1, 100) != HAL_OK)
    {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
        return HAL_ERROR;
    }

    /* CS HIGH */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);

    /*
     * Give ADS1299 some time before next command
     */
    HAL_Delay(1);

    return HAL_OK;
}


/* ============================================================
 * ADS1299 Send Command
 * ============================================================ */
static HAL_StatusTypeDef ADS1299_SendCommand(uint8_t command)
{
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);

    if (HAL_SPI_Transmit(&hspi1, &command, 1, 100) != HAL_OK)
    {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
        return HAL_ERROR;
    }

    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);

    HAL_Delay(1);

    return HAL_OK;
}


/* ============================================================
 * ADS1299 Read Register
 * ============================================================ */
static HAL_StatusTypeDef ADS1299_ReadReg(uint8_t reg, uint8_t *data)
{
    uint8_t cmd[2];

    /*
     * RREG:
     * 001r rrrr
     */
    cmd[0] = 0x20 | (reg & 0x1F);

    /*
     * Read 1 register:
     * N-1 = 0
     */
    cmd[1] = 0x00;

    /* CS LOW */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);

    /* Send RREG command */
    if (HAL_SPI_Transmit(&hspi1, cmd, 2, 100) != HAL_OK)
    {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
        return HAL_ERROR;
    }

    /*
     * ADS1299 needs a short delay before register data
     */
    HAL_Delay(1);

    /* Read register data */
    if (HAL_SPI_Receive(&hspi1, data, 1, 100) != HAL_OK)
    {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
        return HAL_ERROR;
    }

    /* CS HIGH */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);

    return HAL_OK;
}


/* ============================================================
 * ADS1299 Hardware Reset
 * ============================================================ */
static void ADS1299_Reset(void)
{
    /* RESET LOW */
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_RESET);

    HAL_Delay(10);

    /* RESET HIGH */
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_SET);

    HAL_Delay(10);
}


/* ============================================================
 * UART Send String
 * ============================================================ */
static void UART_SendString(char *str)
{
    HAL_UART_Transmit(
        &huart2,
        (uint8_t *)str,
        strlen(str),
        1000
    );
}


/* ============================================================
 * Print Register
 * ============================================================ */
static void ADS1299_PrintReg(uint8_t reg, char *name)
{
    uint8_t value = 0;
    char message[100];

    if (ADS1299_ReadReg(reg, &value) == HAL_OK)
    {
        sprintf(
            message,
            "%s (0x%02X) = 0x%02X\r\n",
            name,
            reg,
            value
        );

        UART_SendString(message);
    }
    else
    {
        UART_SendString(
            "ERROR: register read failed!\r\n"
        );
    }
}


/* ============================================================
 * Convert ADS1299 24-bit signed data to int32
 * ============================================================ */
static int32_t ADS1299_Parse24(uint8_t *data)
{
    int32_t value;

    value =
        ((int32_t)data[0] << 16) |
        ((int32_t)data[1] << 8)  |
        ((int32_t)data[2]);

    /*
     * ADS1299 uses 24-bit two's complement.
     * Sign extension:
     */
    if (value & 0x00800000)
    {
        value |= 0xFF000000;
    }

    return value;
}


/* ============================================================
 * Read one ADS1299 data frame
 *
 * ADS1299:
 *
 * Status = 3 bytes
 * CH1    = 3 bytes
 * CH2    = 3 bytes
 * ...
 * CH8    = 3 bytes
 *
 * Total:
 *
 * 3 + 8*3 = 27 bytes
 *
 * ============================================================ */
static HAL_StatusTypeDef ADS1299_ReadDataFrame(uint8_t *frame)
{
    uint8_t tx_dummy[27] = {0};

    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);

    HAL_StatusTypeDef status =
        HAL_SPI_TransmitReceive(
            &hspi1,
            tx_dummy,
            frame,
            27,
            100
        );

    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);

    return status;
}

/* ============================================================
 * Print one CH1 sample
 * ============================================================ */
static void ADS1299_PrintCH1(uint8_t *frame)
{
    char message[64];

    /*
     * frame[0] ~ frame[2]
     *     STATUS
     *
     * frame[3] ~ frame[5]
     *     CH1
     */

    int32_t ch1 =
        ADS1299_Parse24(&frame[3]);

    sprintf(
        message,
        "%ld\r\n",
        (long)ch1
    );

    UART_SendString(message);
}


/* USER CODE END 0 */

/* USER CODE END 0 */

volatile uint32_t drdy_irq_count = 0;
volatile uint8_t drdy_flag = 0;

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_0)
    {
        drdy_irq_count++;
        drdy_flag = 1;
    }
}
/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
    /* MCU Configuration--------------------------------------------------------*/

    char message[64];

    HAL_Init();

    SystemClock_Config();

    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_SPI1_Init();
    MX_USB_DEVICE_Init();
    MX_USART2_UART_Init();

    /* USER CODE BEGIN 2 */

    uint8_t ads_id = 0;
    uint8_t frame[27];

    int32_t ch1 = 0;
    uint32_t sample_count = 0;

    /* ========================================================
     * Initial GPIO state
     * ======================================================== */

    /* CS HIGH */
    HAL_GPIO_WritePin(
        GPIOA,
        GPIO_PIN_4,
        GPIO_PIN_SET
    );

    /* RESET HIGH */
    HAL_GPIO_WritePin(
        GPIOB,
        GPIO_PIN_1,
        GPIO_PIN_SET
    );

    HAL_Delay(100);

    /* ========================================================
     * UART Header
     * ======================================================== */

    UART_SendString("\r\n");
    UART_SendString("============================\r\n");
    UART_SendString("ADS1299 EEG ACQUISITION TEST\r\n");
    UART_SendString("============================\r\n");

    /* ========================================================
     * STEP 1
     * Hardware RESET
     * ======================================================== */

    UART_SendString(
        "Reset ADS1299...\r\n"
    );

    ADS1299_Reset();

    UART_SendString(
        "Reset done.\r\n"
    );

    /* ========================================================
     * STEP 2
     * Exit RDATAC
     *
     * After reset ADS1299 enters RDATAC.
     * We must send SDATAC before register access.
     * ======================================================== */

    UART_SendString(
        "Send SDATAC...\r\n"
    );

    if (
        ADS1299_SendCommand(0x11)
        != HAL_OK
    )
    {
        UART_SendString(
            "SDATAC ERROR!\r\n"
        );
    }
    else
    {
        UART_SendString(
            "SDATAC done.\r\n"
        );
    }

    /* ========================================================
     * STEP 3
     * Read ADS1299 ID
     * ======================================================== */

    if (
        ADS1299_ReadReg(
            0x00,
            &ads_id
        ) == HAL_OK
    )
    {
        sprintf(
            message,
            "ADS1299 ID = 0x%02X\r\n",
            ads_id
        );

        UART_SendString(message);
    }
    else
    {
        UART_SendString(
            "ERROR: ADS1299 ID read failed!\r\n"
        );
    }

    /* ========================================================
     * STEP 4
     * Configure ADS1299
     * ======================================================== */

    UART_SendString(
        "\r\nConfiguring ADS1299...\r\n"
    );

    /*
     * CONFIG1 = 0x94
     *
     * Data rate = 1000 SPS
     */
    ADS1299_WriteReg(
        0x01,
        0x94
    );

    /*
     * CONFIG2 = 0xC0
     *
     * Internal test signal enabled.
     */
    ADS1299_WriteReg(
        0x02,
        0xC0
    );

    /*
     * CONFIG3 = 0xE0
     *
     * Internal reference configuration.
     */
    ADS1299_WriteReg(
        0x03,
        0xE0
    );

    /*
     * CH1SET = 0x60
     *
     * Channel 1:
     *
     * Power up
     * Gain = 6
     * MUX = internal test signal
     */
    ADS1299_WriteReg(
        0x05,
        0x60

    );

    /*
     * CH2 ~ CH8 powered down
     */
    ADS1299_WriteReg(0x06, 0x81);
    ADS1299_WriteReg(0x07, 0x81);
    ADS1299_WriteReg(0x08, 0x81);
    ADS1299_WriteReg(0x09, 0x81);
    ADS1299_WriteReg(0x0A, 0x81);
    ADS1299_WriteReg(0x0B, 0x81);
    ADS1299_WriteReg(0x0C, 0x81);

    UART_SendString(
        "Configuration written.\r\n"
    );

    /* ========================================================
     * STEP 5
     * Read back registers
     * ======================================================== */

    UART_SendString(
        "\r\nRead back registers:\r\n"
    );

    ADS1299_PrintReg(
        0x00,
        "ID"
    );

    ADS1299_PrintReg(
        0x01,
        "CONFIG1"
    );

    ADS1299_PrintReg(
        0x02,
        "CONFIG2"
    );

    ADS1299_PrintReg(
        0x03,
        "CONFIG3"
    );

    ADS1299_PrintReg(
        0x05,
        "CH1SET"
    );

    ADS1299_PrintReg(
        0x06,
        "CH2SET"
    );

    ADS1299_PrintReg(
        0x07,
        "CH3SET"
    );

    ADS1299_PrintReg(
        0x08,
        "CH4SET"
    );

    ADS1299_PrintReg(
        0x09,
        "CH5SET"
    );

    ADS1299_PrintReg(
        0x0A,
        "CH6SET"
    );

    ADS1299_PrintReg(
        0x0B,
        "CH7SET"
    );

    ADS1299_PrintReg(
        0x0C,
        "CH8SET"
    );

    /* ========================================================
     * STEP 6
     * Start ADS1299 conversions
     * ======================================================== */

    UART_SendString(
        "\r\nStarting ADS1299...\r\n"
    );

    if (
        ADS1299_SendCommand(0x08)
        != HAL_OK
    )
    {
        UART_SendString(
            "START command ERROR!\r\n"
        );

        while (1)
        {
        }
    }

    UART_SendString(
        "START done.\r\n"
    );

    /* ========================================================
     * STEP 7
     * Enter RDATAC mode
     *
     * 0x10 = RDATAC
     * ======================================================== */

    UART_SendString(
        "Send RDATAC...\r\n"
    );

    if (
        ADS1299_SendCommand(0x10)
        != HAL_OK
    )
    {
        UART_SendString(
            "RDATAC ERROR!\r\n"
        );

        while (1)
        {
        }
    }

    UART_SendString(
        "RDATAC started.\r\n"
    );

    /* ========================================================
     * Header for data stream
     * ======================================================== */

    UART_SendString(
        "DATA_START\r\n"
    );

    /* USER CODE END 2 */

    /* Infinite loop */

    while (1)
    {
        /*
         * DRDY falling edge sets drdy_flag.
         * Then read one complete ADS1299 data frame.
         */
        if (drdy_flag)
        {
            drdy_flag = 0;

            if (ADS1299_ReadDataFrame(frame) == HAL_OK)
            {
                /*
                 * ADS1299 frame:
                 *
                 * frame[0]  STATUS
                 * frame[1]  STATUS
                 * frame[2]  STATUS
                 *
                 * frame[3]  CH1 MSB
                 * frame[4]  CH1 MID
                 * frame[5]  CH1 LSB
                 */

                ch1 = ADS1299_Parse24(&frame[3]);

                sample_count++;

                /*
                 * Print one sample every 10 samples.
                 * This prevents the 115200-baud UART
                 * from becoming the bottleneck.
                 */
                sprintf(
                    message,
                    "%ld\r\n",
                    (long)ch1
                );


                UART_SendString(message);
            }
        }
    }
}



/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();

    __HAL_PWR_VOLTAGESCALING_CONFIG(
        PWR_REGULATOR_VOLTAGE_SCALE1
    );

    RCC_OscInitStruct.OscillatorType =
        RCC_OSCILLATORTYPE_HSE;

    RCC_OscInitStruct.HSEState =
        RCC_HSE_ON;

    RCC_OscInitStruct.PLL.PLLState =
        RCC_PLL_ON;

    RCC_OscInitStruct.PLL.PLLSource =
        RCC_PLLSOURCE_HSE;

    RCC_OscInitStruct.PLL.PLLM = 4;
    RCC_OscInitStruct.PLL.PLLN = 72;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 3;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType =
        RCC_CLOCKTYPE_HCLK |
        RCC_CLOCKTYPE_SYSCLK |
        RCC_CLOCKTYPE_PCLK1 |
        RCC_CLOCKTYPE_PCLK2;

    RCC_ClkInitStruct.SYSCLKSource =
        RCC_SYSCLKSOURCE_PLLCLK;

    RCC_ClkInitStruct.AHBCLKDivider =
        RCC_SYSCLK_DIV1;

    RCC_ClkInitStruct.APB1CLKDivider =
        RCC_HCLK_DIV2;

    RCC_ClkInitStruct.APB2CLKDivider =
        RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(
            &RCC_ClkInitStruct,
            FLASH_LATENCY_2) != HAL_OK)
    {
        Error_Handler();
    }
}


/**
  * @brief SPI1 Initialization Function
  * @retval None
  */
static void MX_SPI1_Init(void)
{
    hspi1.Instance = SPI1;

    hspi1.Init.Mode =
        SPI_MODE_MASTER;

    hspi1.Init.Direction =
        SPI_DIRECTION_2LINES;

    hspi1.Init.DataSize =
        SPI_DATASIZE_8BIT;

    /*
     * ADS1299 SPI:
     * CPOL = 0
     * CPHA = 1
     */
    hspi1.Init.CLKPolarity =
        SPI_POLARITY_LOW;

    hspi1.Init.CLKPhase =
        SPI_PHASE_2EDGE;

    /*
     * CS由PA4软件控制
     */
    hspi1.Init.NSS =
        SPI_NSS_SOFT;

    /*
     * 72 MHz / 64
     */
    hspi1.Init.BaudRatePrescaler =
        SPI_BAUDRATEPRESCALER_64;

    hspi1.Init.FirstBit =
        SPI_FIRSTBIT_MSB;

    hspi1.Init.TIMode =
        SPI_TIMODE_DISABLE;

    hspi1.Init.CRCCalculation =
        SPI_CRCCALCULATION_DISABLE;

    hspi1.Init.CRCPolynomial =
        10;

    if (HAL_SPI_Init(&hspi1) != HAL_OK)
    {
        Error_Handler();
    }
}


/**
  * @brief USART2 Initialization Function
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{
    huart2.Instance = USART2;

    huart2.Init.BaudRate = 460800;

    huart2.Init.WordLength =
        UART_WORDLENGTH_8B;

    huart2.Init.StopBits =
        UART_STOPBITS_1;

    huart2.Init.Parity =
        UART_PARITY_NONE;

    huart2.Init.Mode =
        UART_MODE_TX_RX;

    huart2.Init.HwFlowCtl =
        UART_HWCONTROL_NONE;

    huart2.Init.OverSampling =
        UART_OVERSAMPLING_16;

    if (HAL_UART_Init(&huart2) != HAL_OK)
    {
        Error_Handler();
    }
}


/**
  * @brief GPIO Initialization Function
  * @retval None
  */
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();


    /*
     * PC13
     */
    HAL_GPIO_WritePin(
        GPIOC,
        GPIO_PIN_13,
        GPIO_PIN_RESET
    );


    /*
     * ADS1299 RESET
     */
    HAL_GPIO_WritePin(
        GPIOB,
        GPIO_PIN_1,
        GPIO_PIN_SET
    );


    /*
     * ADS1299 CS
     * 默认HIGH = 不选择ADS1299
     */
    HAL_GPIO_WritePin(
        GPIOA,
        GPIO_PIN_4,
        GPIO_PIN_SET
    );


    /*
     * PA9
     */
    HAL_GPIO_WritePin(
        GPIOA,
        GPIO_PIN_9,
        GPIO_PIN_SET
    );


    /* PC13 */
    GPIO_InitStruct.Pin =
        GPIO_PIN_13;

    GPIO_InitStruct.Mode =
        GPIO_MODE_OUTPUT_PP;

    GPIO_InitStruct.Pull =
        GPIO_NOPULL;

    GPIO_InitStruct.Speed =
        GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init(
        GPIOC,
        &GPIO_InitStruct
    );


    /*
     * PA4
     * ADS1299 CS
     */
    GPIO_InitStruct.Pin =
        GPIO_PIN_4;

    GPIO_InitStruct.Mode =
        GPIO_MODE_OUTPUT_PP;

    GPIO_InitStruct.Pull =
        GPIO_NOPULL;

    GPIO_InitStruct.Speed =
        GPIO_SPEED_FREQ_HIGH;

    HAL_GPIO_Init(
        GPIOA,
        &GPIO_InitStruct
    );


    /*
     * PB0
     * ADS1299 DRDY
     */
    GPIO_InitStruct.Pin =
        GPIO_PIN_0;

    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;

    GPIO_InitStruct.Pull =
        GPIO_NOPULL;

    HAL_GPIO_Init(
        GPIOB,
        &GPIO_InitStruct
    );
    HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);


    /*
     * PB1
     * ADS1299 RESET
     */
    GPIO_InitStruct.Pin =
        GPIO_PIN_1;

    GPIO_InitStruct.Mode =
        GPIO_MODE_OUTPUT_PP;

    GPIO_InitStruct.Pull =
        GPIO_NOPULL;

    GPIO_InitStruct.Speed =
        GPIO_SPEED_FREQ_HIGH;

    HAL_GPIO_Init(
        GPIOB,
        &GPIO_InitStruct
    );


    /*
     * PA9
     */
    GPIO_InitStruct.Pin =
        GPIO_PIN_9;

    GPIO_InitStruct.Mode =
        GPIO_MODE_OUTPUT_PP;

    GPIO_InitStruct.Pull =
        GPIO_NOPULL;

    GPIO_InitStruct.Speed =
        GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init(
        GPIOA,
        &GPIO_InitStruct
    );
}


/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
    __disable_irq();

    while (1)
    {
    }
}


#ifdef USE_FULL_ASSERT

void assert_failed(uint8_t *file, uint32_t line)
{
}

#endif
