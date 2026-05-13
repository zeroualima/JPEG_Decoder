# Encodeur JPEG | JPEG ENCODER

![Coverage](https://gitlab.ensimag.fr/formationc/projet/jpeg/2026/13_boubekrs_boulaiay_zerouama/badges/main/coverage.svg)

[Tableau de bord](https://formationc.pages.ensimag.fr/projet/jpeg/2026/13_boubekrs_boulaiay_zerouama)

Ce projet implémente un encodeur JPEG from scratch en C. Il prend en entrée des images PPM (couleur) ou PGM (niveaux de gris) et produit des fichiers JPEG valides conformes au standard JFIF.

---

## Pipeline de compression

Le traitement se fait MCU par MCU (Minimum Coded Unit). Voici les étapes dans l'ordre :

```
<Fichier>.ppm/pgm : gestion dans  main.c

          │
          ▼
┌─────────────────────┐
│  Parsing & padding  │  parser_resize.c
│  Lecture de l'image │  Lecture ligne par ligne, padding à des multiples de 8*v[0] et 8*h[0]
└─────────┬───────────┘
          │  rgb_mcu  (pixels bruts par MCU)
          ▼
┌─────────────────────┐
│  RGB → YCbCr        │  rgb_ycbcr.c    
└─────────┬───────────┘
          │  ycbcr_mcu  (composantes Y, Cb, Cr séparées)
          ▼
┌─────────────────────┐
│  Découpage en blocs │  mcu_compression.c
│  + sous-échant.     │  Blocs 8*8, sous-echantillonnage Cb/Cr selon les facteurs H*V 
└─────────┬───────────┘
          │  bloc[]  (tableau de blocs 8*8 de type Y, Cb ou Cr)
          ▼
┌─────────────────────┐
│  DCT 2D             │  dct.c
│  (séparable 1D)     │  Transformee en cosinus discrete sur chaque bloc 8*8
└─────────┬───────────┘
          │
          ▼
┌─────────────────────┐
│  Scan zig-zag       │  zz.c
│                     │  Reordonnancement zigzag
└─────────┬───────────┘
          │
          ▼
┌─────────────────────┐
│  Quantification     │  quantification.c
│                     │  Tables Q distinctes pour Y et Cb/Cr
└─────────┬───────────┘
          │
          ▼
┌─────────────────────┐
│  Codage RLE/Huffman │  codage_rle.c + htables.c
│                     │  Codage DC différentiel, AC run-length + tables Huffman JPEG standard
└─────────┬───────────┘
          │
          ▼
┌─────────────────────┐
│  Écriture JPEG      │  make_JPEG.c
│                     │  ecriture des  marqueurs (SOI, APP0, DQT, SOF0, DHT, SOS, Donnees brutes, EOI)
└─────────────────────┘
          │
          ▼
    <Fichier>.jpg    : ohhh finalement!!

```

---

## Structures de données

Les structures principales qui circulent dans le pipeline :

```c
/* Facteurs de sous-échantillonnage pour les 3 composantes (Y, Cb, Cr) */
typedef struct {
    uint8_t h[3];   // facteurs horizontaux
    uint8_t v[3];   // facteurs verticaux
} sampling_factors;

/* Représente l'image source avec ses métadonnées et un buffer glissant pour la lecture */
typedef struct {
    int w, h;               // dimensions originales
    int pw, ph;             // dimensions après padding (multiples de 8*h[0] et 8*v[0])
    int colors;             // 1 = PGM, 3 = PPM
    uint8_t *row_buffer;    // fenêtre glissante de mcu_height lignes de pixels
    long header_offset;     // offset du premier octet de pixel dans le fichier
    FILE *f;
    // ...
} Image;

/* MCU brute en RGB — un tableau de pixels, chaque pixel = [R,G,B] ou [Y] */
typedef struct { RGB *data; } rgb_mcu;

/* MCU après conversion, composantes séparées */
typedef struct {
    uint8_t *Y, *Cb, *Cr;
} ycbcr_mcu;

/* Un bloc 8*8 de coefficients, avec son type pour savoir quelle table Huffman utiliser */
typedef struct {
    int16_t data[64];
    bloc_type type;   // Y, Cb ou Cr
} bloc;
```

---

## Compilation

### Prérequis

- GCC (C99)
- `make`
- Python 3 + pip (tests d'intégration)
- `gcovr` (couverture)
- `valgrind` + `kcachegrind` (profilage, optionnel)

### Cibles make

| Commande          | Ce que ça fait                                                   |
|-------------------|------------------------------------------------------------------|
| `make all`        | Compile l'encodeur + les tests (ASan + couverture activés)       |
| `make debug`      | Recompile proprement en mode debug (`-Og`, ASan)                 |
| `make perf`       | Recompile en `-O3` sans instrumentation pour mesurer les perfs   |
| `make tests`      | Lance toute la suite de tests (Unity + pytest)                   |
| `make couverture` | Tests + rapport de couverture HTML                               |
| `make profilage`  | Callgrind sur l'image de test + ouverture dans kcachegrind       |
| `make clean`      | Supprime les binaires                                            |
| `make realclean`  | Nettoyage complet avant commit                                   |

```bash
make all    # compilation standard
make debug  # si vous voulez déboguer avec gdb
```

---

## Utilisation

```
./ppm2jpeg [--outfile=<fichier.jpg>] [--sample=H1xV1,H2xV2,H3xV3] <input.ppm|input.pgm>
```

| Option                    | Description                                                        |
|---------------------------|--------------------------------------------------------------------|
| `--outfile=<fichier.jpg>` | Fichier de sortie (par défaut : `out/<nom>.jpg`)                   |
| `--sample=HxV,HxV,HxV`   | Facteurs de sous-échantillonnage pour Y, Cb, Cr                    |

```bash
# Couleur sans sous-échantillonnage
./ppm2jpeg images/etu/shaun_the_sheep.ppm

# 4:2:0 avec fichier de sortie explicite
./ppm2jpeg --sample=2x2,1x1,1x1 --outfile=out/result.jpg images/etu/horizontal.ppm

# Niveaux de gris
./ppm2jpeg images/etu/invader.pgm
```

---

## Sous-échantillonnage chrominance

Le flag `--sample=H0xV0,H1xV1,H2xV2` définit les facteurs pour Y, Cb et Cr. Une MCU contient alors `H0*V0` blocs Y, `H1*V1` blocs Cb et `H2*V2` blocs Cr.
quelque modes conseillees:

| Mode  | `--sample`     | Remarque                                   |
|-------|----------------|--------------------------------------------|
| 4:4:4 | `1x1,1x1,1x1` | Pas de sous-échantillonnage (défaut)        |
| 4:2:2 | `2x1,1x1,1x1` | Sous-échantillonnage horizontal seulement   |
| 4:2:0 | `2x2,1x1,1x1` | Le plus courant, horizontal + vertical      |
| 4:1:1 | `4x1,1x1,1x1` | Sous-échantillonnage horizontal fort        |


---

## Tests

### Unitaires (Unity)

```bash
make all
./tests/test_dct.bin
./tests/test_zz.bin
```

### Intégration (pytest)

```bash
make tests
```

### Couverture

```bash
make couverture
```


---
## Répartition des tâches

| Membre                      | Modules                                                                                                        |
|-----------------------------|----------------------------------------------------------------------------------------------------------------|
| **Saad Boubekri**           | Parseur PPM/PGM (de l'image complete + ligne par ligne des MCU), padding, buffer glissant, interface CLI (`parser_resize.c`, `main.c`)                        |
| **Ayman Boulaich**          | RGB→YCbCr, DCT (naïve + optimisée 1D séparable), zig-zag, quantification, découpage MCU et sampling factors (`rgb_ycbcr.c`, `dct.c`, `zz.c`, `quantification.c`, `mcu_compression.c`) |
| **Mohammed Amine Zerouali** | Codage RLE et de Huffman des AC/DC, écriture du bitstream et des marqueurs JPEG (`codage_rle.c`, `make_JPEG.c`) |
