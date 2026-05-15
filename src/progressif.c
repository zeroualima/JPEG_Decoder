#include "progressif.h"

/* codage_rle.c */

/* version adaptée pour le mode progressif */
void chaine_Huff_vect_progressif(FILE *f, int16_t *coeffs, bool is_Y, bool is_Cb, int predicateur, int debut, int fin) {
    if (debut == 0 && fin == 0) {
        // Scan 1, Mode Progressif
        chaine_Huff_coeff(f, coeffs[0], -1, true, is_Y, is_Cb, predicateur);
    } else if (fin != 0) {
        // Autres Scans (#?)
        int cpt_zeros = 0;
        for (int i = debut; i < fin + 1; i++) { // Detect when all comings are zeros
            if (coeffs[i] == 0) {
                cpt_zeros++;
                /* !!!! attention "i" s'arret à "fin" et non pas à 63 */
                if (i == fin) { // Il faut ecrire EOB
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
    } else {
        fprintf(stderr, "UNSUPPORTED COMBINATION OF PARAMETERS");
    }
}

/* make_JPEG.c */

/* =================================================== Mode Progressif ============================================== */

/* 
    ==========> Schema general du scan pour le mode progressif : 

    Scan_1 :    on scanne les DC de toutes les composantes          (debut = 0 et fin = 0)
                    -> Y            si .pgm (nb_colors == 1)
                    -> Y, Cr et Cb  si .ppm (nb_colors == 3)
                
                etape_1 : ecriture table de Huffman de DC de Y                          (toujours)
                etape_2 : ecriture table de Huffman de DC de Cb/Cr                      (si nb_colors == 3) 
                etape_3 :  | # si .pgm (nb_colors == 1) : ecriture SOS de Y             (nb_comp = 1)
                           | # si .ppm (nb_colors == 3) : ecriture SOS de Y, Cb et Cr   (nb_comp = 3)

    Scan_2 :    on scanne les 9 premiers AC de Y                    (debut = 1 et fin = 9)
                etape_1 : ecriture table de Huffman de AC de Y
                etape_2 : ecriture SOS de Y                                             (nb_comp = 1)

    Scan_3 :    on scanne les AC de Cr                              (debut = 1 et fin = 63)
                etape_1 : ecriture table de Huffman de AC de Cr 
                etape_2 : ecriture SOS de Cr                                            (nb_comp = 1)

    Scan_4 :    on scanne les AC de Cb                              (debut = 1 et fin = 63)
                etape_1 : ecriture table de Huffman de AC de Cb 
                etape_2 : ecriture SOS de Cb                                            (nb_comp = 1)

    Scan_5 :    on scanne les 54 AC restent de Y                    (debut = 10 et fin = 63)
                etape_1 : ecriture table de Huffman de AC de Y
                etape_2 : ecriture SOS de Y                                             (nb_comp = 1)
*/

/* ecriture de l'entete : SOI - APP0 - DQT - SOF2 */
void add_JPEG_entete_progressif(FILE *f, uint16_t hauteur, uint16_t largeur, uint8_t nb_couleurs, sampling_factors s) {
    uint8_t fH_Y = s.h[0];
    uint8_t fV_Y = s.v[0];
    uint8_t fH_Cb = s.h[1];
    uint8_t fV_Cb = s.v[1];
    uint8_t fH_Cr = s.h[2];
    uint8_t fV_Cr = s.v[2];

    // SOI
    fwrite(soi, 1, 2, f);

    // APP0
    fwrite(app0, 1, sizeof(app0), f);

    // DQT
    write_DQT(f, 0, quantification_table_Y);
    write_DQT(f, 1, quantification_table_CbCr);

    // SOF2
    write_SOF(f, 0xC2, hauteur, largeur, nb_couleurs, fH_Y, fV_Y, fH_Cb, fV_Cb, fH_Cr, fV_Cr);
}

/* ecriture de SOS en fct de "nb_comp" et "scan" et "nb_couleurs" */
void write_SOS_progressif(FILE *f, uint8_t nb_comp, uint8_t scan, uint8_t nb_couleurs, uint8_t SS, uint8_t SE, uint8_t AhAl) {

    uint16_t longueur = 2 + 1 + 2 * nb_comp + 3;

    uint8_t entete[] = {
        0xFF, 0xDA,
        (longueur >> 8) & 0xFF, longueur & 0xFF,
        nb_comp,
    };
    fwrite(entete, 1, sizeof(entete), f);

    if (nb_couleurs == 1 && (scan == 0 || scan == 4)) {
        /* cas de .pgm */
        uint8_t composantes[] = {0x01, (0 << 4) | 0 };          /* DC de Y */
        fwrite(composantes, 1, sizeof(composantes), f);
    } else {
        /* cas de .ppm */
        if (scan == 0) {
            /* DC de Y, Cb et Cr */
            uint8_t composantes[] = {
                0x01, (0 << 4) | 0,
                0x02, (1 << 4) | 0,
                0x03, (1 << 4) | 0 };
            fwrite(composantes, 1, sizeof(composantes), f);
        } else if (scan == 1 || scan == 4) {
            uint8_t composantes[] = {0x01, (0 << 4) | 0 };      /* AC de Y */
            fwrite(composantes, 1, sizeof(composantes), f); 
        } else if (scan == 2) {
            uint8_t composantes[] = {0x03, (1 << 4) | 1 };      /* AC de Cr */
            fwrite(composantes, 1, sizeof(composantes), f);
        } else if (scan == 3) {
            uint8_t composantes[] = {0x02, (1 << 4) | 1 };      /* AC de Cb */
            fwrite(composantes, 1, sizeof(composantes), f);
        }
    }

    // toujours ces 3 octets
    uint8_t fin[] = {SS, SE, AhAl};
    fwrite(fin, 1, sizeof(fin), f);
}

/* ecriture de DHT et application d'ecriture de SOS dans les different cas */
void write_DHT_SOS_progressif(FILE *f, int scan, int nb_colors, int debut, int fin) {
    if (scan == 0) {
        write_DHT(f, 0, 0, htables_nb_symb_per_lengths[0][0], htables_symbols[0][0]);       // DC_Y   , is_AC = 0, iH = 0
        if (nb_colors == 3) {
            write_DHT(f, 0, 1, htables_nb_symb_per_lengths[0][1], htables_symbols[0][1]);   // DC_CbCr, is_AC = 0, iH = 1
            write_SOS_progressif(f, 3, scan, nb_colors, debut, fin, 0);
        } else {
            write_SOS_progressif(f, 1, scan, nb_colors, debut, fin, 0);
        }
    } else if (scan == 1 || scan == 4) {
        write_DHT(f, 1, 0, htables_nb_symb_per_lengths[1][0], htables_symbols[1][0]);       // AC_Y   , is_AC = 1, iH = 0
        write_SOS_progressif(f, 1, scan, nb_colors, debut, fin, 0);
    } else if (nb_colors == 3 && (scan == 2 || scan == 3)) {
        write_DHT(f, 1, 1, htables_nb_symb_per_lengths[1][1], htables_symbols[1][1]);       // AC_CbCr, is_AC = 1, iH = 1            
        write_SOS_progressif(f, 1, scan, nb_colors, debut, fin, 0);
    }
}

/* ecriture de la zone de valeur utile de "blocs" en fct de "scan" */
void add_JPEG_total_bitstream_progressif(FILE *f, int nb_blocs, bloc *blocs, int debut, int fin, int scan) {
    // BitStream Progressif (scan #?)

    /* on fait "pred? = (blocs[i].data)[0];" uniqument pour traiter les DC */

    /* ===================== Huffmann ===================== */
    if (scan == 0) {
        for (int i = 0; i < nb_blocs; i++) {
            if (blocs[i].type == Y) {
                chaine_Huff_vect_progressif(f, blocs[i].data, true, false, predY, debut, fin);
                predY = (blocs[i].data)[0];
            } else if (blocs[i].type == Cb) {
                chaine_Huff_vect_progressif(f, blocs[i].data, false, true, predCb, debut, fin);
                predCb = (blocs[i].data)[0];
            } else {
                chaine_Huff_vect_progressif(f, blocs[i].data, false, false, predCr, debut, fin);
                predCr = (blocs[i].data)[0];
            }
        }
    } else if (scan == 1) {
        for (int i = 0; i < nb_blocs; i++) {
            if (blocs[i].type == Y) {
                chaine_Huff_vect_progressif(f, blocs[i].data, true, false, predY, debut, fin);
            }
        }
    } else if (scan == 2) {
        for (int i = 0; i < nb_blocs; i++) {
            if (blocs[i].type == Cr) {
                chaine_Huff_vect_progressif(f, blocs[i].data, false, false, predCr, debut, fin);
            }
        }
    } else if (scan == 3) {
        for (int i = 0; i < nb_blocs; i++) {
            if (blocs[i].type == Cb) {
                chaine_Huff_vect_progressif(f, blocs[i].data, false, true, predCb, debut, fin);
            }
        }
    } else if (scan == 4) {
        for (int i = 0; i < nb_blocs; i++) {
            if (blocs[i].type == Y) {
                chaine_Huff_vect_progressif(f, blocs[i].data, true, false, predY, debut, fin);
            }
        }
    }
}

/* flush et ecriture de EOI */
void add_JPEG_end_progressif(FILE *f, int scan, int scan_max) {
    if (scan == scan_max - 1) {
        add_JPEG_end(f);
    } else {
        flush_bits(f);
    }
}