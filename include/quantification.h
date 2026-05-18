#ifndef _QUANTIFICATION_H_
#define _QUANTIFICATION_H_

#include <stdint.h>
#include <math.h>

#include "zz.h"
#include "qtables.h"

/* structures */

/* prototypes */
void quantification_bloc(int16_t *input, int16_t *output, bloc_type bloc);
void quantification_application(bloc *zz_flow, bloc *q_flow, int nbr_blocs);

#endif /* _QUANTIFICATION_H_ */