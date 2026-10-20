.section ".text.boot"
.global _start

_start:
    @ The Vector Table
    ldr pc, =reset_handler
    ldr pc, =undefined_handler
    ldr pc, =swi_handler
    ldr pc, =prefetch_handler
    ldr pc, =data_handler
    ldr pc, =unused_handler
    ldr pc, =irq_handler_stub @ 0x18: HardwareInterrupts
    ldr pc, =fiq_handler

reset_handler:
    ldr r0, =_start
    mcr p15, 0, r0, c12, c0, 0

    mrc p15, 0, r1, c0, c0, 5
    and r1, r1, #3
    cmp r1, #0
    bne halt

    ldr sp, =__stack_top

    mrs r0, cpsr
    bic r1, r0, #0x1F
    orr r1, r1, #0x12
    msr cpsr_c, r1
    
    ldr sp, =__stack_top
    sub sp, sp, #4096 
    
    bic r1, r0, #0x1F
    orr r1, r1, #0x13
    msr cpsr_c, r1

    ldr r4, =__bss_start
    ldr r9, =__bss_end
    mov r5, #0
clear_bss:
    cmp r4, r9
    bge run_kernel
    str r5, [r4]
    add r4, r4, #4
    b clear_bss

run_kernel:
    bl kernel_main

halt:
    wfe
    b halt

.global irq_handler_stub
.extern c_uartIRQHandler

irq_handler_stub:
    SUB LR, LR, #4
    STMFD SP!, {R0-R12, LR}
    BL c_irqDispatcher
    LDMFD SP!, {R0-R12, PC}^

@unused handlers
undefined_handler: b undefined_handler
swi_handler: b swi_handler
prefetch_handler: b prefetch_handler
data_handler: b data_handler
unused_handler: b unused_handler
fiq_handler: b fiq_handler