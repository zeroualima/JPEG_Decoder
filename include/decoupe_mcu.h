#ifndef DECOUPE_MCU
#define DECOUPE_MCU

#include "parser.h"

Image *resize(Image *old, int mcu_h, int mcu_v, int block_h, int block_v);

#endif