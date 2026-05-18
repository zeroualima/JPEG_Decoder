#include "codage_rle.h"

// entree : vect 1D de 64 coeff
// on initialise le prédicateur à 0
// Classes de magnitude des coefficients DC (pour AC, la classe max est 10 et la magnitude 0 n’est jamais utilisée).

uint8_t buffer = 0;
uint8_t bit_count = 0;

void write_bits(FILE *f, int val, int nbr_bits) {
    // si nbr_bits > 8, on traite par tranches de 8 bits
    if (nbr_bits > 8) {
        write_bits(f, val >> 8, nbr_bits - 8);      // bits de poids fort d'abord
        write_bits(f, val & 0xFF, 8);                // puis les 8 bits de poids faible
        return;
    }
    if (bit_count + nbr_bits > 8 && nbr_bits <= 8) {
        buffer = (buffer << (8 - bit_count)) | (val >> (nbr_bits + bit_count - 8));
        fwrite(&buffer, 1, 1, f);
        if (buffer == 0xFF)
            fwrite("\x00", 1, 1, f);
        int overflow = nbr_bits + bit_count - 8;
        buffer = val & ((1 << overflow) - 1);
        bit_count = overflow;
    } else if (bit_count + nbr_bits <= 8) {
        buffer = (buffer << nbr_bits) | val;
        bit_count = bit_count + nbr_bits;
        if (bit_count == 8) {
            fwrite(&buffer, 1, 1, f);
            if (buffer == 0xFF)
                fwrite("\x00", 1, 1, f);  // byte stuffing !
            buffer = 0;
            bit_count = 0;
        }
    }
    //  else {
    //     fprintf(stderr, "Operation Impossible : nbr_bits > 8");
    // }
}

void flush_bits(FILE *f) {
    if (bit_count > 0) {
        buffer <<= (8 - bit_count);
        buffer |= (1 << (8 - bit_count)) - 1; // padding avec des 1
        fwrite(&buffer, 1, 1, f);
        if (buffer == 0xFF)
            fwrite("\x00", 1, 1, f);
    }

    buffer = 0;
    bit_count = 0;
}

Coeff_Huff magnitude(int val) {
    Coeff_Huff new;
    // if (val < -2047 || val > 2047) {
    //     fprintf(stderr, "val < -2047 ou val > 2047\n");
    //     exit(1);
    // }
    uint8_t classe = 0;
    if (val != 0) {
        for (uint8_t m = 1; m <= 11; m++) {
            int b = pow(2, m-1);
            if ((-(2*b - 1) <= val && val <= -b) || (b <= val && val <= (2*b - 1))) {
                classe = m;
                break;
            }
        }
    }
    new.classe = classe;
    new.indice = (val >= 0) ? val : val + (1 << classe) - 1;
    return new;
}

Chemin_Huff codage_Huff(int classe, const uint8_t counts[], const uint8_t symbols[]) {
    Chemin_Huff result;
    int code = 0;
    int indice = 0;

    for (int profondeur = 1; profondeur <= 16; profondeur++) {
        for (int j = 0; j < counts[profondeur - 1]; j++) {
            if (symbols[indice] == (uint8_t)classe) {
                result.chemin = code;
                result.profondeur = profondeur;
                return result;
            }
            code++;
            indice++;
        }
        code <<= 1;  // passer au niveau suivant
    }
    
    fprintf(stderr, "SOMETHING BAD HAPPENED! classe=%d introuvable\n", classe);
    // result.chemin = 0;
    // result.profondeur = 0;
    return result;
}

void chaine_Huff_coeff(FILE *f, int16_t coeff, int cpt_zeros, bool is_DC, bool is_Y, bool is_Cb, int predicateur) {
    if (is_DC) {
        Coeff_Huff coeff_info = magnitude(coeff - predicateur);
        Chemin_Huff code;
        if(is_Y) {
            code = codage_Huff(coeff_info.classe, htables_nb_symb_per_lengths[0][0], htables_symbols[0][0]);
        } else if (is_Cb) {
            code = codage_Huff(coeff_info.classe, htables_nb_symb_per_lengths[0][1], htables_symbols[0][1]);
        } else {
            code = codage_Huff(coeff_info.classe, htables_nb_symb_per_lengths[0][2], htables_symbols[0][2]);
        }
        write_bits(f, code.chemin, code.profondeur);
        write_bits(f, coeff_info.indice, coeff_info.classe);
    } else {
        int classe = (cpt_zeros << 4) | magnitude(coeff).classe;
        Chemin_Huff code;
        if(is_Y) {
            code = codage_Huff(classe, htables_nb_symb_per_lengths[1][0], htables_symbols[1][0]);
        } else if (is_Cb) {
            code = codage_Huff(classe, htables_nb_symb_per_lengths[1][1], htables_symbols[1][1]);
        } else {
            code = codage_Huff(classe, htables_nb_symb_per_lengths[1][2], htables_symbols[1][2]);
        }
        write_bits(f, code.chemin, code.profondeur);
        Coeff_Huff coeff_ac = magnitude(coeff);
        write_bits(f, coeff_ac.indice, coeff_ac.classe);
    }
}

void chaine_Huff_vect(FILE *f, int16_t *coeffs, bool is_Y, bool is_Cb, int predicateur) {
    // Mode Baseline
    chaine_Huff_coeff(f, coeffs[0], -1, true, is_Y, is_Cb, predicateur);
    int cpt_zeros = 0;
    for (int i = 1; i < 64; i++) { // Detect when all comings are zeros
        if (coeffs[i] == 0) {
            cpt_zeros++;
            if (i == 63) { // Il faut ecrire EOB
                Chemin_Huff eob;
                if (is_Y) {
                    eob = codage_Huff(0x00, htables_nb_symb_per_lengths[1][0], htables_symbols[1][0]);
                } else if (is_Cb) {
                    eob = codage_Huff(0x00, htables_nb_symb_per_lengths[1][1], htables_symbols[1][1]);
                } else {
                    eob = codage_Huff(0x00, htables_nb_symb_per_lengths[1][2], htables_symbols[1][2]);
                }
                write_bits(f, eob.chemin, eob.profondeur);
            }
        } else {
            // ZRL : émettre 0xF0 pour chaque groupe de 16 zéros
            while (cpt_zeros >= 16) {
                Chemin_Huff zrl;
                if (is_Y)
                    zrl = codage_Huff(0xF0, htables_nb_symb_per_lengths[1][0], htables_symbols[1][0]);
                else if (is_Cb)
                    zrl = codage_Huff(0xF0, htables_nb_symb_per_lengths[1][1], htables_symbols[1][1]);
                else
                    zrl = codage_Huff(0xF0, htables_nb_symb_per_lengths[1][2], htables_symbols[1][2]);
                write_bits(f, zrl.chemin, zrl.profondeur);
                cpt_zeros -= 16;
            }
            chaine_Huff_coeff(f, coeffs[i], cpt_zeros, false, is_Y, is_Cb, -1);
            cpt_zeros = 0;
        }
    }    
}

