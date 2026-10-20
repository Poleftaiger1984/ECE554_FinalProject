#include "render.h"
#include "globals.h"
#include "ringBuffer.h"

/*
    Standard C Library Implementations
*/

void *memset (void *dest, int val, unsigned int len)
{
    unsigned char *ptr = dest;
    while (len-- > 0)
    {
        *ptr++ = (unsigned char)val;
    }
    return dest;
}

void str_cpy(char *dest, const char *src)
{
    while (*src)
    {
        *dest++ = *src++;
    }
    *dest = '\0';
}

int str_cmp(const char *s1, const char *s2)
{
    while (*s1 && (*s1 == *s2))
    {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

void *memcpy(void *dest, const void *src, unsigned int n)
{
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;
    while (n--)
    {
        *d++ = *s++;
    }
    return dest;
}

/*
    End Standard C Library Implementations
*/

/*
    Display Rendering Functions
*/

void drawDisplay(PCStats *statistics, int isConnected)
{
    //HEADER
    if (isConnected)
    {
        drawString(20, 20, "PI MONITOR          [   CONNECTED   ]", COLOR_GREEN, COLOR_BLACK); // Green
    }
    else
    {
        drawString(20, 20, "PI MONITOR          [ DISCONNECTED! ]", COLOR_RED, COLOR_BLACK); // Red
    }
    drawRect(20, 35, 600, 2, COLOR_DARKGRAY); //dark Gray

    //CPU SECTION
    drawString(20, 45, "CPU", COLOR_WHITE, COLOR_BLACK); //white
    if(statistics->m_cpu_usage < WARN_THRESH)
    {
        drawProgressBar(20, 60, 120, 15, statistics->m_cpu_usage, COLOR_GREEN, 0xFF333333); 
    }
    else if(statistics->m_cpu_usage > WARN_THRESH && statistics->m_cpu_usage < CRIT_THRESH)
    {
        drawProgressBar(20, 60, 120, 15, statistics->m_cpu_usage, COLOR_YELLOW, 0xFF333333); 
    }
    else if(statistics->m_cpu_usage >= CRIT_THRESH)
    {
        drawProgressBar(20, 60, 120, 15, statistics->m_cpu_usage, COLOR_RED, 0xFF333333); 
    }
    int next_x = drawNumber(150, 63, statistics->m_cpu_usage, COLOR_WHITE, COLOR_BLACK);        
    drawString(next_x, 63, "%   ", COLOR_WHITE, COLOR_BLACK);

    next_x = drawNumber(20, 80, statistics->m_cpu_temp_raw / 10, COLOR_YELLOW, COLOR_BLACK);
    drawString(next_x, 80, ".", COLOR_YELLOW, COLOR_BLACK);
    next_x += 8; // move past the decimal
    next_x = drawNumber(next_x, 80, statistics->m_cpu_temp_raw % 10, COLOR_YELLOW, COLOR_BLACK);
    drawString(next_x, 80, " C   ", COLOR_YELLOW, COLOR_BLACK); //cyan

    //VERTICAL DIVIDER
    drawRect(210, 45, 2, 50, COLOR_DARKGRAY); //dark Gray

    //RAM SECTION
    drawString(230, 45, "RAM", COLOR_WHITE, COLOR_BLACK); //white
    if(statistics->m_ram_usage < WARN_THRESH)
    {
        drawProgressBar(230, 60, 120, 15, statistics->m_ram_usage, COLOR_GREEN, 0xFF333333);
    }
    else if(statistics->m_ram_usage > WARN_THRESH && statistics->m_ram_usage < CRIT_THRESH)
    {
        drawProgressBar(230, 60, 120, 15, statistics->m_ram_usage, COLOR_YELLOW, 0xFF333333);
    }
    else if(statistics->m_ram_usage >= CRIT_THRESH)
    {
        drawProgressBar(230, 60, 120, 15, statistics->m_ram_usage, COLOR_RED, 0xFF333333);
    }
    next_x = drawNumber(360, 63, statistics->m_ram_usage, COLOR_WHITE, COLOR_BLACK);
    drawString(next_x, 63, "%   ", COLOR_WHITE, COLOR_BLACK);
    next_x = drawNumber(230, 80, statistics->m_ram_used_MB, COLOR_YELLOW, COLOR_BLACK);
    drawString(next_x, 80, " / ", COLOR_YELLOW, COLOR_BLACK);
    next_x = drawNumber(next_x + 24, 80, statistics->m_ram_total_MB, COLOR_YELLOW, COLOR_BLACK); // +24 bypasses the " / "
    drawString(next_x, 80, " GB   ", COLOR_YELLOW, COLOR_BLACK); //cyan

    //VERTICAL DIVIDER
    drawRect(420, 45, 2, 50, COLOR_DARKGRAY); //dark Gray

    //DISK SECTION
    drawString(440, 45, "DISK I/O", COLOR_WHITE, COLOR_BLACK); //white
    {
        int fill = statistics->m_disk_io_mbps * 100 / DISK_IO_MAX_MBPS;
        if (fill > 100) fill = 100;
        unsigned int bar_color = (statistics->m_disk_io_mbps >= DISK_IO_CRIT_MBPS) ? COLOR_RED :
                                 (statistics->m_disk_io_mbps >= DISK_IO_WARN_MBPS) ? COLOR_YELLOW : COLOR_GREEN;
        drawProgressBar(440, 60, 120, 15, fill, bar_color, 0xFF333333);
    }
    next_x = drawNumber(440, 80, statistics->m_disk_io_mbps, COLOR_YELLOW, COLOR_BLACK);
    drawString(next_x, 80, " MB/s   ", COLOR_YELLOW, COLOR_BLACK);

    //LOWER DIVIDER
    drawRect(20, 105, 600, 2, COLOR_DARKGRAY); //dark Gray Line

    //ALARMS
    char *alarmText = "";
    unsigned int alarmColor = COLOR_DARKGRAY;
    unsigned int textColor = COLOR_WHITE;
    int alarmActive = 1;

    //FATAL PRIORITY
    if (!isConnected)
    {
        alarmText = "[FTL] PC LINK LOST: Check USB Connection / Host App";
        alarmColor = COLOR_RED;
    }
    //CRITICAL PRIORITY
    else if (statistics->m_cpu_temp_raw >= TEMP_CRIT_THRESH)
    {
        alarmText = "[CRT] PC THERMAL CRITICAL: Throttling Detected";
        alarmColor = COLOR_RED;
    }
    else if (statistics->m_cpu_usage > CRIT_THRESH)
    {
        alarmText = "[CRT] PC CPU OVERLOAD: Monitoring Unreliable";
        alarmColor = COLOR_RED;
    }
    else if (statistics->m_ram_usage >= CRIT_THRESH)
    {
        alarmText = "[CRT] PC RAM DEPLETED: System Instability Risk";
        alarmColor = COLOR_RED;
    }
    else if (statistics->m_disk_io_mbps >= DISK_IO_CRIT_MBPS)
    {
        alarmText = "[CRT] PC DISK THRASH: I/O Saturation Risk";
        alarmColor = COLOR_RED;
    }
    //WARNING PRIORITY
    else if (statistics->m_cpu_temp_raw >= TEMP_WARN_THRESH)
    {
        alarmText = "[WRN] PC THERMAL RISE: Check PC Airflow";
        alarmColor = COLOR_YELLOW;
        textColor = COLOR_BLACK;
    }
    else if (statistics->m_cpu_usage > WARN_THRESH)
    {
        alarmText = "[WRN] PC CPU LOAD HIGH: Monitoring Jitter Likely";
        alarmColor = COLOR_YELLOW;
        textColor = COLOR_BLACK;
    }
    else if (statistics->m_ram_usage >= WARN_THRESH)
    {
        alarmText = "[WRN] PC RAM SATURATED: Background Tasks Slow";
        alarmColor = COLOR_YELLOW;
        textColor = COLOR_BLACK;
    }
    else if (statistics->m_disk_io_mbps >= DISK_IO_WARN_MBPS)
    {
        alarmText = "[WRN] PC DISK BUSY: High I/O Activity";
        alarmColor = COLOR_YELLOW;
        textColor = COLOR_BLACK;
    }
    //NO ALARMS
    else
    {
        alarmActive = 0;
    }

    //RENDER ALARM BANNER
    drawRect(20, 120, 600, 20, COLOR_BLACK); //background clear

    if(alarmActive)
    {
        drawRect(20, 120, 600, 20, alarmColor);
        drawString(25, 126, alarmText, textColor, alarmColor);
    }
    else
    {
        drawString(25, 126, "SYSTEM NOMINAL - NO ANOMALIES DETECTED", COLOR_GREY, COLOR_BLACK);
    }

    //TOP BACKGROUND TASK
    //DIVIDER
    drawRect(20, 150, 600, 2, COLOR_DARKGRAY);

    //SECTION HEADER
    drawString(20, 160, "TOP RESOURCE HOGGING TASKS: ", COLOR_GREY, COLOR_BLACK);
    drawRect(20, 175, 600, 15, COLOR_BLACK);

    if(statistics->m_taskNames[0][0] == '\0')
    {
        drawString(20, 175, "AWAITING DATA...", COLOR_WHITE, COLOR_BLACK);
    }
    else
    {
        //loop through 3 tasks
        for (int i = 0; i < 3; i++)
        {
            int y_pos = 175 + (i * 20); //shifts down 20 pixels from the last

            if(str_cmp(statistics->m_taskNames[i], "NONE") == 0 || statistics->m_taskNames[i][0] == '\0')
            {
                continue;
            }

            drawString(20, y_pos, statistics->m_taskNames[i], COLOR_YELLOW, COLOR_BLACK);
            int next_x = 130;

            drawString(next_x, y_pos, "CPU: ", COLOR_WHITE, COLOR_BLACK);
            next_x += 40;
            next_x = drawNumber(next_x, y_pos, statistics->m_task_cpu[i], COLOR_WHITE, COLOR_BLACK);
            drawString(next_x, y_pos, "%   ", COLOR_WHITE, COLOR_BLACK);

            next_x = 240;
            drawString(next_x, y_pos, "RAM: ", COLOR_WHITE, COLOR_BLACK);
            next_x += 40;
            next_x = drawNumber(next_x, y_pos, statistics->m_task_ram[i], COLOR_WHITE, COLOR_BLACK);
            drawString(next_x, y_pos, " MB   ", COLOR_WHITE, COLOR_BLACK);
        } 
    }
}

void drawAIDisplay(PCStats *statistics)
{
    drawRect(20, 260, 600, 200, COLOR_RED);
    drawRect(22, 262, 596, 196, COLOR_BLACK);

    drawString(180, 280, "!!! AI ANOMALY REPORT !!!", COLOR_RED, COLOR_BLACK);
    drawRect(40, 300, 560, 2, COLOR_RED);

    char lineBuffer[75];
    int line_index = 0;
    int current_y = 315;

    for (int i = 0; statistics->m_aiReport[i] != '\0'; i++)
    {
        lineBuffer[line_index++] = statistics->m_aiReport[i];

        if (line_index >= 65)
        {
            int space_index = line_index - 1;
            while (space_index > 0 && lineBuffer[space_index] != ' ')
            {
                space_index--;
            }

            if (space_index > 0)
            {
                lineBuffer[space_index] = '\0';
                drawString(40, current_y, lineBuffer, COLOR_WHITE, COLOR_BLACK);
                current_y += 20;

                int remaining = line_index - 1 - space_index;
                for (int j = 0; j < remaining; j++)
                {
                    lineBuffer[j] = lineBuffer[space_index + 1 + j];
                }
                line_index = remaining;
            }
            else{
                lineBuffer[line_index] = '\0';
                drawString(40, current_y, lineBuffer, COLOR_WHITE, COLOR_BLACK);
                current_y += 20;
                line_index = 0;
            }
        }
    }

    if (line_index > 0)
    {
        lineBuffer[line_index] = '\0';
        drawString(40, current_y, lineBuffer, COLOR_WHITE, COLOR_BLACK);
    }

    drawString(40, 420, "Returning to Normal Statistics in 10 Seconds...", COLOR_GREY, COLOR_BLACK);
}

/*
    End Display Rendering Functions
*/

/*
    UART Functions
*/

void uart_putc(unsigned char c) 
{
    while (UART0_FR & (1 << 5)) {}
    UART0_DR = c;
}

void uart_puts(const char* str) 
{
    for (int i = 0; str[i] != '\0'; i++) uart_putc(str[i]);
}

/*
    End UART Functions
*/

/*
    Mailbox Functions
*/

void mboxWrite(unsigned char ch, unsigned int data) 
{
    while (MBOX_STATUS & MBOX_FULL);
    MBOX_WRITE = (data & 0xFFFFFFF0) | (ch & 0xF);
}

unsigned int mboxRead(unsigned char ch) 
{
    while (1) 
    {
        while (MBOX_STATUS & MBOX_EMPTY);
        unsigned int res = MBOX_READ;
        if ((res & 0xF) == ch) return res & 0xFFFFFFF0;
    }
}

/*
    End Mailbox Functions
*/

/*
    Helper Functions
*/

void parseString(const char *string, int *index, char *dest, int max_len)
{
    int d_index = 0;
    while (string[*index] != '\0' && string[*index] != ',' && string[*index] != '\n' && string[*index] != '\r')
    {
        if (d_index < max_len - 1)
        {
            dest[d_index] = string[*index];
            d_index++;
        }
        (*index)++;
    }
    dest[d_index] = '\0';
}

unsigned int getSystemTime()
{
    return TIMER_CLO;
}

void dummyDelay(int cycles)
{
    for (volatile int i = 0; i < cycles; i++) {}
}

int parseNum (const char *string, int *index)
{
    int result = 0;

    while (string[*index] != '\0' && (string[*index] < '0' || string[*index] > '9'))
    {
        (*index)++;
    }

    while (string[*index] >= '0' && string[*index] <= '9')
    {
        result = (result * 10) + (string[*index] - '0');
        (*index)++;
    }
    return result;
}

/*
    End Helper Functions
*/

/*
    Message Buffer Functions
*/

void decodeStatsString(const char *string, PCStats *stats)
{
    int i = 0;

    //AI REPORT
    if (string[0] == '@')
    {
        int j = 0;
        i = 1;

        while (string[i] != '\0' && string[i] != '\n' && j < 255)
        {
            stats->m_aiReport[j] = string[i];
            j++;
            i++;
        }
        stats->m_aiReport[j] = '\0';
        stats->m_aiActive = 1;

        return;
    }
    //NORMAL STATS
    else if (string[0] == '$')
    {
        i = 1;
    }

    while(string[i] != '\0')
    { 
        //CU = CPU USAGE
        //CT = CPU TEMP
        //RT = RAM TOTAL
        //RU = RAM USAGE
        //RM = RAM USAGE MB
        //DI = DISK I/O MB/s
        //TN = TASK NAME
        //TC = TASK CPU USAGE
        //TR = TASK RAM USAGE
  
        if(string[i] == 'C' && string[i + 1] == 'U') 
        {
            i += 2;
            stats->m_cpu_usage = parseNum(string, &i);
        }
        else if (string[i] == 'C' && string[i + 1] == 'T') 
        {
            i += 2;
            stats->m_cpu_temp_raw = parseNum(string, &i);
        }
        else if(string[i] == 'R' && string[i + 1] == 'U') 
        {
            i += 2;
            stats->m_ram_usage = parseNum(string, &i);
        }
        else if (string[i] == 'R' && string[i + 1] == 'T') 
        {
            i += 2;
            stats->m_ram_total_MB = parseNum(string, &i);
        }
        else if (string[i] == 'R' && string[i + 1] == 'M') 
        {
            i += 2;
            stats->m_ram_used_MB = parseNum(string, &i);
        }
        else if(string[i] == 'D' && string[i + 1] == 'I')
        {
            i += 2;
            stats->m_disk_io_mbps = parseNum(string, &i);
        }
        else if(string[i] == 'A' && string[i + 1] == 'N') 
        {
            i += 2;
            parseString(string, &i, stats->m_taskNames[0], 16);
        }
        else if (string[i] == 'A' && string[i + 1] == 'C') 
        {
            i += 2;
            stats->m_task_cpu[0] = parseNum(string, &i);
        }
        else if (string[i] == 'A' && string[i + 1] == 'R') 
        {
            i += 2;
            stats->m_task_ram[0] = parseNum(string, &i);
        }
        else if(string[i] == 'B' && string[i + 1] == 'N') 
        {
            i += 2;
            parseString(string, &i, stats->m_taskNames[1], 16);
        }
        else if (string[i] == 'B' && string[i + 1] == 'C') 
        {
            i += 2;
            stats->m_task_cpu[1] = parseNum(string, &i);
        }
        else if (string[i] == 'B' && string[i + 1] == 'R') 
        {
            i += 2;
            stats->m_task_ram[1] = parseNum(string, &i);
        }
        else if(string[i] == 'C' && string[i + 1] == 'N') 
        {
            i += 2;
            parseString(string, &i, stats->m_taskNames[2], 16);
        }
        else if (string[i] == 'C' && string[i + 1] == 'C') 
        {
            i += 2;
            stats->m_task_cpu[2] = parseNum(string, &i);
        }
        else if (string[i] == 'C' && string[i + 1] == 'R') 
        {
            i += 2;
            stats->m_task_ram[2] = parseNum(string, &i);
        }
        else 
        {
            i++; //If its anything else skip it
        }
    }
}

int checkUartBuffer(PCStats *stats)
{
    static char localBuffer[256];
    static int index = 0;
    char c;
    int hasData;
    int hasPacketFinished = 0; //Track if a packet finished

    while (1)
    {
        __asm__ volatile("cpsid i" : : : "memory"); //Disable interrupts

        hasData = rbPop(&c);

        __asm__ volatile("cpsie i" : : : "memory"); //Enable Interrupts

        if (!hasData) break;

        if (c == '\n' || c == '\r')
        {
            localBuffer[index] = '\0';
            decodeStatsString(localBuffer, stats);
            index = 0;
            hasPacketFinished = 1; //Packet Complete
        }
        else if (index < 254)
        {
            localBuffer[index] = c;
            index++;
        }
        else
        {
            index = 0;
        }
    }

    return hasPacketFinished;
}

/*
    End Message Buffer Functions
*/

/*
    Hardware Interrupt Functions
*/

void c_uartIRQHandler()
{
    UART0_ICR = 0x7FF;

    UART0_RSRECR = 0;

    while ((UART0_FR & (1 << 4)) == 0)
    {
        char c = (char)(UART0_DR & 0xFF);
        rbPush(c);
    }
}

void c_irqDispatcher()
{
    if (IRQ_PENDING2 & (1 << 25))
    {
        c_uartIRQHandler();
    }
}

void enableUartInterrupts()
{
    UART0_RSRECR = 0; //CLEAR OVERRUN ERRORS
    
    while ((UART0_FR & (1 << 4)) == 0)
    {
        volatile char dummy = (char)(UART0_DR & 0xFF);
    }
    UART0_IMSC |= (1 << 4) | (1 << 6);
    IRQ_ENABLE2 |= (1 << 25);

    __asm__ volatile("cpsie i");
}

/*
    End Hardware Interrupt Functions
*/

/*
    Main
*/

void kernel_main() {
    /* 
        Initialization
    */
    uart_puts("\r\n--- Initializing GPU ---\r\n");

    extern unsigned int _start;

    unsigned int *vectors_dst = (unsigned int *)0x0;
    unsigned int *vectors_src = &_start;

    for (int i = 0; i < 16; i++) 
    {
        vectors_dst[i] = vectors_src[i];
    }

    mbox[0] = 35 * 4;
    mbox[1] = 0; // Request
    
    mbox[2] = 0x48003;  // Set Physical WH
    mbox[3] = 8; 
    mbox[4] = 8;        // Value size
    mbox[5] = 640; 
    mbox[6] = 480;

    mbox[7] = 0x48004;  // Set Virtual WH
    mbox[8] = 8; 
    mbox[9] = 8;        // Value size
    mbox[10] = 640; 
    mbox[11] = 480;

    mbox[12] = 0x48005; // Set Depth
    mbox[13] = 4; 
    mbox[14] = 4;       // Value size
    mbox[15] = 32;

    mbox[16] = 0x40001; // Allocate Buffer
    mbox[17] = 8; 
    mbox[18] = 0;       //
    mbox[19] = 4096;    // Alignment
    mbox[20] = 0;

    mbox[21] = 0;

    // Send to GPU (Channel 8)
    mboxWrite(8, (unsigned int)mbox);
    mboxRead(8);

    if (mbox[1] == MBOX_RESPONSE) 
    {
        
        uart_puts("GPU Responded.\r\n");

        if (mbox[19] == 4096 || mbox[19] == 0) 
        {
            uart_puts("ERROR: GPU did not allocate memory.\r\n");
            while(1); 
        }

        fb = (unsigned int*)(mbox[19] & 0x3FFFFFFF);

        //CLEAR SCREEN
        for (int i = 0; i < 640 * 480; i++) 
        {
            fb[i] = COLOR_BLACK;
        }
    } 
    else 
    {
        uart_puts("GPU didn't to respond.\r\n");
    }

    PCStats globalStats = 
    { 
        .m_cpu_temp_raw = 0, 
        .m_ram_total_MB = 0, 
        .m_ram_used_MB = 0, 
        .m_ram_usage = 0, 
        .m_cpu_usage = 0, 
        .m_disk_io_mbps = 0,
        .m_taskNames = {{'\0'}, {'\0'}, {'\0'}},
        .m_task_cpu = {0, 0, 0},
        .m_task_ram = {0, 0, 0},
        .m_aiActive = 0,
        .m_aiReport = {'\0'}
    };

    PCStats latestStats = globalStats;

    int hasScreenBeenUpdated = 0;
    int isConnected = 0;
    unsigned int lastPacketTime = getSystemTime();
    int showingAIScreen = 0;
    unsigned int aiScreenTimer = 0;

    rbInit();
    enableUartInterrupts();
    
    /* 
        End Initialization
    */

    while (1)
    {

        int gotNewPacket = checkUartBuffer(&latestStats);

        if (gotNewPacket)
        {
            lastPacketTime = getSystemTime();
            if (isConnected == 0)
            {
                isConnected = 1;
                hasScreenBeenUpdated = 0; //Redraw Connected
            }

            if (latestStats.m_aiActive)
            {
                str_cpy(globalStats.m_aiReport, latestStats.m_aiReport);

                showingAIScreen = 1;
                latestStats.m_aiActive = 0; 
                aiScreenTimer = getSystemTime();

                //for (int i = 0; i < 640 * 480; i++)
            }
        }

        //WATCHDOG TIMER 5000 MS - 5 SECONDS
        if (isConnected && (getSystemTime() - lastPacketTime > 5000000))
        {
            isConnected = 0;
            hasScreenBeenUpdated = 0; //Redraw Disconnected
        }

        //AI SCREEN TIMEOUT (10 Seconds = 10,000,000 μs
        if (showingAIScreen && (getSystemTime() - aiScreenTimer > 10000000))
        {
            showingAIScreen = 0;
            drawRect(20, 260, 600, 220, COLOR_BLACK);
            hasScreenBeenUpdated = 0;
        }


        if(globalStats.m_ram_used_MB != latestStats.m_ram_used_MB ||
            globalStats.m_cpu_temp_raw != latestStats.m_cpu_temp_raw || 
            globalStats.m_cpu_usage != latestStats.m_cpu_usage ||
            globalStats.m_disk_io_mbps != latestStats.m_disk_io_mbps ||
            globalStats.m_task_cpu[0] != latestStats.m_task_cpu[0] ||
            globalStats.m_task_ram[0] != latestStats.m_task_ram[0] ||
            str_cmp(globalStats.m_taskNames[0], latestStats.m_taskNames[0]) != 0 ||
            globalStats.m_task_cpu[1] != latestStats.m_task_cpu[1] ||
            globalStats.m_task_ram[1] != latestStats.m_task_ram[1] ||
            str_cmp(globalStats.m_taskNames[1], latestStats.m_taskNames[1]) != 0 ||
            globalStats.m_task_cpu[2] != latestStats.m_task_cpu[2] ||
            globalStats.m_task_ram[2] != latestStats.m_task_ram[2] ||
            str_cmp(globalStats.m_taskNames[2], latestStats.m_taskNames[2]) != 0)
        {
            globalStats.m_cpu_usage = latestStats.m_cpu_usage;
            globalStats.m_cpu_temp_raw = latestStats.m_cpu_temp_raw;

            globalStats.m_ram_usage = latestStats.m_ram_usage;
            globalStats.m_ram_used_MB = latestStats.m_ram_used_MB;
            globalStats.m_ram_total_MB = latestStats.m_ram_total_MB;

            globalStats.m_disk_io_mbps = latestStats.m_disk_io_mbps;

            for (int i = 0; i < 3; i++)
            {
                globalStats.m_task_cpu[i] = latestStats.m_task_cpu[i];
                globalStats.m_task_ram[i] = latestStats.m_task_ram[i];
                str_cpy(globalStats.m_taskNames[i], latestStats.m_taskNames[i]);
            }

            hasScreenBeenUpdated = 0;
        }

        if(!hasScreenBeenUpdated)
        {
            if (showingAIScreen)
            {
                drawAIDisplay(&globalStats);
            }

            drawDisplay(&globalStats, isConnected);
            hasScreenBeenUpdated = 1;
        }
    }
}

/*
    End Main
*/