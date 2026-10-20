#ifndef RENDER_H
#define RENDER_H

extern unsigned int *fb;

void drawPixel(int x, int y, unsigned int color);

void drawRect(int x, int y, int w, int h, unsigned int color);

void drawChar(int x, int y, char c, unsigned int color, unsigned int bg_color);

//draw full string of text
void drawString(int x, int y, const char* str, unsigned int color, unsigned int bg_textColor);

void drawProgressBar(int x, int y, int width, int height, int percent, unsigned int fg_color, unsigned int bg_color);

void intToString(int num, char* str);

int drawNumber (int x, int y, int num, unsigned int color, unsigned int bg_color);

#endif