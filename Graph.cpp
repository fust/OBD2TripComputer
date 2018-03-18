#include "Graph.h"

void Graph::init(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, TFT_HX8357 tft)
{
  x = x0;
  y = y0;
  maxX = x1;
  maxY = y1;
  display = tft;

  curX = 1;

  values = (uint16_t *)malloc((maxX - x) * sizeof(uint16_t));

  clear();
}

void Graph::clear()
{
  display.fillRect(x, y, maxX, maxY, TFT_BLACK);
  display.drawFastVLine(x, y, (maxY - y) - 1, TFT_WHITE);
  display.drawFastHLine(x, maxY - 1, (maxX - x) - 1, TFT_WHITE);
}

void Graph::plot(uint16_t value)
{
  uint16_t val = (value * 100) / (maxY - y);

  values[curX] = val;

  display.drawPixel(x + curX, maxY - val, TFT_WHITE);
  
  if (curX >= maxX) {
    scroll();
  } else {
    curX++;
  }
}

void Graph::scroll()
{
  for (uint16_t i = 0; i < (maxX - x); i++) {
    if ((maxY - values[i]) < (maxY - 1) && i > 0) {
      display.drawPixel(x + i, maxY - values[i], TFT_BLACK);
    }
    values[i] = values[i + 1];
    display.drawPixel(x + i, maxY - values[i], TFT_WHITE);
  }
  values[maxX - x] = 0;
}

