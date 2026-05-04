#ifndef _TRAITEMENT_MCU_H_
#define _TRAITEMENT_MCU_H_

#include "parser_resize.h"
#include "rgb_ycbcr.h"
#include "mcu_compression.h"
#include "dct.h"
#include "zz.h"
#include "quantification.h"

/* structures */

/* prototypes */
void traitement_mcu(rgb_mcu *vect_img, bloc *mcu_flow, sampling_factors s, int nbr_bloc_mcu, double *cos_table, int colors, ycbcr_mcu *tmp_1, bloc *tmp_2);

#endif /* _TRAITEMENT_MCU_H_ */