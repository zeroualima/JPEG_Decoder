# JPEG Encoder

[Dashboard](https://formationc.pages.ensimag.fr/projet/jpeg/2026/13_boubekrs_boulaiay_zerouama)

This project implements a JPEG encoder from scratch in C. It takes PPM (color) or PGM (grayscale) images as input and produces valid JPEG files compliant with the JFIF standard.

---

## Compression Pipeline

Processing is done MCU by MCU (Minimum Coded Unit). Here are the steps in order:

```
<File>.ppm/pgm : handled in main.c

          │
          ▼
┌─────────────────────┐
│  Parsing & padding  │  parser_resize.c
│  Image reading      │  Line-by-line reading, padding to multiples of 8*v[0] and 8*h[0]
└─────────┬───────────┘
          │  rgb_mcu  (raw pixels per MCU)
          ▼
┌─────────────────────┐
│  RGB → YCbCr        │  rgb_ycbcr.c    
└─────────┬───────────┘
          │  ycbcr_mcu  (separated Y, Cb, Cr components)
          ▼
┌─────────────────────┐
│  Block splitting    │  mcu_compression.c
│  + subsampling      │  8x8 blocks, Cb/Cr subsampling according to H*V factors 
└─────────┬───────────┘
          │  bloc[]  (array of 8x8 blocks of type Y, Cb, or Cr)
          ▼
┌─────────────────────┐
│  2D DCT             │  dct.c
│  (1D separable)     │  Discrete cosine transform on each 8x8 block
└─────────┬───────────┘
          │
          ▼
┌─────────────────────┐
│  Zig-zag scan       │  zz.c
│                     │  Zig-zag reordering
└─────────┬───────────┘
          │
          ▼
┌─────────────────────┐
│  Quantization       │  quantification.c
│                     │  Distinct Q tables for Y and Cb/Cr
└─────────┬───────────┘
          │
          ▼
┌─────────────────────┐
│  RLE/Huffman coding │  codage_rle.c + htables.c
│                     │  Differential DC coding, AC run-length + standard JPEG Huffman tables
└─────────┬───────────┘
          │
          ▼
┌─────────────────────┐
│  JPEG writing       │  make_JPEG.c
│                     │  Writing markers (SOI, APP0, DQT, SOF0, DHT, SOS, Raw data, EOI)
└─────────────────────┘
          │
          ▼
    <File>.jpg     : finally!!

```

---

## Data Structures

The main structures flowing through the pipeline:

```c
/* Subsampling factors for the 3 components (Y, Cb, Cr) */
typedef struct {
    uint8_t h[3];   // horizontal factors
    uint8_t v[3];   // vertical factors
} sampling_factors;

/* Represents the source image with its metadata and a sliding buffer for reading */
typedef struct {
    int w, h;               // original dimensions
    int pw, ph;             // dimensions after padding (multiples of 8*h[0] and 8*v[0])
    int colors;             // 1 = PGM, 3 = PPM
    uint8_t *row_buffer;    // sliding window of mcu_height pixel lines
    long header_offset;     // offset of the first pixel byte in the file
    FILE *f;
    // ...
} Image;

/* Raw RGB MCU — an array of pixels, each pixel = [R,G,B] or [Y] */
typedef struct { RGB *data; } rgb_mcu;

/* MCU after conversion, separated components */
typedef struct {
    uint8_t *Y, *Cb, *Cr;
} ycbcr_mcu;

/* An 8x8 block of coefficients, with its type to know which Huffman table to use */
typedef struct {
    int16_t data[64];
    bloc_type type;   // Y, Cb, or Cr
} bloc;

```

---

## Compilation

### Prerequisites

* GCC (C99)
* `make`
* Python 3 + pip (integration tests)
* `gcovr` (coverage)
* `valgrind` + `kcachegrind` (profiling, optional)

### Make Targets

| Command | Description |
| --- | --- |
| `make all` | Compiles the encoder + tests (ASan + coverage enabled) |
| `make debug` | Clean recompile in debug mode (`-Og`, ASan) |
| `make perf` | Recompiles in `-O3` without instrumentation for performance measurement |
| `make tests` | Runs the entire test suite (Unity + pytest) |
| `make couverture` | Tests + HTML coverage report |
| `make profilage` | Callgrind on the test image + opens in kcachegrind |
| `make clean` | Removes binaries |
| `make realclean` | Complete cleanup before commit |

```bash
make all    # standard compilation
make debug  # if you want to debug with gdb

```

---

## Usage

```
./ppm2jpeg [--outfile=<file.jpg>] [--sample=H1xV1,H2xV2,H3xV3] <input.ppm|input.pgm>

```

| Option | Description |
| --- | --- |
| `--outfile=<file.jpg>` | Output file (default: `out/<name>.jpg`) |
| `--sample=HxV,HxV,HxV` | Subsampling factors for Y, Cb, Cr |

```bash
# Color without subsampling
./ppm2jpeg images/etu/shaun_the_sheep.ppm

# 4:2:0 with explicit output file
./ppm2jpeg --sample=2x2,1x1,1x1 --outfile=out/result.jpg images/etu/horizontal.ppm

# Grayscale
./ppm2jpeg images/etu/invader.pgm

```

---

## Chrominance Subsampling

The `--sample=H0xV0,H1xV1,H2xV2` flag sets the factors for Y, Cb, and Cr. An MCU then contains `H0*V0` Y blocks, `H1*V1` Cb blocks, and `H2*V2` Cr blocks.
Some recommended modes:

| Mode | `--sample` | Notes |
| --- | --- | --- |
| 4:4:4 | `1x1,1x1,1x1` | No subsampling (default) |
| 4:2:2 | `2x1,1x1,1x1` | Horizontal subsampling only |
| 4:2:0 | `2x2,1x1,1x1` | Most common, horizontal + vertical |
| 4:1:1 | `4x1,1x1,1x1` | Strong horizontal subsampling |

---

## Tests

### Unit Tests (Unity)

```bash
make all
./tests/test_dct.bin
./tests/test_zz.bin
./tests/test_all.py # We added many tests to properly evaluate the program

# Several pgm and ppm images taken from the USC-SIPI Image Database for testing purposes only.
./images/test_pics
./images/test_pics/pgm_tests
./images/test_pics/ppm_tests

```

### Integration (pytest)

```bash
make tests

```

### Coverage

```bash
make couverture

```

---

## Task Distribution

| Member | Modules |
| --- | --- |
| **Saad Boubekri** | PPM/PGM parser (complete image + line-by-line MCU reading), padding, sliding buffer, CLI interface (`parser_resize.c`, `main.c`, `progressif.c`) |
| **Ayman Boulaich** | RGB→YCbCr, DCT (naive + 1D separable optimized), zig-zag, quantization, MCU splitting, and sampling factors (`rgb_ycbcr.c`, `dct.c`, `zz.c`, `quantification.c`, `mcu_compression.c`, `traitement_mcu.c`, `progressif.c`) |
| **Mohammed Amine Zerouali** | RLE and Huffman coding for AC/DC, bitstream and JPEG markers writing (`codage_rle.c`, `make_JPEG.c`, `progressif.c`) |
