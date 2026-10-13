#ifndef DECODE_H
#define DECODE_H

#include <stdio.h>
#include "types.h"

#define MAX_DECODE_SECRET_BUF_SIZE 100
#define MAX_DECODE_FILE_SUFFIX 10

typedef struct _DecodeInfo
{
    char *stego_image_fname;
    FILE *fptr_stego_image;

    char *output_fname;
    FILE *fptr_output;

    char extn_secret_file[MAX_DECODE_FILE_SUFFIX];
    long size_secret_file;

} DecodeInfo;

Status read_and_validate_decode_args(char *argv[], DecodeInfo *decInfo);
Status open_decode_files(DecodeInfo *decInfo);
Status do_decoding(DecodeInfo *decInfo);

Status decode_magic_string(DecodeInfo *decInfo);
Status decode_secret_file_extn_size(int *size, DecodeInfo *decInfo);
Status decode_secret_file_extn(char *extn, int size, DecodeInfo *decInfo);
Status decode_secret_file_size(long *size, DecodeInfo *decInfo);
Status decode_secret_file_data(DecodeInfo *decInfo);

char decode_byte_from_lsb(char *image_buffer);
int decode_size_from_lsb(char *buffer);

#endif