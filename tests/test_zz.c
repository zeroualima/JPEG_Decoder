#include "unity.h"
#include "zz.h"

void setUp(void) {
    // set stuff up here
}

void tearDown(void) {
    // clean stuff up here
}

void test2(void){
	TEST_ASSERT(1 > 0);
	TEST_ASSERT_FALSE(2+2==5);
//	TEST_FAIL_MESSAGE("Parce que la vie est injuste!");
}

void test_position(void) {
    /* 
        j'ai change le type des donnees de "uint8_t" a "int16_t"
        pour les adapter a la fonction "zz_bloc"
    */

    int16_t identite[64] = {
        0, 1, 2, 3, 4, 5, 6, 7,
        8, 9, 10, 11, 12, 13, 14, 15,
        16, 17, 18, 19, 20, 21, 22, 23,
        24, 25, 26, 27, 28, 29, 30, 31,
        32, 33, 34, 35, 36, 37, 38, 39,
        40, 41, 42, 43, 44, 45, 46, 47,
        48, 49, 50, 51, 52, 53, 54, 55,
        56, 57, 58, 59, 60, 61, 62, 63
    };
    int16_t zzi[64] = {
	0, 1, 8, 16, 9, 2, 3, 10, 
	17, 24, 32, 25, 18, 11, 4, 5, 
	12, 19, 26, 33, 40, 48, 41, 34, 
	27, 20, 13, 6, 7, 14, 21, 28, 
	35, 42, 49, 56, 57, 50, 43, 36, 
	29, 22, 15, 23, 30, 37, 44, 51, 
	58, 59, 52, 45, 38, 31, 39, 46, 
	53, 60, 61, 54, 47, 55, 62, 63 
    };
    
    /* j'ai pas utilise "zz" proposee */

    int16_t output[64];

    zz_bloc(identite, output);

    TEST_ASSERT_EQUAL_UINT8_ARRAY(zzi,output,64);
}


int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_position);
    RUN_TEST(test2);
    return UNITY_END();
}
