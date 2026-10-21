TARGET = freertos_led

CC = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE = arm-none-eabi-size
STFLASH = st-flash

CPU = -mcpu=cortex-m3 -mthumb

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

INCLUDES = -I. \
           -ICMSIS/CM3/CoreSupport \
           -ICMSIS/CM3/DeviceSupport/ST/STM32F10x \
           -ISTM32F10x_StdPeriph_Driver/inc \
           -IFreeRTOS-Kernel/include \
           -IFreeRTOS-Kernel/portable/GCC/ARM_CM3

C_SOURCES = main.c \
            system_stm32f10x.c \
            STM32F10x_StdPeriph_Driver/src/stm32f10x_gpio.c \
            STM32F10x_StdPeriph_Driver/src/stm32f10x_rcc.c \
            FreeRTOS-Kernel/tasks.c \
            FreeRTOS-Kernel/queue.c \
            FreeRTOS-Kernel/list.c \
            FreeRTOS-Kernel/portable/GCC/ARM_CM3/port.c \
            FreeRTOS-Kernel/portable/MemMang/heap_4.c

ASM_SOURCES = startup.s

OBJECTS = $(C_SOURCES:.c=.o) \
          $(ASM_SOURCES:.s=.o)

LDFLAGS = $(CPU) \
          -Tlinker.ld \
          -Wl,--gc-sections \
          -Wl,-Map=$(TARGET).map \
          --specs=nosys.specs

# Mac dinh: build ELF va BIN
all: $(TARGET).elf $(TARGET).bin

# Bien dich file C
%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Bien dich startup
%.o: %.s
	$(CC) $(CPU) -c $< -o $@

# Lien ket ELF
$(TARGET).elf: $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	$(SIZE) $@

# Tao BIN
$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

# Kiem tra ST-LINK
probe:
	st-info --probe

# Nap va reset STM32
flash: $(TARGET).bin
	$(STFLASH) --reset write $(TARGET).bin 0x08000000

# Bien dich va nap
program: all flash

# Xoa file da bien dich
clean:
	rm -f $(OBJECTS) $(TARGET).elf $(TARGET).bin $(TARGET).map

.PHONY: all probe flash program clean