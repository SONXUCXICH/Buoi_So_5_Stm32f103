# ========== CAU HINH DU AN ==========

TARGET = bai2_std_n11

CC = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE = arm-none-eabi-size
STFLASH = st-flash

# ========== CAU HINH CPU ==========

CPU = -mcpu=cortex-m3 -mthumb

# ========== CAU HINH BIEN DICH ==========

CFLAGS = $(CPU) \
         -Wall \
         -Wextra \
         -O0 \
         -g3 \
         -std=gnu11 \
         -ffunction-sections \
         -fdata-sections \
         -DSTM32F10X_MD \
         -DUSE_STDPERIPH_DRIVER

# ========== DUONG DAN THU VIEN ==========

INCLUDES = -I. \
           -ICMSIS/CM3/CoreSupport \
           -ICMSIS/CM3/DeviceSupport/ST/STM32F10x \
           -ISTM32F10x_StdPeriph_Driver/inc \
           -IFreeRTOS-Kernel/include \
           -IFreeRTOS-Kernel/portable/GCC/ARM_CM3

# ========== DANH SACH FILE C ==========

C_SOURCES = main.c \
            system_stm32f10x.c \
            STM32F10x_StdPeriph_Driver/src/stm32f10x_gpio.c \
            STM32F10x_StdPeriph_Driver/src/stm32f10x_rcc.c \
            STM32F10x_StdPeriph_Driver/src/stm32f10x_exti.c \
            STM32F10x_StdPeriph_Driver/src/stm32f10x_usart.c \
            STM32F10x_StdPeriph_Driver/src/stm32f10x_flash.c \
            STM32F10x_StdPeriph_Driver/src/misc.c \
            FreeRTOS-Kernel/tasks.c \
            FreeRTOS-Kernel/queue.c \
            FreeRTOS-Kernel/list.c \
            FreeRTOS-Kernel/portable/GCC/ARM_CM3/port.c \
            FreeRTOS-Kernel/portable/MemMang/heap_4.c

# ========== FILE KHOI DONG ==========

ASM_SOURCES = startup.s

# ========== DANH SACH OBJECT ==========

OBJECTS = $(C_SOURCES:.c=.o) \
          $(ASM_SOURCES:.s=.o)

# ========== CAU HINH LINKER ==========

LDFLAGS = $(CPU) \
          -Tlinker.ld \
          -Wl,--gc-sections \
          -Wl,-Map=$(TARGET).map \
          --specs=nosys.specs

# ========== BUILD ELF VA BIN ==========

all: $(TARGET).elf $(TARGET).bin

# Bien dich cac file C
%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Bien dich file Assembly
%.o: %.s
	$(CC) $(CPU) -c $< -o $@

# Lien ket cac object tao file ELF
$(TARGET).elf: $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	$(SIZE) $@

# Chuyen file ELF sang BIN
$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

# Kiem tra ST-LINK
probe:
	st-info --probe

# Nap chuong trinh vao STM32 va Reset
flash: $(TARGET).bin
	$(STFLASH) --reset write $(TARGET).bin 0x08000000

# Bien dich va nap chuong trinh
program: all flash

# Xoa cac file da bien dich
clean:
	rm -f $(OBJECTS) $(TARGET).elf $(TARGET).bin $(TARGET).map

.PHONY: all probe flash program clean