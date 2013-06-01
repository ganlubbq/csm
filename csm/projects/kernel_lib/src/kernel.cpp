//*******************************************************************************
// Title: Communication System Modeler v.1.0
// File: kernel.cpp
// Author: Pavel Morozkin
// Date: May 31th 2013
// Revised: May 31th 2013
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
#include "kernel.h"
#include <string.h>

int kernel_run
	(
	int galois_field_degree,
	int bch_code_length,
	int error_correction,
	int decoded_seq_buf_size_frames,
	double ber,
	char* out_file_postfix,
	char* input_file_name,
	int gui_progress)
{
	FILE* flog = stdout;

	/************************************************************************/
	/* ��������� ��������� ���������.                                       */
	/************************************************************************/
	char* file_path = input_file_name;
	int* data_in = get_file_data(file_path);
	int data_size = get_file_size(file_path) * CHAR_BIT;
	//print_data(data_in, data_size);
	int* data_out = NULL;

	int use_bch_codec = 0;

	/************************************************************************/
	/* ��������� ���-������                                                 */
	/************************************************************************/
	//int galois_field_degree = 4;
	//int bch_code_length = 15;
	//int error_correction = 3;

	/************************************************************************/
	/* ��������� ���-������                                                 */
	/************************************************************************/
	//double ber = 0;//2*1e-5;

	/************************************************************************/
	/* ��������������� ����������.                                          */
	/************************************************************************/
	int skip_q_cnt = 0;
	int percent_precision = gui_progress ? (int)1e3 : 5;
	int sent_bits_cnt = 0;

	/************************************************************************/
	/* �������� ����������� �����������.                                    */
	/************************************************************************/
	bch_encoder_t bch_encoder = bch_encoder_create(flog, galois_field_degree, bch_code_length, error_correction);
	bch_decoder_t bch_decoder = bch_decoder_create(flog, galois_field_degree, bch_code_length, error_correction);

	/* ��������� ����� ������ ������, ������� ����� ���������� ���-�����. */
	int bch_frame_size = bch_encoder_get_frame_size(bch_encoder);
	int bch_codeword_size = bch_encoder_get_codeword_size(bch_encoder);

	/************************************************************************/
	/* �������� ����������� �����������.                                    */
	/************************************************************************/
	transmitter_t transmitter = transmitter_create(flog, bch_frame_size, data_in, data_size);
	receiver_t receiver = receiver_create(flog, bch_frame_size, data_size);

	/************************************************************************/
	/* ��������� ����������� ������.                                        */
	/************************************************************************/
	int regs_q = 2; /* ! do not change */
	int codeword_length = bch_code_length * regs_q;
	//int decoded_seq_buf_size_frames = 2;
	int decoded_seq_buf_size = bch_code_length * decoded_seq_buf_size_frames + 1;
	int skip_q = decoded_seq_buf_size_frames;

	/************************************************************************/
	/* �������� ����������� �����������.                                    */
	/************************************************************************/
	cnv_encoder_t cnv_encoder = cnv_encoder_create(flog, regs_q, codeword_length);
	cnv_decoder_t cnv_decoder = cnv_decoder_create(flog, regs_q, decoded_seq_buf_size);

	/************************************************************************/
	/* �������� ����������� �����������.                                    */
	/************************************************************************/
	channel_bs_t channel_bs = channel_bs_create(flog, ber);

	/************************************************************************/
	/* ��������� ������������� �������                                      */
	/************************************************************************/
	int bch_frame_fifo_size = decoded_seq_buf_size_frames + 1;
	int bch_codeword_fifo_size = decoded_seq_buf_size_frames + 1;

	/************************************************************************/
	/* �������� ������������� �������.                                      */
	/************************************************************************/
	frame_fifo_t bch_frame_fifo = frame_fifo_create(bch_frame_fifo_size, bch_frame_size);
	codeword_fifo_t bch_codeword_fifo = codeword_fifo_create(bch_codeword_fifo_size, bch_codeword_size);

	/************************************************************************/
	/* ������ ����������� �����������.                                      */
	/************************************************************************/
	transmitter->start(transmitter);
	receiver->start(receiver);
	bch_encoder->start(bch_encoder);
	bch_decoder->start(bch_decoder);
	cnv_encoder->start(cnv_encoder);
	cnv_decoder->start(cnv_decoder);
	channel_bs->start(channel_bs);

	/************************************************************************/
	/* ������ ����� �������������.                                          */
	/************************************************************************/
	int send_bits_all = data_size+decoded_seq_buf_size_frames*bch_frame_size;
L_Loop:
	if (!(sent_bits_cnt % percent_precision) && !gui_progress)
		printf("\rProgress: %.0f%%", ((float)sent_bits_cnt/
		(float)send_bits_all) * 100);
	else if (!(sent_bits_cnt % percent_precision))
		printf("Progress: %.0f%%\n", ((float)sent_bits_cnt/
		(float)send_bits_all) * 100);
	sent_bits_cnt += bch_frame_size;

	/* ��������� ������ ������ ������������. */
	frame_t bch_frame_in = transmitter->transmit_frame(transmitter);
	if (bch_frame_in == NULL) goto L_Dummy_Generation;

	frame_display(bch_frame_in);

	/* ������ ���������������� ������ ������ � ������������� �����. */
	bch_frame_fifo = frame_fifo_put(bch_frame_fifo, bch_frame_in);

	/* ����������� ������ ������ ���-������� � ��������� �������� ����� ���-����. */
	codeword_t bch_codeword_in = bch_encoder->encode(bch_encoder, bch_frame_in);
	codeword_display(bch_codeword_in);

	/* �������������� �������� ����� ���-���� � ������ ������ ��� ����������� ������. */
	frame_t cnv_frame_in = convert_codeword_to_frame(bch_codeword_in);
	frame_display(cnv_frame_in);

	/* ����������� ������ ������ ���������� ������� � ��������� �������� ����� ����������� ����.*/
	codeword_t cnv_codeword_in = cnv_encoder->encode(cnv_encoder, cnv_frame_in);
	goto L_Skip_Dummy_Generation;

L_Dummy_Generation:
	bch_codeword_in = codeword_create(bch_encoder->code_length);
	cnv_codeword_in = codeword_create(cnv_encoder->codeword_length);

L_Skip_Dummy_Generation:
	/* ������ ���������������� �������� ����� ���-���� � ������������� �����. */
	bch_codeword_fifo = codeword_fifo_put(bch_codeword_fifo, bch_codeword_in);
	codeword_destroy(bch_codeword_in); 
	codeword_display(cnv_codeword_in);

	/* �������� �������� ����� ����������� ���� �� ������. */
	codeword_t cnv_codeword_out = channel_bs->transfer(channel_bs, cnv_codeword_in);
	codeword_display(cnv_codeword_out);	

	/* ������������� �������� ����� ����������� ���� � ��������� ������ ������. */
	frame_t cnv_frame_out = cnv_decoder->decode(cnv_decoder, cnv_codeword_out, cnv_codeword_in);
	frame_display(cnv_frame_out);

	/* �������������� ��������������� ������ ������ � �������� ����� ��� ���-�������������. */
	codeword_t bch_codeword_out = convert_frame_to_codeword(cnv_frame_out);
	codeword_display(bch_codeword_out);	
	frame_destroy(cnv_frame_out);

	/* �������� �������� ���������. */
	if(skip_q_cnt != skip_q) { 
		skip_q_cnt++; 
		codeword_destroy(bch_codeword_out); 
		goto L_Loop;
	}

	/* ���������� �� �������������� ������ �������� ����� ���-����. */
	bch_codeword_in = codeword_fifo_get(bch_codeword_fifo);
	codeword_display(bch_codeword_in);

	/* ������������� �������� ����� ���-��������� � ��������� ������ ������. */
	frame_t bch_frame_out = bch_decoder->decode(bch_decoder, bch_codeword_out, bch_codeword_in);
	frame_display(bch_frame_out);

	/* ���������� �� �������������� ������ ������ ������. */
	bch_frame_in = frame_fifo_get(bch_frame_fifo);
	frame_display(bch_frame_in);

	/* ����� ������ ������ � ������ ��� ������������. */
	receiver->receive_frame(receiver, bch_frame_out, bch_frame_in);

	/* �������� ������� ������� � ������. */
	if(frame_fifo_is_empty(bch_frame_fifo))
		goto L_Stop;
	else 
		goto L_Loop;

	/************************************************************************/
	/* ������� ����������� �����������.                                     */
	/************************************************************************/
L_Stop:
	printf("\rProgress: 100%%\n");

	transmitter->stop(transmitter);
	receiver->stop(receiver);
	bch_encoder->stop(bch_encoder);
	bch_decoder->stop(bch_decoder);
	cnv_encoder->stop(cnv_encoder);
	cnv_decoder->stop(cnv_decoder);
	channel_bs->stop(channel_bs);

	/************************************************************************/
	/* �������� ������������ �������� ������.                               */
	/************************************************************************/
	data_out = receiver_get_received_data(receiver);

	if (cmp_data(data_in, data_out, data_size))
		printf("test: OK\n");
	else
		printf("test: FAIL\n");

	free(data_in);

	out_file_postfix = extend_out_file_postfix(
		out_file_postfix,
		galois_field_degree,
		bch_code_length,
		error_correction,
		decoded_seq_buf_size,
		ber);
	put_file_data(data_out, file_path, out_file_postfix);

	//print_data(data_in, data_size);
	//print_data(data_out, data_size);

	/************************************************************************/
	/* ����������� ����������� �����������.                                 */
	/************************************************************************/
	transmitter_destroy(transmitter);
	receiver_destroy(receiver);
	bch_encoder_destroy(bch_encoder);
	bch_decoder_destroy(bch_decoder);
	cnv_encoder_destroy(cnv_encoder);
	cnv_decoder_destroy(cnv_decoder);
	channel_bs_destroy(channel_bs);
	frame_fifo_destroy(bch_frame_fifo);
	codeword_fifo_destroy(bch_codeword_fifo);

	return 0;
}

int kernel_run_01
	(
	int galois_field_degree,
	int bch_code_length,
	int error_correction,
	int decoded_seq_buf_size_frames,
	double ber,
	char* out_file_postfix,
	char* input_file_name,
	int gui_progress)
{
	FILE* flog = stdout;

	/************************************************************************/
	/* ��������� ��������� ���������.                                       */
	/************************************************************************/
	char* file_path = input_file_name;
	int* data_in = get_file_data(file_path);
	int data_size = get_file_size(file_path) * CHAR_BIT;
	//print_data(data_in, data_size);
	int* data_out = NULL;

	int use_bch_codec = 0;

	/************************************************************************/
	/* ��������� ���-������                                                 */
	/************************************************************************/
	//double ber = 0;//2*1e-5;

	/************************************************************************/
	/* ��������������� ����������.                                          */
	/************************************************************************/
	int skip_q_cnt = 0;
	int percent_precision = gui_progress ? (int)1e3 : 5;
	int sent_bits_cnt = 0;

	/************************************************************************/
	/* �������� ����������� �����������.                                    */
	/************************************************************************/
	int frame_size = 10;
	transmitter_t transmitter = transmitter_create(flog, frame_size, data_in, data_size);
	receiver_t receiver = receiver_create(flog, frame_size, data_size);

	/************************************************************************/
	/* ��������� ����������� ������.                                        */
	/************************************************************************/
	int regs_q = 2; /* ! do not change */
	int codeword_length = frame_size * regs_q;
	//int decoded_seq_buf_size_frames = 2;
	int decoded_seq_buf_size = frame_size * decoded_seq_buf_size_frames + 1;
	int skip_q = decoded_seq_buf_size_frames;

	/************************************************************************/
	/* �������� ����������� �����������.                                    */
	/************************************************************************/
	cnv_encoder_t cnv_encoder = cnv_encoder_create(flog, regs_q, codeword_length);
	cnv_decoder_t cnv_decoder = cnv_decoder_create(flog, regs_q, decoded_seq_buf_size);

	/************************************************************************/
	/* �������� ����������� �����������.                                    */
	/************************************************************************/
	channel_bs_t channel_bs = channel_bs_create(flog, ber);

	/************************************************************************/
	/* ��������� ������������� �������                                      */
	/************************************************************************/
	int frame_fifo_size = decoded_seq_buf_size_frames + 1;

	/************************************************************************/
	/* �������� ������������� �������.                                      */
	/************************************************************************/
	frame_fifo_t frame_fifo = frame_fifo_create(frame_fifo_size, frame_size);

	/************************************************************************/
	/* ������ ����������� �����������.                                      */
	/************************************************************************/
	transmitter->start(transmitter);
	receiver->start(receiver);
	cnv_encoder->start(cnv_encoder);
	cnv_decoder->start(cnv_decoder);
	channel_bs->start(channel_bs);

	/************************************************************************/
	/* ������ ����� �������������.                                          */
	/************************************************************************/
	int send_bits_all = data_size+decoded_seq_buf_size_frames*frame_size;
L_Loop:
	if (!(sent_bits_cnt % percent_precision) && !gui_progress)
		printf("\rProgress: %.0f%%", ((float)sent_bits_cnt/
		(float)send_bits_all) * 100);
	else if (!(sent_bits_cnt % percent_precision))
		printf("Progress: %.0f%%\n", ((float)sent_bits_cnt/
		(float)send_bits_all) * 100);
	sent_bits_cnt += frame_size;

	/* ��������� ������ ������ ������������. */
	frame_t frame_in = transmitter->transmit_frame(transmitter);
	if (frame_in == NULL) goto L_Dummy_Generation;

	frame_display(frame_in);

	/* ������ ���������������� ������ ������ � ������������� �����. */
	frame_fifo = frame_fifo_put(frame_fifo, frame_in);
	
	/* ����������� ������ ������ ���������� ������� � ��������� �������� ����� ����������� ����.*/
	codeword_t cnv_codeword_in = cnv_encoder->encode(cnv_encoder, frame_in);
	goto L_Skip_Dummy_Generation;

L_Dummy_Generation:
	cnv_codeword_in = codeword_create(cnv_encoder->codeword_length);

L_Skip_Dummy_Generation:
	codeword_display(cnv_codeword_in);

	/* �������� �������� ����� ����������� ���� �� ������. */
	codeword_t cnv_codeword_out = channel_bs->transfer(channel_bs, cnv_codeword_in);
	codeword_display(cnv_codeword_out);	

	/* ������������� �������� ����� ����������� ���� � ��������� ������ ������. */
	frame_t frame_out = cnv_decoder->decode(cnv_decoder, cnv_codeword_out, cnv_codeword_in);
	frame_display(frame_out);

	/* �������� �������� ���������. */
	if(skip_q_cnt != skip_q) { 
		skip_q_cnt++; 
		goto L_Loop;
	}

	/* ���������� �� �������������� ������ ������ ������. */
	frame_in = frame_fifo_get(frame_fifo);
	frame_display(frame_in);

	/* ����� ������ ������ � ������ ��� ������������. */
	receiver->receive_frame(receiver, frame_out, frame_in);

	/* �������� ������� ������� � ������. */
	if(frame_fifo_is_empty(frame_fifo))
		goto L_Stop;
	else 
		goto L_Loop;

	/************************************************************************/
	/* ������� ����������� �����������.                                     */
	/************************************************************************/
L_Stop:
	printf("\rProgress: 100%%\n");

	transmitter->stop(transmitter);
	receiver->stop(receiver);
	cnv_encoder->stop(cnv_encoder);
	cnv_decoder->stop(cnv_decoder);
	channel_bs->stop(channel_bs);

	/************************************************************************/
	/* �������� ������������ �������� ������.                               */
	/************************************************************************/
	data_out = receiver_get_received_data(receiver);

	if (cmp_data(data_in, data_out, data_size))
		printf("test: OK\n");
	else
		printf("test: FAIL\n");

	free(data_in);

	out_file_postfix = extend_out_file_postfix(
		out_file_postfix,
		galois_field_degree,
		bch_code_length,
		error_correction,
		decoded_seq_buf_size,
		ber);
	put_file_data(data_out, file_path, out_file_postfix);

	//print_data(data_in, data_size);
	//print_data(data_out, data_size);

	/************************************************************************/
	/* ����������� ����������� �����������.                                 */
	/************************************************************************/
	transmitter_destroy(transmitter);
	receiver_destroy(receiver);
	cnv_encoder_destroy(cnv_encoder);
	cnv_decoder_destroy(cnv_decoder);
	channel_bs_destroy(channel_bs);
	frame_fifo_destroy(frame_fifo);

	return 0;
}
