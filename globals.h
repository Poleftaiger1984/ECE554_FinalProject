#ifndef GLOBALS_H
#define GLOBALS_H

#define PERIPHERAL_BASE 0x3F000000

//UART REGISTERS
#define UART0_DR    *((volatile unsigned int*)(PERIPHERAL_BASE + 0x201000))
#define UART0_FR    *((volatile unsigned int*)(PERIPHERAL_BASE + 0x201018))
#define UART0_IMSC   (*(volatile unsigned int*)0x3F201038) //INTERUPT MASK
#define UART0_ICR    (*(volatile unsigned int*)0x3F201044) //INTERUPT CLEAR
#define UART0_RSRECR (*(volatile unsigned int*)(PERIPHERAL_BASE + 0x201004)) //RECIEVE STATUS

//PI INTERRUPT CONTROLLER
#define IRQ_ENABLE2  (*(volatile unsigned int*)0x3F00B214)
#define IRQ_PENDING2 (*(volatile unsigned int*)0x3F00B208)

//MAILBOX REGISTERS FOR GRAPHICS
#define MBOX_BASE    (PERIPHERAL_BASE + 0xB880)
#define MBOX_READ    *((volatile unsigned int*)(MBOX_BASE + 0x00))
#define MBOX_STATUS  *((volatile unsigned int*)(MBOX_BASE + 0x18))
#define MBOX_WRITE   *((volatile unsigned int*)(MBOX_BASE + 0x20))
#define MBOX_RESPONSE 0x80000000
#define MBOX_FULL     0x80000000
#define MBOX_EMPTY    0x40000000

volatile unsigned int __attribute__((aligned(16))) mbox[36];

//WATCHDOG
#define TIMER_BASE (PERIPHERAL_BASE + 0x3000)
#define TIMER_CLO  *(volatile unsigned int*)(TIMER_BASE + 0x04)

//COLOR DEFINITIONS (A-B-G-R FORMAT)
#define COLOR_RED       0xFF0000FF
#define COLOR_YELLOW    0xFF00FFFF
#define COLOR_GREEN     0xFF00FF00
#define COLOR_DARKGRAY  0xFF444444
#define COLOR_WHITE     0xFFFFFFFF
#define COLOR_BLACK     0xFF000000
#define COLOR_GREY      0xFF888888

//ANOMALY THRESHOLDS
#define WARN_THRESH             75
#define CRIT_THRESH             95
#define TEMP_WARN_THRESH        800 //80 CELCIUS
#define TEMP_CRIT_THRESH        950 //95 CELCIUS
#define DISK_IO_WARN_MBPS       100 //100 MB/s
#define DISK_IO_CRIT_MBPS       200 //200 MB/s
#define DISK_IO_MAX_MBPS        200 //scale for progress bar

typedef struct PCStats
{
    int m_cpu_usage;
    int m_cpu_temp_raw;
    
    int m_ram_usage;
    int m_ram_used_MB;
    int m_ram_total_MB;

    int m_disk_io_mbps;

    char m_taskNames[3][16];
    int m_task_cpu[3];
    int m_task_ram[3];

    char m_aiReport[256];
    int m_aiActive;
} PCStats;

#endif