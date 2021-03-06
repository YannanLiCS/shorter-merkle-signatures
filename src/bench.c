/*
 * Copyright (C) 2015-2017 Geovandro Pereira
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <string.h>
#include <time.h>
#include "bench.h"
#include "mss.h"


#ifdef VERBOSE
#include "util.h"
#endif

#define BENCH_KEYGEN 1
#define BENCH_SIGNATURE ((unsigned long) 1 << MSS_HEIGHT)
#define MSG_LEN_BENCH 16
#define HASH_LEN LEN_BYTES(WINTERNITZ_N)

struct mss_node nodes[2];
struct mss_state state_bench;
struct mss_node currentLeaf_bench;
struct mss_node authpath_bench[MSS_HEIGHT];
mmo_t hash1, hash2;

unsigned char pkey_test[NODE_VALUE_SIZE];

unsigned char seed[LEN_BYTES(WINTERNITZ_N)] = {0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF, 0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF};
unsigned char h1[HASH_LEN], h2[HASH_LEN];
unsigned char sig_bench[WINTERNITZ_L*HASH_LEN];
unsigned char aux[HASH_LEN];

void bench_mss_signature() {
    unsigned long i;
    unsigned char k;
    unsigned char si[LEN_BYTES(WINTERNITZ_N)], ri[LEN_BYTES(WINTERNITZ_N)];
    clock_t elapsed;

    char M[1 << MSS_HEIGHT][MSG_LEN_BENCH];
    
    for (i = 0; i < ((unsigned long) 1 << MSS_HEIGHT); i++)
        for (k = 0; k < MSG_LEN_BENCH; k++)
            M[i][k] = (char) 2 * i + k;

    MMO_init(&hash1);
    MMO_init(&hash2);
    memcpy(si, seed, LEN_BYTES(WINTERNITZ_N));
    
    printf("\n\nBenchmarking MSS operations. Signature and verification are run %d times.\n", 1 << MSS_HEIGHT);
    
    printf("\nBenchmarking MSS key gen with %d execution(s)...\n", BENCH_KEYGEN);
    
    elapsed = -clock();
    for (k = 0; k < BENCH_KEYGEN; k++)
        mss_keygen_core(&hash1, &hash2, seed, &nodes[0], &nodes[1], &state_bench, pkey_test);
    elapsed += clock();
    printf("Elapsed: %.1f ms\n\n", 1000 * (float) elapsed / CLOCKS_PER_SEC / BENCH_KEYGEN);

    printf("Benchmarking MSS sign ...\n");

    elapsed = -clock();
    for (i = 0; i < BENCH_SIGNATURE; i++) {
        fsgen(si, si, ri);
        mss_sign_core(&state_bench, si, ri, &currentLeaf_bench, (const char *) M[i], MSG_LEN_BENCH, &hash1, h1, i, &nodes[0], &nodes[1], sig_bench, authpath_bench);
    }
    
    elapsed += clock();
    printf("Elapsed: %.1f ms\n\n", 1000 * (float) elapsed / CLOCKS_PER_SEC / BENCH_SIGNATURE);

    printf("Benchmarking MSS verify ...\n");
    elapsed = -clock();
    for (i = 0; i < BENCH_SIGNATURE; i++)
        mss_verify_core(authpath_bench, (const char *) M[i], MSG_LEN_BENCH, h1, i, sig_bench, aux, &currentLeaf_bench, pkey_test);

    elapsed += clock();
    printf("Elapsed: %.1f ms\n\n", 1000 * (float) elapsed / CLOCKS_PER_SEC / BENCH_SIGNATURE);

}

void bench_hash() {

    clock_t elapsed;
    unsigned long k, hashbenchs = 10000;
    
    unsigned char data[hashbenchs][2*MSG_LEN_BENCH];
    unsigned char digest[hashbenchs][HASH_LEN];

    for (k = 0; k < hashbenchs; k++) {
        for (int i = 0; i < 2*MSG_LEN_BENCH; i++) {
            data[k][i] = 2*k + i;
        }
    }
    
    printf("Running %lu times each hash function operation.\n\n", hashbenchs);
    
    printf("Benchmarking underlying hash - init ...\n");
    
    elapsed = -clock();
    for (k = 0; k < hashbenchs; k++)
        MMO_init(&hash1);
    elapsed += clock();
    printf("Elapsed: %.1f us\n\n", (1000000 * (float) elapsed) / CLOCKS_PER_SEC / hashbenchs);
    
    printf("Benchmarking underlying hash - MMO update...\n");
    elapsed = -clock();
    for (k = 0; k < hashbenchs; k++) {
        MMO_update(&hash1, data[k], MSG_LEN_BENCH);
    }
    elapsed += clock();
    printf("Elapsed: %.1f us\n\n", 1000000 * (float) elapsed / CLOCKS_PER_SEC / hashbenchs);

    printf("Benchmarking underlying hash - MMO final...\n");
    elapsed = -clock();
    for (k = 0; k < hashbenchs; k++) {
        MMO_final(&hash1, digest[k]);
    }
    elapsed += clock();
    printf("Elapsed: %.1f us\n\n", 1000000 * (float) elapsed / CLOCKS_PER_SEC / hashbenchs);    
        
    printf("Benchmarking a fixed 16-byte input/ouput MMO hash ...\n");
    elapsed = -clock();    
    for (k = 0; k < hashbenchs; k++) {
        MMO_hash16(&hash1, data[k], digest[k]);
    }
    elapsed += clock();
    printf("Elapsed: %.1f us\n\n", 1000000 * (float) elapsed / CLOCKS_PER_SEC / hashbenchs);    
    
    printf("Benchmarking a fixed 32-byte input, 16-byte output MMO hash ...\n");
    elapsed = -clock();    
    for (k = 0; k < hashbenchs; k++) {
        MMO_hash32(&hash1, data[k], data[k], digest[k]);
    }
    elapsed += clock();
    printf("Elapsed: %.1f us\n\n", 1000000 * (float) elapsed / CLOCKS_PER_SEC / hashbenchs);    
    
    printf("Benchmarking a fixed 32-byte input, 32-byte output SHA256 hash ...\n");
    elapsed = -clock();    
    for (k = 0; k < hashbenchs; k++) {
        hash32(data[k], 2*MSG_LEN_BENCH , digest[k]);
    }
    elapsed += clock();
    printf("Elapsed: %.1f us\n\n", 1000000 * (float) elapsed / CLOCKS_PER_SEC / hashbenchs);     
    
}

void do_bench(enum BENCH operation) {

    switch (operation) {
        case BENCH_MSS:
            bench_mss_signature();
            break;
        case BENCH_HASH:
            bench_hash();
            break;
        default:
            break;
    }

}

int main() {
    
    printf("\nParameters:  WINTERNITZ_N=%u, Tree_Height=%u, Treehash_K=%u, WINTERNITZ_w=%u \n\n", MSS_SEC_LVL, MSS_HEIGHT, MSS_K, WINTERNITZ_W);
    
    do_bench(BENCH_HASH);
    do_bench(BENCH_MSS);    
    
    return 0;
}

