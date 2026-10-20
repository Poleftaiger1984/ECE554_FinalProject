#include "render.h"
#include "font.h"

unsigned int *fb;

void drawPixel(int x, int y, unsigned int color) 
{
    if (x < 0 || x >= 640 || y < 0 || y >= 480) return;
    fb[y * 640 + x] = color;
}

void drawRect(int x, int y, int w, int h, unsigned int color) 
{
    for (int current_y = y; current_y < y + h; current_y++) {
        for (int current_x = x; current_x < x + w; current_x++) {
            drawPixel(current_x, current_y, color);
        }
    }
}

void drawChar(int x, int y, char c, unsigned int color, unsigned int bg_color) 
{
    if (c < 32 || c > 126) return; 
    
    int font_index = c - 32;

    for (int row = 0; row < 8; row++) {
        unsigned char row_data = font8x8[font_index][row];
        for (int col = 0; col < 8; col++) {
            if ((row_data >> (7 - col)) & 1) {
                drawPixel(x + col, y + row, color);
            }
            else
            {
                drawPixel(x + col, y + row, bg_color);
            }
        }
    }
}

void drawString(int x, int y, const char* str, unsigned int color, unsigned int bg_textColor) 
{
    int current_x = x;
    for (int i = 0; str[i] != '\0'; i++) {
        drawChar(current_x, y, str[i], color, bg_textColor);
        current_x += 8; // Move 8 pixels to the right for next letter
    }
}

void drawProgressBar(int x, int y, int width, int height, int percent, unsigned int fg_color, unsigned int bg_color) 
{
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;

    int fill_width = (width * percent) / 100;
    drawRect(x, y, width, height, bg_color);
    if (fill_width > 0) {
        drawRect(x, y, fill_width, height, fg_color);
    }
}

void intToString(int num, char* str)
{
    int i = 0;
    int is_negative = 0;

    if(num == 0)
    {
        str[i++] = '0';
        str[i] = '\0';
        return;
    }

    if(num < 0)
    {
        is_negative = 1;
        num = -num;
    }

    while (num != 0)
    {
        int rem = num % 10;
        str[i++] = rem + '0';
        num = num / 10;
    }

    if(is_negative) str[i++] = '-';

    str[i] = '\0';

    //Reverse string
    int start = 0;
    int end = i - 1;
    while (start < end)
    {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }
}

int drawNumber(int x, int y, int num, unsigned int color, unsigned int bg_color)
{
    char num_str[16];
    intToString(num, num_str);
    drawString(x, y, num_str, color, bg_color);

    int length = 0;
    while (num_str[length] != '\0') length++;

    return x + (length * 8);
}