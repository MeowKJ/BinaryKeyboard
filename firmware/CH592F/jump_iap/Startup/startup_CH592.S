	.section	.init,"ax",@progbits
	.global	_start
	.align	1
_start:
	j	handle_reset

	.section    .vector,"ax",@progbits
	.align  1
_vector_base:
	.option norvc;

	.word   0
	.word   0
	.word   NMI_Handler
	.word   HardFault_Handler
	.word   0xF5F9BDA9
	.word   Ecall_M_Mode_Handler
	.word   0
	.word   0
	.word   Ecall_U_Mode_Handler
	.word   Break_Point_Handler
	.word   0
	.word   0
	.word   SysTick_Handler
	.word   0
	.word   SW_Handler
	.word   0
	.word   TMR0_IRQHandler
	.word   GPIOA_IRQHandler
	.word   GPIOB_IRQHandler
	.word   SPI0_IRQHandler
	.word   BB_IRQHandler
	.word   LLE_IRQHandler
	.word   USB_IRQHandler
	.word   0
	.word   TMR1_IRQHandler
	.word   TMR2_IRQHandler
	.word   UART0_IRQHandler
	.word   UART1_IRQHandler
	.word   RTC_IRQHandler
	.word   ADC_IRQHandler
	.word   I2C_IRQHandler
	.word   PWMX_IRQHandler
	.word   TMR3_IRQHandler
	.word   UART2_IRQHandler
	.word   UART3_IRQHandler
	.word   WDOG_BAT_IRQHandler

	.option rvc;

	.section    .vector_handler, "ax", @progbits
	.weak   NMI_Handler
	.weak   HardFault_Handler
	.weak   Ecall_M_Mode_Handler
	.weak   Ecall_U_Mode_Handler
	.weak   Break_Point_Handler
	.weak   SysTick_Handler
	.weak   SW_Handler
	.weak   TMR0_IRQHandler
	.weak   GPIOA_IRQHandler
	.weak   GPIOB_IRQHandler
	.weak   SPI0_IRQHandler
	.weak   BB_IRQHandler
	.weak   LLE_IRQHandler
	.weak   USB_IRQHandler
	.weak   TMR1_IRQHandler
	.weak   TMR2_IRQHandler
	.weak   UART0_IRQHandler
	.weak   UART1_IRQHandler
	.weak   RTC_IRQHandler
	.weak   ADC_IRQHandler
	.weak   I2C_IRQHandler
	.weak   PWMX_IRQHandler
	.weak   TMR3_IRQHandler
	.weak   UART2_IRQHandler
	.weak   UART3_IRQHandler
	.weak   WDOG_BAT_IRQHandler

NMI_Handler:
HardFault_Handler:
Ecall_M_Mode_Handler:
Ecall_U_Mode_Handler:
Break_Point_Handler:
SysTick_Handler:
SW_Handler:
TMR0_IRQHandler:
GPIOA_IRQHandler:
GPIOB_IRQHandler:
SPI0_IRQHandler:
BB_IRQHandler:
LLE_IRQHandler:
USB_IRQHandler:
TMR1_IRQHandler:
TMR2_IRQHandler:
UART0_IRQHandler:
UART1_IRQHandler:
RTC_IRQHandler:
ADC_IRQHandler:
I2C_IRQHandler:
PWMX_IRQHandler:
TMR3_IRQHandler:
UART2_IRQHandler:
UART3_IRQHandler:
WDOG_BAT_IRQHandler:
1:
	j 1b

	.section	.handle_reset,"ax",@progbits
	.weak	handle_reset
	.align	1
handle_reset:
	j	0x6D000
