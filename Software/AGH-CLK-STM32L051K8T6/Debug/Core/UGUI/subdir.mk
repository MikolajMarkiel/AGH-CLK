################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/UGUI/ugui.c \
../Core/UGUI/ugui_button.c \
../Core/UGUI/ugui_checkbox.c \
../Core/UGUI/ugui_image.c \
../Core/UGUI/ugui_progress.c \
../Core/UGUI/ugui_sim.c \
../Core/UGUI/ugui_textbox.c 

OBJS += \
./Core/UGUI/ugui.o \
./Core/UGUI/ugui_button.o \
./Core/UGUI/ugui_checkbox.o \
./Core/UGUI/ugui_image.o \
./Core/UGUI/ugui_progress.o \
./Core/UGUI/ugui_sim.o \
./Core/UGUI/ugui_textbox.o 

C_DEPS += \
./Core/UGUI/ugui.d \
./Core/UGUI/ugui_button.d \
./Core/UGUI/ugui_checkbox.d \
./Core/UGUI/ugui_image.d \
./Core/UGUI/ugui_progress.d \
./Core/UGUI/ugui_sim.d \
./Core/UGUI/ugui_textbox.d 


# Each subdirectory must supply rules for building sources it contributes
Core/UGUI/%.o Core/UGUI/%.su: ../Core/UGUI/%.c Core/UGUI/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m0plus -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32L051xx -c -I../Core/Inc -I../Drivers/STM32L0xx_HAL_Driver/Inc -I../Drivers/STM32L0xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32L0xx/Include -I../Drivers/CMSIS/Include -I"D:/Projekty/AGH-CLK-01/Software/AGH-CLK-STM32L051K8T6/Core/UGUI" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Core-2f-UGUI

clean-Core-2f-UGUI:
	-$(RM) ./Core/UGUI/ugui.d ./Core/UGUI/ugui.o ./Core/UGUI/ugui.su ./Core/UGUI/ugui_button.d ./Core/UGUI/ugui_button.o ./Core/UGUI/ugui_button.su ./Core/UGUI/ugui_checkbox.d ./Core/UGUI/ugui_checkbox.o ./Core/UGUI/ugui_checkbox.su ./Core/UGUI/ugui_image.d ./Core/UGUI/ugui_image.o ./Core/UGUI/ugui_image.su ./Core/UGUI/ugui_progress.d ./Core/UGUI/ugui_progress.o ./Core/UGUI/ugui_progress.su ./Core/UGUI/ugui_sim.d ./Core/UGUI/ugui_sim.o ./Core/UGUI/ugui_sim.su ./Core/UGUI/ugui_textbox.d ./Core/UGUI/ugui_textbox.o ./Core/UGUI/ugui_textbox.su

.PHONY: clean-Core-2f-UGUI

