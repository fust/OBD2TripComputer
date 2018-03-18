#include <Arduino.h>
#include <TFT_HX8357.h>

#ifndef __GRAPH_H
#define __GRAPH_H

class Graph {
  public:
    void init(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, TFT_HX8357 tft);
    void clear();
    void plot(uint16_t value);
  private:
    uint16_t x, y, maxX, maxY, curX;
    uint16_t *values;
    TFT_HX8357 display;
    void scroll();
};


#endif
