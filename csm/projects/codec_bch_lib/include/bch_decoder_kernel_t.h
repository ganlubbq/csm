//*******************************************************************************
// Title: Communication System Modeler v.1.1
// File: bch_decoder_kernel_t.h
// Author: Pavel Morozkin
// Date: August 28th 2013
// Revised: August 28th 2013
//*******************************************************************************
// NOTE:
// The author is not responsible for any malfunctioning of this program, nor for
// any damage caused by it. Please include the original program along with these
// comments in any redistribution.
//
// For more information and suggestions, please email me: pavelmorozkin@yandex.ru
//
// COPYRIGHT NOTICE: This computer program is free for non-commercial purposes.
// You may implement this program for any non-commercial application. You may
// also implement this program for commercial purposes, provided that you
// obtain my written permission. Any modification of this program is covered 
// by this copyright.
//
// Copyright (c) 2013, Pavel Morozkin. All rights reserved.
//*******************************************************************************
#ifndef _BCH_DECODER_KERNEL_T_
#define _BCH_DECODER_KERNEL_T_

#include "common.h"

#include "codeword_t.h"
#include "frame_t.h"
#include "bch_codec_kernel.h"

#include <stdio.h>

#define SELF bch_decoder_kernel_t self
typedef struct bch_decoder_kernel bch_decoder_kernel_base_t;
typedef bch_decoder_kernel_base_t* bch_decoder_kernel_t;

struct bch_decoder_kernel {
	int galois_field_degree;
	int code_length;
	int inf_symbols_q;
	int error_correction;
	FILE* log;
	bch_codec_kernel_vars_t bch_decoder_kernel_vars;
	int (*start) (SELF);
	int (*stop) (SELF);
	frame_t (*decode) (SELF, codeword_t codeword_out, codeword_t codeword_in);
};

void bch_decoder_kernel_init (SELF);
void bch_decoder_kernel_deinit (SELF);

bch_decoder_kernel_t bch_decoder_kernel_create 
(
	FILE* log,
	int galois_field_degree,
	int code_length,
	int inf_symbols_q,
	int error_correction
);
void bch_decoder_kernel_destroy (SELF);

frame_t bch_decoder_kernel_decode (SELF, codeword_t codeword_out, codeword_t codeword_in);

int bch_decoder_kernel_start (SELF);
int bch_decoder_kernel_stop (SELF);

unsigned int bch_decoder_kernel_get_founded_errors(SELF);
unsigned int bch_decoder_kernel_get_recovered_bits(SELF);

#undef SELF

#endif