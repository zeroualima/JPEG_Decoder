#include "make_JPEG.h"

// SOI
uint8_t soi[] = {0xFF, 0xD8};

// EOI
uint8_t eoi[] = {0xFF, 0xD9};

// APP0
uint8_t app0[] = {
    0xFF, 0xE0, // marqueur APP0
    0x00, 0x10, // longueur = 16
    'J','F','I','F','\0', // identifiant
    0x01, 0x01, // version 1.1
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 // Données spécifiques au JFIF, non traitées, 7 octets à 0
};

// DQT
void write_DQT(FILE *f, uint8_t iQ, const uint8_t table_zigzag[64]) {
    uint8_t entete[] = {
        0xFF, 0xDB, // marqueur DQT
        0x00, 0x43, // longueur = 67 = 2(2 octets de longueur eux-mêmes) + 1(le byte précision/iQ) + 64(les valeurs de la table)
        (0x00 << 4) | iQ  // précision 8 bits = (0x00) + indice iQ, iQ = 0 ou 1 (pour deux tables de quantification, la luminance Y, et la chrominance Cb et Cr)
    };
    fwrite(entete, 1, sizeof(entete), f);

    // Les 64 valeurs en ordre zig-zag
    fwrite(table_zigzag, 1, 64, f);
}

// SOF
void write_SOF(FILE *f, uint8_t marqueur, uint16_t hauteur, uint16_t largeur, uint8_t nb_couleurs, uint8_t fH_Y, uint8_t fV_Y, uint8_t fH_Cb, uint8_t fV_Cb,  uint8_t fH_Cr, uint8_t fV_Cr) {
    uint16_t longueur = 8 + 3 * nb_couleurs;
    uint8_t sof[] = {
        0xFF, marqueur,
        (longueur >> 8) & 0xFF, longueur & 0xFF,
        0x08,
        (hauteur >> 8) & 0xFF, hauteur & 0xFF,
        (largeur >> 8) & 0xFF, largeur & 0xFF,
        nb_couleurs,
        0x01, (fH_Y << 4) | fV_Y, 0x00,
    };
    fwrite(sof, 1, sizeof(sof), f);

    if (nb_couleurs == 3) {
        uint8_t sof_[] = {
            0x02, (fH_Cb << 4) | fV_Cb, 0x01, // Cb : iC=2, sampling 1×1, iQ=1
            0x03, (fH_Cr << 4) | fV_Cr, 0x01, // Cr : iC=3, sampling 1×1, iQ=1
        };
        fwrite(sof_, 1, sizeof(sof_), f);
    }
}


// DHT
void write_DHT(FILE *f, uint8_t is_AC, uint8_t iH, const uint8_t nb_symb_per_lengths[16], const uint8_t *symboles) {
    uint8_t N = 0; // Nombre de symbole
    for (int i = 0; i < 16; i++) {
        N += nb_symb_per_lengths[i];
    }

    uint16_t longueur = 2 + 1 + 16 + N; // longueur = 2 (longueur) + 1 (Type + indice) + 16 (profondeurs) + N (symboles)

    uint8_t entete[] = {
        0xFF, 0xC4, // marqueur DHT
        (longueur >> 8) & 0xFF, longueur & 0xFF,
        (is_AC << 4) | iH, // type (0=DC, 1=AC) + indice iH = 0...3
    };
    fwrite(entete, 1, sizeof(entete), f);

    fwrite(nb_symb_per_lengths, 1, 16, f); // Les 16 nombres de symboles par profondeur dans la table de Huffmann

    fwrite(symboles, 1, N, f); // les N symboles
}

// SOS
void write_SOS(FILE *f, uint8_t nb_couleurs, uint8_t iH_DC_Y, uint8_t iH_AC_Y, uint8_t iH_DC_C, uint8_t iH_AC_C, uint8_t SS, uint8_t SE, uint8_t AhAl) {

    uint16_t longueur = 2 + 1 + 2 * nb_couleurs + 3;

    uint8_t entete[] = {
        0xFF, 0xDA,
        (longueur >> 8) & 0xFF, longueur & 0xFF,
        nb_couleurs,
    };
    fwrite(entete, 1, sizeof(entete), f);

    if (nb_couleurs == 1) {
        // une seule composante Y
        uint8_t composantes[] = {0x01, (iH_DC_Y << 4) | iH_AC_Y};
        fwrite(composantes, 1, sizeof(composantes), f);
    } else {
        // trois composantes Y, Cb, Cr
        uint8_t composantes[] = {
            0x01, (iH_DC_Y << 4) | iH_AC_Y,
            0x02, (iH_DC_C << 4) | iH_AC_C,
            0x03, (iH_DC_C << 4) | iH_AC_C,
        };
        fwrite(composantes, 1, sizeof(composantes), f);
    }

    // toujours ces 3 octets car mode baseline
    uint8_t fin[] = {SS, SE, AhAl};
    fwrite(fin, 1, sizeof(fin), f);
}

/* =================================================== Mode Baseline ============================================== */

void add_JPEG_entete(FILE *f, uint16_t hauteur, uint16_t largeur, uint8_t nb_couleurs, sampling_factors s) {
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

    // SOF0
    write_SOF(f, 0xC0, hauteur, largeur, nb_couleurs, fH_Y, fV_Y, fH_Cb, fV_Cb, fH_Cr, fV_Cr);

    // DHT
    write_DHT(f, 0, 0, htables_nb_symb_per_lengths[0][0], htables_symbols[0][0]); // DC_Y, is_AC = 0, iH = 0        
    write_DHT(f, 1, 0, htables_nb_symb_per_lengths[1][0], htables_symbols[1][0]); // AC_Y, is_AC = 1, iH = 0
    if (nb_couleurs == 3) {
        write_DHT(f, 0, 1, htables_nb_symb_per_lengths[0][1], htables_symbols[0][1]); // DC_CbCr, is_AC = 0, iH = 1
        write_DHT(f, 1, 1, htables_nb_symb_per_lengths[1][1], htables_symbols[1][1]); // AC_CbCr, is_AC = 1, iH = 1
    }

    // SOS
    if (nb_couleurs == 1) {
        write_SOS(f, 1, 0, 0, 0, 0, 0x00, 0x3F, 0x00);
    } else {
        write_SOS(f, 3, 0, 0, 1, 1, 0x00, 0x3F, 0x00);
    }
}

int predY = 0;
int predCb = 0;
int predCr = 0;

void add_JPEG_total_bitstream(FILE *f, int nb_blocs, bloc *blocs) {
    // BitStream baseline
    /* ===================== Huffmann ===================== */
    for (int i = 0; i < nb_blocs; i++) {
        if (blocs[i].type == Y) {
            chaine_Huff_vect(f, blocs[i].data, true, false, predY);
            predY = (blocs[i].data)[0];
        } else if (blocs[i].type == Cb) {
            chaine_Huff_vect(f, blocs[i].data, false, true, predCb);
            predCb = (blocs[i].data)[0];
        } else {
            chaine_Huff_vect(f, blocs[i].data, false, false, predCr);
            predCr = (blocs[i].data)[0];
        }
    }
}

void add_JPEG_end(FILE *f) {
    flush_bits(f);
    // EOI
    fwrite(eoi, 1, 2, f);
}

// void add_JPEG_bitstream(FILE *f, bloc bloc) {
//     // BitStream
//     /* ===================== Huffmann ===================== */
//     if (bloc.type == Y) {
//         chaine_Huff_vect(f, bloc.data, true, false, predY);
//         predY = (bloc.data)[0];
//     } else if (bloc.type == Cb) {
//         chaine_Huff_vect(f, bloc.data, false, true, predCb);
//         predCb = (bloc.data)[0];
//     } else {
//         chaine_Huff_vect(f, bloc.data, false, false, predCr);
//         predCr = (bloc.data)[0];
//     }
// }

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