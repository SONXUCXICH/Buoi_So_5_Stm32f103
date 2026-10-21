#include <stdint.h>
#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* ========== 1. KHAI BAO CAU HINH ========== */

// So luong LED can dieu khien
#define LED_COUNT       3

// Chu ky PWM gom 1000 xung dem
#define PWM_PERIOD      1000

// Moi lan nhan nut tang hoac giam 10% do sang
#define BRIGHTNESS_STEP 10

// Do sang ban dau cua cac LED la 50%
#define INITIAL_DUTY    50

// Dinh nghia chan cua 3 nut nhan
#define BTN_SELECT      0     // PB0: Chuyen LED
#define BTN_UP          1     // PB1: Tang do sang
#define BTN_DOWN        10    // PB10: Giam do sang

/* ========== 2. KHAI BAO CAU TRUC DU LIEU ========== */

// Cau truc du lieu gui tu Task nut nhan den Task LED
typedef struct
{
    uint8_t duty;       // Do sang LED tu 0 den 100%
    uint8_t identify;   // Bao LED nhay khi duoc chon
} LED_Message;

// Cau truc quan ly tung LED
typedef struct
{
    QueueHandle_t queue;  // Queue nhan lenh dieu khien LED
    uint8_t channel;      // Kenh PWM cua LED
} LED_Context;

// Cau truc luu trang thai nut nhan
typedef struct
{
    uint8_t stable;          // Trang thai nut da on dinh
    uint8_t last_raw;        // Gia tri doc gan nhat
    TickType_t changed_at;   // Thoi diem nut thay doi
} ButtonState;

// Mang quan ly 3 LED
static LED_Context leds[LED_COUNT];

/* ========== 3. HAM CHUYEN MILIGIAY SANG TICK ========== */

// Dam bao thoi gian tre toi thieu la 1 Tick
static TickType_t ms_to_ticks(uint32_t ms)
{
    TickType_t ticks = pdMS_TO_TICKS(ms);

    if (ticks == 0)
    {
        ticks = 1;
    }

    return ticks;
}

/* ========== 4. LAY TAN SO CLOCK TIMER 2 ========== */

static uint32_t TIM2_GetClock(void)
{
    // Doc he so chia clock cua APB1
    uint32_t ppre = (RCC->CFGR >> 8) & 7;

    // Tinh he so chia clock APB1
    uint32_t div = (ppre < 4)
                   ? 1
                   : (1UL << (ppre - 3));

    // Tinh tan so clock APB1
    uint32_t pclk = SystemCoreClock / div;

    // Neu APB1 co chia clock thi Timer nhan clock gap doi
    if (div == 1)
    {
        return pclk;
    }
    else
    {
        return pclk * 2;
    }
}

/* ========== 5. CAU HINH GPIO ========== */

void GPIO_Config(void)
{
    // Bat clock cho GPIOA, GPIOB va AFIO
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN |
                    RCC_APB2ENR_IOPBEN |
                    RCC_APB2ENR_AFIOEN;

    // Chon anh xa mac dinh cua Timer 2
    // TIM2_CH1 = PA0
    // TIM2_CH2 = PA1
    // TIM2_CH3 = PA2
    AFIO->MAPR &= ~(3UL << 8);

    // Cau hinh PA0, PA1, PA2 lam dau ra PWM
    // Che do Alternate Function Push-Pull
    GPIOA->CRL &= ~0x00000FFF;
    GPIOA->CRL |=  0x00000BBB;

    // Cau hinh PB0 va PB1 lam dau vao Pull-up
    GPIOB->CRL &= ~0x000000FF;
    GPIOB->CRL |=  0x00000088;

    // Cau hinh PB10 lam dau vao Pull-up
    GPIOB->CRH &= ~0x00000F00;
    GPIOB->CRH |=  0x00000800;

    // Kich hoat dien tro keo len noi bo cho 3 nut
    GPIOB->BSRR = (1UL << BTN_SELECT) |
                  (1UL << BTN_UP) |
                  (1UL << BTN_DOWN);
}

/* ========== 6. THAY DOI DO SANG LED ========== */

void PWM_SetDuty(uint8_t channel, uint8_t duty)
{
    uint16_t value;

    // Gioi han do sang toi da 100%
    if (duty > 100)
    {
        duty = 100;
    }

    // Chuyen do sang phan tram sang gia tri PWM
    value = (uint16_t)
            (PWM_PERIOD * (uint32_t)duty / 100);

    // Cap nhat gia tri PWM cho tung kenh
    switch (channel)
    {
        case 0:
            // LED 1 su dung TIM2_CH1 tai PA0
            TIM2->CCR1 = value;
            break;

        case 1:
            // LED 2 su dung TIM2_CH2 tai PA1
            TIM2->CCR2 = value;
            break;

        case 2:
            // LED 3 su dung TIM2_CH3 tai PA2
            TIM2->CCR3 = value;
            break;

        default:
            break;
    }
}

/* ========== 7. CAU HINH PWM TAN SO 1KHZ ========== */

void PWM_Config(void)
{
    uint32_t tim_clk;
    uint32_t prescaler;

    // Bat clock cho Timer 2
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

    // Cap nhat tan so clock cua he thong
    SystemCoreClockUpdate();

    // Lay tan so clock cua Timer 2
    tim_clk = TIM2_GetClock();

    // Cau hinh tan so dem Timer la 1MHz
    // PWM = 1MHz / 1000 = 1kHz
    prescaler = tim_clk / 1000000UL;

    // Kiem tra bo chia clock hop le
    if (prescaler == 0 ||
        prescaler > 65536 ||
        tim_clk % 1000000UL != 0)
    {
        while (1);
    }

    // Dung Timer truoc khi cau hinh
    TIM2->CR1 = 0;

    // Thiet lap he so chia tan so
    TIM2->PSC = prescaler - 1;

    // Dat chu ky PWM bang 1000 xung dem
    TIM2->ARR = PWM_PERIOD - 1;

    // Cau hinh PWM Mode 1 cho kenh 1 va kenh 2
    TIM2->CCMR1 = (6UL << 4)  |
                  (1UL << 3)  |
                  (6UL << 12) |
                  (1UL << 11);

    // Cau hinh PWM Mode 1 cho kenh 3
    TIM2->CCMR2 = (6UL << 4) |
                  (1UL << 3);

    // Cho phep xuat PWM ra 3 kenh
    TIM2->CCER = TIM_CCER_CC1E |
                 TIM_CCER_CC2E |
                 TIM_CCER_CC3E;

    // Cai dat do sang ban dau cho 3 LED
    PWM_SetDuty(0, INITIAL_DUTY);
    PWM_SetDuty(1, INITIAL_DUTY);
    PWM_SetDuty(2, INITIAL_DUTY);

    // Bat che do nap lai ARR
    TIM2->CR1 |= TIM_CR1_ARPE;

    // Cap nhat cac thanh ghi Timer
    TIM2->EGR = TIM_EGR_UG;

    // Bat Timer 2 de bat dau tao PWM
    TIM2->CR1 |= TIM_CR1_CEN;
}

/* ========== 8. DOC TRANG THAI NUT NHAN ========== */

uint8_t Button_Read(uint8_t pin)
{
    // Nut nhan duoc noi xuong GND
    // Khi nhan nut: muc logic 0
    // Khi tha nut: muc logic 1
    // Ham tra ve 1 neu nut duoc nhan
    // Ham tra ve 0 neu nut khong duoc nhan

    if ((GPIOB->IDR & (1UL << pin)) == 0)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

/* ========== 9. CHONG DOI PHIM NHAN ========== */

uint8_t Button_Pressed(ButtonState *btn,
                       uint8_t pin,
                       TickType_t now)
{
    // Doc trang thai nut nhan hien tai
    uint8_t raw = Button_Read(pin);

    // Neu trang thai nut thay doi
    if (raw != btn->last_raw)
    {
        // Luu trang thai moi
        btn->last_raw = raw;

        // Luu thoi diem xay ra thay doi
        btn->changed_at = now;
    }

    // Kiem tra trang thai nut on dinh trong 30ms
    if (raw != btn->stable &&
        (TickType_t)(now - btn->changed_at)
        >= ms_to_ticks(30))
    {
        // Cap nhat trang thai on dinh
        btn->stable = raw;

        // Chi xu ly khi nut vua duoc nhan
        if (raw == 1)
        {
            return 1;
        }
    }

    // Khong co su kien nhan nut moi
    return 0;
}

/* ========== 10. GUI DU LIEU QUA QUEUE ========== */

void LED_Send(uint8_t index,
              uint8_t duty,
              uint8_t identify)
{
    LED_Message msg;

    // Gan gia tri do sang can dieu khien
    msg.duty = duty;

    // Gan trang thai nhay bao LED duoc chon
    msg.identify = identify;

    // Gui du lieu moi nhat den Queue cua LED
    // Moi Queue chi luu mot thong diep
    xQueueOverwrite(leds[index].queue, &msg);
}

/* ========== 11. TASK DIEU KHIEN LED ========== */

void LED_Task(void *pvParameters)
{
    // Lay thong tin LED tu tham so Task
    LED_Context *ctx = (LED_Context *)pvParameters;

    // Bien nhan du lieu tu Queue
    LED_Message msg;

    while (1)
    {
        // Cho nhan du lieu tu Queue
        // Task tam dung neu chua co du lieu moi
        if (xQueueReceive(ctx->queue,
                          &msg,
                          portMAX_DELAY) == pdPASS)
        {
            // Neu LED vua duoc chon
            if (msg.identify == 1)
            {
                // Tat LED trong 150ms
                PWM_SetDuty(ctx->channel, 0);
                vTaskDelay(ms_to_ticks(150));

                // Bat LED trong 150ms
                PWM_SetDuty(ctx->channel, 100);
                vTaskDelay(ms_to_ticks(150));
            }

            // Cap nhat do sang theo du lieu Queue
            PWM_SetDuty(ctx->channel, msg.duty);
        }
    }
}

/* ========== 12. TASK DOC 3 NUT NHAN ========== */

void Button_Task(void *pvParameters)
{
    // Mang chua chan cua 3 nut nhan
    const uint8_t pins[3] =
    {
        BTN_SELECT,
        BTN_UP,
        BTN_DOWN
    };

    // Mang quan ly trang thai 3 nut
    ButtonState buttons[3];

    // Luu do sang cua tung LED
    uint8_t brightness[LED_COUNT] =
    {
        INITIAL_DUTY,
        INITIAL_DUTY,
        INITIAL_DUTY
    };

    // LED dang duoc lua chon
    // active = 0: LED1
    // active = 1: LED2
    // active = 2: LED3
    uint8_t active = 0;

    uint8_t i;
    TickType_t now;

    (void)pvParameters;

    // Lay thoi gian hien tai cua FreeRTOS
    now = xTaskGetTickCount();

    // Khoi tao trang thai cua 3 nut nhan
    for (i = 0; i < 3; i++)
    {
        buttons[i].stable = Button_Read(pins[i]);
        buttons[i].last_raw = buttons[i].stable;
        buttons[i].changed_at = now;
    }

    // Vong lap chinh cua Task nut nhan
    while (1)
    {
        // Lay thoi gian hien tai
        now = xTaskGetTickCount();

        /* ----- NUT 1: CHUYEN LED ----- */

        if (Button_Pressed(&buttons[0],
                           pins[0],
                           now))
        {
            // Chuyen LED theo thu tu 1 -> 2 -> 3 -> 1
            active = (active + 1) % LED_COUNT;

            // Gui lenh cho LED moi duoc chon
            // LED nhay mot lan de bao hieu
            LED_Send(active,
                     brightness[active],
                     1);
        }

        /* ----- NUT 2: TANG DO SANG ----- */

        if (Button_Pressed(&buttons[1],
                           pins[1],
                           now))
        {
            // Tang 10% do sang cho LED dang chon
            if (brightness[active] <=
                100 - BRIGHTNESS_STEP)
            {
                brightness[active] += BRIGHTNESS_STEP;
            }
            else
            {
                // Do sang toi da la 100%
                brightness[active] = 100;
            }

            // Gui do sang moi qua Queue
            LED_Send(active,
                     brightness[active],
                     0);
        }

        /* ----- NUT 3: GIAM DO SANG ----- */

        if (Button_Pressed(&buttons[2],
                           pins[2],
                           now))
        {
            // Giam 10% do sang cho LED dang chon
            if (brightness[active] >= BRIGHTNESS_STEP)
            {
                brightness[active] -= BRIGHTNESS_STEP;
            }
            else
            {
                // Do sang toi thieu la 0%
                brightness[active] = 0;
            }

            // Gui do sang moi qua Queue
            LED_Send(active,
                     brightness[active],
                     0);
        }

        // Quet trang thai cac nut sau moi 10ms
        vTaskDelay(ms_to_ticks(10));
    }
}

/* ========== 13. HAM MAIN ========== */

int main(void)
{
    uint8_t i;

    // Buoc 1: Khoi tao GPIO cho LED va nut nhan
    GPIO_Config();

    // Buoc 2: Khoi tao PWM tan so 1kHz
    PWM_Config();

    // Buoc 3: Tao 3 Queue cho 3 Task LED
    for (i = 0; i < LED_COUNT; i++)
    {
        // Gan kenh PWM cho tung LED
        leds[i].channel = i;

        // Moi LED co mot Queue rieng
        // Moi Queue chua mot thong diep
        leds[i].queue = xQueueCreate(
            1,
            sizeof(LED_Message)
        );

        // Dung chuong trinh neu tao Queue that bai
        if (leds[i].queue == NULL)
        {
            while (1);
        }
    }

    // Buoc 4: Tao Task dieu khien LED 1
    if (xTaskCreate(
            LED_Task,
            "LED1",
            configMINIMAL_STACK_SIZE,
            &leds[0],
            1,
            NULL
        ) != pdPASS)
    {
        while (1);
    }

    // Buoc 5: Tao Task dieu khien LED 2
    if (xTaskCreate(
            LED_Task,
            "LED2",
            configMINIMAL_STACK_SIZE,
            &leds[1],
            1,
            NULL
        ) != pdPASS)
    {
        while (1);
    }

    // Buoc 6: Tao Task dieu khien LED 3
    if (xTaskCreate(
            LED_Task,
            "LED3",
            configMINIMAL_STACK_SIZE,
            &leds[2],
            1,
            NULL
        ) != pdPASS)
    {
        while (1);
    }

    // Buoc 7: Tao Task doc 3 nut nhan
    // Task nut nhan co do uu tien cao hon Task LED
    if (xTaskCreate(
            Button_Task,
            "BUTTON",
            configMINIMAL_STACK_SIZE + 64,
            NULL,
            2,
            NULL
        ) != pdPASS)
    {
        while (1);
    }

    // Buoc 8: Khoi dong FreeRTOS Scheduler
    vTaskStartScheduler();

    // Chuong trinh khong quay lai day neu chay dung
    while (1);
}