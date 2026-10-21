# BÀI THỰC HÀNH BUỔI 5 – STM32F103 / FreeRTOS / PWM / EXTI / UART / QUEUE

## 1. Giới thiệu

Bài tập thực hiện điều khiển độ sáng 3 LED bằng PWM và xử lý ngắt ngoài EXTI kết hợp truyền dữ liệu UART trên vi điều khiển **STM32F103**, sử dụng hệ điều hành thời gian thực **FreeRTOS** và thư viện **STM32 Standard Peripheral Library (STD)**.

Buổi thực hành gồm hai bài tập:

- **Bài tập 01:** Điều khiển độ sáng 3 LED bằng PWM tần số 1 kHz, sử dụng 4 task FreeRTOS và Queue để truyền dữ liệu.
- **Bài tập 02:** Đếm số lần nhấn nút bằng ngắt ngoài EXTI, truyền dữ liệu từ ISR đến task thông qua Queue và hiển thị kết quả trên Serial Terminal bằng UART.

**Mục tiêu:**

- Làm quen với lập trình đa nhiệm trên FreeRTOS.
- Tạo và quản lý các task bằng `xTaskCreate()`.
- Sử dụng PWM để điều khiển độ sáng LED.
- Sử dụng Queue để truyền dữ liệu giữa các task.
- Cấu hình và xử lý ngắt ngoài EXTI theo cạnh xuống.
- Sử dụng `xQueueSendFromISR()` để gửi dữ liệu từ ngắt.
- Sử dụng `portYIELD_FROM_ISR()` để yêu cầu chuyển ngữ cảnh khi cần thiết.
- Giao tiếp UART giữa STM32F103 và máy tính.

## 2. Phần cứng và phần mềm

**Phần cứng:**

- Vi điều khiển STM32F103C8T6.
- 3 LED và 3 điện trở hạn dòng 220Ω–330Ω.
- 3 nút nhấn cho Bài 1 và 1 nút nhấn cho Bài 2.
- Bộ nạp/debug ST-Link.
- Module USB-UART TTL 3,3 V để giao tiếp với máy tính.
- Dây kết nối và breadboard.

**Phần mềm và thư viện:**

- Ubuntu Linux chạy trên VMware.
- Visual Studio Code.
- STM32 Standard Peripheral Library (STD).
- CMSIS.
- FreeRTOS Kernel.
- ARM GNU Toolchain (`arm-none-eabi-gcc`).
- Makefile để biên dịch chương trình.
- Minicom để quan sát dữ liệu UART.

## 3. Cấu hình phần cứng

### 3.1. Bài tập 01 – Điều khiển độ sáng 3 LED

| Thiết bị | Chân GPIO | Chức năng |
|---|---|---|
| LED 1 | PA0 – TIM2_CH1 | Điều khiển PWM LED 1 |
| LED 2 | PA1 – TIM2_CH2 | Điều khiển PWM LED 2 |
| LED 3 | PA2 – TIM2_CH3 | Điều khiển PWM LED 3 |
| Nút 1 | PB0 | Chuyển LED đang điều khiển |
| Nút 2 | PB1 | Tăng độ sáng LED |
| Nút 3 | PB10 | Giảm độ sáng LED |

Các chân PA0, PA1, PA2 được cấu hình ở chế độ Alternate Function Push-Pull để xuất tín hiệu PWM từ Timer 2.

**Thông số PWM:**

- Tần số: 1 kHz.
- Chu kỳ PWM: 1 ms.
- Độ sáng ban đầu: 50%.
- Bước thay đổi độ sáng: 10%.
- Giới hạn độ sáng: 0%–100%.

Các nút nhấn sử dụng chế độ Input Pull-Up và được nối xuống GND.

### 3.2. Bài tập 02 – Ngắt ngoài EXTI và UART

| Thiết bị | Chân GPIO | Chức năng |
|---|---|---|
| Nút nhấn | PB0 – EXTI0 | Tạo ngắt cạnh xuống |
| UART TX | PA9 – USART1_TX | Gửi dữ liệu lên máy tính |
| UART RX | PA10 – USART1_RX | Nhận dữ liệu UART |
| LED trạng thái | PC13 | Đổi trạng thái khi có sự kiện |

**Thông số UART:**

- Baudrate: 115200.
- Data bits: 8.
- Stop bits: 1.
- Parity: None.

Nút nhấn PB0 được cấu hình ngắt ngoài theo cạnh xuống (Falling Edge), kết hợp cơ chế chống dội để hạn chế ghi nhận nhiều ngắt cho một lần nhấn.

## 4. Nguyên lý hoạt động

### 4.1. Bài tập 01 – Điều khiển PWM bằng FreeRTOS

**Khởi tạo GPIO và PWM**

Chương trình cấu hình PA0, PA1, PA2 làm đầu ra PWM và sử dụng TIM2 với ba kênh độc lập.

Độ sáng LED được điều chỉnh bằng cách thay đổi Duty Cycle của tín hiệu PWM.

Công thức:

**Duty Cycle = (Thời gian mức HIGH / Chu kỳ PWM) × 100%**

Trong đó:

- Duty Cycle: Tỷ lệ thời gian tín hiệu ở mức HIGH.
- Chu kỳ PWM: 1 ms ứng với tần số 1 kHz.
- Duty Cycle càng lớn thì LED càng sáng đối với cách nối LED từ GPIO qua điện trở xuống GND.

**Tạo các task FreeRTOS**

Chương trình sử dụng `xTaskCreate()` để tạo 4 task:

- Task LED1: Điều khiển PWM của LED 1.
- Task LED2: Điều khiển PWM của LED 2.
- Task LED3: Điều khiển PWM của LED 3.
- Task BUTTON: Đọc và xử lý 3 nút nhấn.

Mỗi LED có một Queue riêng để nhận dữ liệu điều khiển.

**Xử lý nút nhấn**

Task BUTTON kiểm tra trạng thái 3 nút theo chu kỳ 10 ms, kết hợp chống dội 30 ms.

- PB0: Chuyển LED theo thứ tự LED1 → LED2 → LED3 → LED1.
- PB1: Tăng độ sáng LED đang chọn 10%.
- PB10: Giảm độ sáng LED đang chọn 10%.

Giá trị độ sáng của mỗi LED được lưu riêng biệt.

**Truyền dữ liệu qua Queue**

Khi có sự kiện nhấn nút, Task BUTTON gửi dữ liệu mới đến Queue tương ứng bằng `xQueueOverwrite()`.

Các Task LED chờ dữ liệu bằng `xQueueReceive()`. Khi nhận được dữ liệu, task cập nhật giá trị thanh ghi CCR của Timer 2 để thay đổi độ sáng.

### 4.2. Bài tập 02 – Xử lý EXTI, Queue và UART

**Khởi tạo ngắt ngoài**

PB0 được cấu hình Input Pull-Up và liên kết với EXTI0.

Khi nhấn nút, tín hiệu chuyển từ HIGH xuống LOW, tạo ngắt cạnh xuống.

**Xử lý ngắt EXTI0**

Hàm `EXTI0_IRQHandler()` thực hiện:

1. Kiểm tra trạng thái ngắt EXTI0.
2. Xóa cờ ngắt.
3. Tăng biến đếm số lần nhấn nút.
4. Gửi giá trị biến đếm vào Queue bằng `xQueueSendFromISR()`.
5. Sử dụng `portYIELD_FROM_ISR()` để yêu cầu chuyển ngữ cảnh nếu cần thiết.

**Truyền dữ liệu bằng UART**

Chương trình tạo một task UART để nhận dữ liệu từ Queue.

Task sử dụng `xQueueReceive()` với thời gian chờ `portMAX_DELAY`, giúp task chuyển sang trạng thái Blocked khi chưa có dữ liệu.

Sau khi nhận giá trị đếm, task định dạng chuỗi và truyền qua USART1 đến Serial Terminal của máy tính.

**Chống dội nút nhấn**

Chương trình sử dụng thời gian chống dội khoảng 200 ms để hạn chế hiện tượng một lần nhấn bị ghi nhận thành nhiều lần.

## 5. Kết quả mong đợi

### 5.1. Bài tập 01

Sau khi chương trình hoạt động:

- Ba LED ban đầu sáng ở mức 50%.
- Nhấn PB0 để lựa chọn LED cần điều khiển.
- LED vừa được chọn nháy để báo hiệu.
- Nhấn PB1 làm tăng độ sáng LED đang chọn 10%.
- Nhấn PB10 làm giảm độ sáng LED đang chọn 10%.
- Các LED vẫn duy trì độ sáng riêng khi chuyển đổi giữa các LED.

Ba task LED hoạt động độc lập về mặt logic và nhận lệnh điều khiển thông qua FreeRTOS Queue.

### 5.2. Bài tập 02

Khi kết nối UART với máy tính và mở Serial Terminal, mỗi lần nhấn PB0 hợp lệ sẽ làm tăng biến đếm.

Dữ liệu dự kiến hiển thị:

```text
DTMT01_N11: Button Pressed = 1
DTMT01_N11: Button Pressed = 2
DTMT01_N11: Button Pressed = 3
DTMT01_N11: Button Pressed = 4
DTMT01_N11: Button Pressed = 5
```

Giá trị biến đếm được truyền từ hàm ngắt EXTI0 đến Task UART thông qua Queue, thay vì thực hiện truyền chuỗi UART trực tiếp trong ISR.

## 6. Hướng dẫn chạy chương trình

**Bước 1: Chuẩn bị project**

Đảm bảo hai thư mục `Bai1` và `Bai2` đã có đầy đủ:

- Thư viện CMSIS.
- STM32 Standard Peripheral Library.
- FreeRTOS Kernel.
- `FreeRTOSConfig.h`.
- `startup.s`.
- `linker.ld`.
- `Makefile`.
- `main.c`.

**Bước 2: Biên dịch Bài 1**

```bash
cd ~/Downloads/BareMetal_Week05/Bai1
make clean
make
```

**Bước 3: Biên dịch Bài 2**

```bash
cd ~/Downloads/BareMetal_Week05/Bai2
make clean
make
```

**Bước 4: Kiểm tra ST-Link**

```bash
make probe
```

**Bước 5: Nạp chương trình**

```bash
make flash
```

Hoặc:

```bash
make program
```

**Bước 6: Quan sát UART của Bài 2**

Mở Minicom với tốc độ 115200 baud:

```bash
sudo minicom -D /dev/ttyUSB0 -b 115200
```

Nhấn nút PB0 trên mạch STM32F103 và quan sát giá trị biến đếm được truyền lên Terminal.

Lưu ý: Đường dẫn `/dev/ttyUSB0` có thể thay đổi tùy thiết bị USB-UART và cấu hình VMware.

## 7. Các hàm chính được sử dụng

| Hàm | Chức năng |
|---|---|
| `RCC_APB2PeriphClockCmd()` | Bật clock cho ngoại vi |
| `GPIO_Init()` | Khởi tạo GPIO |
| `TIM_TimeBaseInit()` | Cấu hình bộ đếm Timer |
| `TIM_OC1Init()` | Cấu hình kênh PWM 1 |
| `TIM_SetCompare1()` | Điều chỉnh Duty Cycle PWM |
| `xTaskCreate()` | Tạo task FreeRTOS |
| `vTaskDelay()` | Tạm dừng task theo thời gian Tick |
| `xQueueCreate()` | Tạo Queue |
| `xQueueOverwrite()` | Ghi đè dữ liệu mới nhất trong Queue |
| `xQueueSendFromISR()` | Gửi dữ liệu từ ISR vào Queue |
| `xQueueReceive()` | Nhận dữ liệu từ Queue |
| `EXTI_Init()` | Cấu hình ngắt ngoài |
| `EXTI_GetITStatus()` | Kiểm tra trạng thái ngắt |
| `EXTI_ClearITPendingBit()` | Xóa cờ ngắt |
| `USART_Init()` | Khởi tạo UART |
| `USART_SendData()` | Gửi dữ liệu qua UART |
| `portYIELD_FROM_ISR()` | Yêu cầu chuyển ngữ cảnh từ ISR |
| `vTaskStartScheduler()` | Khởi động bộ lập lịch FreeRTOS |

Các hàm STD trong bảng thể hiện các API ngoại vi tương ứng; tùy phiên bản `main.c`, một số chức năng có thể được thực hiện trực tiếp qua thanh ghi STM32 thay vì gọi hàm thư viện.

## 8. Kết luận

Bài thực hành Buổi 5 minh họa cách sử dụng hệ điều hành thời gian thực FreeRTOS trong điều khiển ngoại vi và xử lý sự kiện trên STM32F103.

**Bài tập 01** giúp hiểu rõ cơ chế lập lịch đa nhiệm thông qua việc tạo 3 task điều khiển LED, một task xử lý nút nhấn và truyền dữ liệu giữa các task bằng Queue. Đồng thời, bài tập củng cố kiến thức về Timer và điều chế độ rộng xung PWM.

**Bài tập 02** giúp làm quen với ngắt ngoài EXTI, xử lý sự kiện từ phần cứng, truyền dữ liệu từ ISR vào Queue và sử dụng UART để giao tiếp với máy tính.

Thông qua hai bài tập, người học nắm được các kỹ thuật lập trình hệ thống nhúng thời gian thực, quản lý task, đồng bộ dữ liệu và kết hợp FreeRTOS với các ngoại vi của STM32F103.

**Bài thực hành Buổi 5 – STM32F103 / FreeRTOS / PWM / EXTI / UART / Queue – Nhóm 11**
