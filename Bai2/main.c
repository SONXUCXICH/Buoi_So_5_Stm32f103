#include <stdint.h>
#include <stdio.h>

#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_flash.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_exti.h"
#include "misc.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* ========== 1. KHAI BAO CAU HINH ========== */

#define UART_BAUD       115200U
#define QUEUE_LENGTH    10U
#define BUTTON_PIN      GPIO_Pin_0
#define BUTTON_EXTI     EXTI_Line0
#define DEBOUNCE_TIME   200U
#define GROUP_CODE      "DTMT01_N11"

/* Queue chuyen so lan nhan nut tu ISR den Task UART */
static QueueHandle_t q_btn_count = NULL;

/* Bien dem tong so lan nhan nut */
static volatile uint32_t button_press_count = 0U;

/* ========== 2. CAU HINH CLOCK 72MHZ ========== */

static void Clock_Config(void)
{
    /* Su dung HSE 8MHz va PLL x9 de tao clock 72MHz */
    RCC_DeInit();
    RCC_HSEConfig(RCC_HSE_ON);

    /* Cho thach anh ngoai HSE san sang */
    if (RCC_WaitForHSEStartUp() != SUCCESS)
    {
        while (1) { }
    }

    /* Cau hinh Flash khi he thong chay 72MHz */
    FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);
    FLASH_SetLatency(FLASH_Latency_2);

    /* AHB = 72MHz, APB1 = 36MHz, APB2 = 72MHz */
    RCC_HCLKConfig(RCC_SYSCLK_Div1);
    RCC_PCLK1Config(RCC_HCLK_Div2);
    RCC_PCLK2Config(RCC_HCLK_Div1);

    /* PLL = HSE 8MHz x 9 */
    RCC_PLLConfig(RCC_PLLSource_HSE_Div1, RCC_PLLMul_9);
    RCC_PLLCmd(ENABLE);

    while (RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET) { }

    /* Chuyen clock he thong sang PLL */
    RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);
    while (RCC_GetSYSCLKSource() != 0x08U) { }

    SystemCoreClockUpdate();
}

/* ========== 3. CAU HINH LED PC13 ========== */

static void LED_Config(void)
{
    GPIO_InitTypeDef gpio;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

    /* LED PC13 tren Blue Pill hoat dong o muc thap */
    GPIO_SetBits(GPIOC, GPIO_Pin_13);

    gpio.GPIO_Pin = GPIO_Pin_13;
    gpio.GPIO_Speed = GPIO_Speed_2MHz;
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOC, &gpio);
}

/* ========== 4. CAU HINH UART1 115200 BAUD ========== */

static void UART1_Config(void)
{
    GPIO_InitTypeDef gpio;
    USART_InitTypeDef uart;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA |
                           RCC_APB2Periph_AFIO |
                           RCC_APB2Periph_USART1, ENABLE);

    /* PA9 la USART1_TX, che do Alternate Function Push Pull */
    gpio.GPIO_Pin = GPIO_Pin_9;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &gpio);

    /* PA10 la USART1_RX, che do Input Floating */
    gpio.GPIO_Pin = GPIO_Pin_10;
    gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &gpio);

    /* UART1: 115200 baud, 8 data bit, 1 stop bit, no parity */
    USART_StructInit(&uart);
    uart.USART_BaudRate = UART_BAUD;
    uart.USART_WordLength = USART_WordLength_8b;
    uart.USART_StopBits = USART_StopBits_1;
    uart.USART_Parity = USART_Parity_No;
    uart.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    uart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;

    USART_Init(USART1, &uart);
    USART_Cmd(USART1, ENABLE);
}

/* ========== 5. GUI DU LIEU QUA UART ========== */

static void UART1_SendString(const char *str)
{
    while (*str != '\0')
    {
        /* Doi thanh ghi truyen rong */
        while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET) { }

        USART_SendData(USART1, (uint16_t)(uint8_t)(*str));
        str++;
    }

    /* Doi truyen xong ky tu cuoi cung */
    while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET) { }
}

/* ========== 6. CAU HINH NUT PB0 VA EXTI0 ========== */

static void EXTI0_Config(void)
{
    GPIO_InitTypeDef gpio;
    EXTI_InitTypeDef exti;
    NVIC_InitTypeDef nvic;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB |
                           RCC_APB2Periph_AFIO, ENABLE);

    /* PB0 la Input Pull Up, nhan nut se keo xuong GND */
    gpio.GPIO_Pin = BUTTON_PIN;
    gpio.GPIO_Speed = GPIO_Speed_2MHz;
    gpio.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOB, &gpio);

    /* Chon PB0 la nguon ngat EXTI Line 0 */
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource0);

    /* Cau hinh ngat canh xuong, chua cho phep ngat */
    exti.EXTI_Line = BUTTON_EXTI;
    exti.EXTI_Mode = EXTI_Mode_Interrupt;
    exti.EXTI_Trigger = EXTI_Trigger_Falling;
    exti.EXTI_LineCmd = DISABLE;
    EXTI_Init(&exti);
    EXTI_ClearITPendingBit(BUTTON_EXTI);

    /* Cau hinh 4 bit Preemption cho FreeRTOS */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);

    /* Uu tien ngat 6, phu hop gioi han FreeRTOS mac dinh */
    nvic.NVIC_IRQChannel = EXTI0_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 6U;
    nvic.NVIC_IRQChannelSubPriority = 0U;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);
}

/* ========== 7. HAM NGAT NGOAI EXTI0 ========== */

void EXTI0_IRQHandler(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    uint32_t count;

    if (EXTI_GetITStatus(BUTTON_EXTI) != RESET)
    {
        /* Khoa ngat ngay de han che rung phim */
        NVIC_DisableIRQ(EXTI0_IRQn);

        /* Xoa co ngat hien tai */
        EXTI_ClearITPendingBit(BUTTON_EXTI);

        /* Tang so lan nhan nut */
        button_press_count++;
        count = button_press_count;

        /* Gui gia tri dem vao Queue tu ISR */
        if (q_btn_count != NULL)
        {
            xQueueSendFromISR(q_btn_count,
                              &count,
                              &xHigherPriorityTaskWoken);
        }
    }

    /* Chuyen ngay sang Task neu Task co uu tien cao hon */
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/* ========== 8. TASK TRUYEN UART ========== */

static void Task_UART(void *pvParameters)
{
    EXTI_InitTypeDef exti;
    uint32_t count = 0U;
    char msg[80];

    (void)pvParameters;

    /* Bat ngat EXTI0 sau khi Scheduler da khoi dong */
    exti.EXTI_Line = BUTTON_EXTI;
    exti.EXTI_Mode = EXTI_Mode_Interrupt;
    exti.EXTI_Trigger = EXTI_Trigger_Falling;
    exti.EXTI_LineCmd = ENABLE;

    EXTI_ClearITPendingBit(BUTTON_EXTI);
    NVIC_ClearPendingIRQ(EXTI0_IRQn);
    EXTI_Init(&exti);

    while (1)
    {
        /* Task cho du lieu Queue o trang thai Blocked */
        if (xQueueReceive(q_btn_count, &count, portMAX_DELAY) == pdTRUE)
        {
            /* Dao trang thai LED PC13 khi nhan du lieu */
            if (GPIO_ReadOutputDataBit(GPIOC, GPIO_Pin_13) != Bit_RESET)
            {
                GPIO_ResetBits(GPIOC, GPIO_Pin_13);
            }
            else
            {
                GPIO_SetBits(GPIOC, GPIO_Pin_13);
            }

            /* Dinh dang thong bao voi ma nhom 11 */
            snprintf(msg, sizeof(msg),
                     GROUP_CODE ": Button Pressed = %lu\r\n",
                     (unsigned long)count);

            /* Truyen chuoi len Serial Terminal */
            UART1_SendString(msg);

            /* Khoa ngat trong 200ms de chong doi nut nhan */
            vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_TIME));

            /* Xoa co ngat sinh ra khi tiep diem bi rung */
            EXTI_ClearITPendingBit(BUTTON_EXTI);
            NVIC_ClearPendingIRQ(EXTI0_IRQn);

            /* Cho phep ngat EXTI0 cho lan nhan tiep theo */
            NVIC_EnableIRQ(EXTI0_IRQn);
        }
    }
}

/* ========== 9. HAM MAIN ========== */

int main(void)
{
    /* Cau hinh clock 72MHz nhu chuong trinh HAL goc */
    Clock_Config();

    /* Cau hinh LED, UART va nut nhan */
    LED_Config();
    UART1_Config();
    EXTI0_Config();

    /* Tao Queue 10 phan tu, moi phan tu la uint32_t */
    q_btn_count = xQueueCreate(QUEUE_LENGTH, sizeof(uint32_t));

    if (q_btn_count != NULL)
    {
        /* Tao Task UART nhan Queue va in ra Terminal */
        if (xTaskCreate(Task_UART, "UART_Task",
                        configMINIMAL_STACK_SIZE + 128U,
                        NULL, 2U, NULL) == pdPASS)
        {
            /* Khoi dong bo lap lich FreeRTOS */
            vTaskStartScheduler();
        }
    }

    /* Neu khoi tao that bai, chuong trinh dung tai day */
    while (1) { }
}