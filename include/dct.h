#ifndef _DCT_H_
#define _DCT_H_

#include <math.h>

#include "mcu_compression.h"

// Un prototype pour une DCT qui travaille en place
// A jeter selon vos besoins/choix
// void dct_naive(int16_t *block) {
// 	(void)block;
// }

#define PI 3.14159265358979323846
#define C(k) ((k) == 0 ? 1.0 / sqrt(2.0) : 1.0) 


void cos_init(double *cos_table);
void dct_bloc_naive(int16_t *input, int16_t *output,double *cos_table);
void dct_1d(double *input, double *output, double *cos_table);
void dct_bloc_1d(int16_t *input, int16_t *output, double *cos_table);
void dct_application(bloc *mcu_flow, bloc *dct_flow, int nbr_blocs, double *cos_table);

#endif /* _DCT_H_ */
