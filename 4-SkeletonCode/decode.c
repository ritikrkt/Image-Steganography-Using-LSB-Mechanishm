#include <stdio.h>
#include <string.h>

#include "decode.h"
#include "encode.h"
#include "common.h"
#include "types.h"


Status read_and_validate_decode_args(char *argv[], DecodeInfo *decInfo)
{
    /* Stego image should be .bmp */
    if (strstr(argv[2], ".bmp") != NULL)
    {
        decInfo->stego_image_fname = argv[2];
    }
    else
    {
        return e_failure;
    }

    /* Output file name */
    if (argv[3] != NULL)
    {
        decInfo->output_fname = argv[3];
    }
    else
    {
        decInfo->output_fname = "output.txt";
    }

    return e_success;
}
Status open_decode_files(DecodeInfo *decInfo)
{
    decInfo->fptr_stego_image = fopen(decInfo->stego_image_fname, "rb");

    if (decInfo->fptr_stego_image == NULL)
    {
        perror("fopen");
        return e_failure;
    }

    decInfo->fptr_output = fopen(decInfo->output_fname, "wb");

    if (decInfo->fptr_output == NULL)
    {
        perror("fopen");
        return e_failure;
    }

    return e_success;
}
char decode_byte_from_lsb(char *image_buffer)
{
    char ch = 0;

    for (int i = 0; i < 8; i++)
    {
        ch |= (image_buffer[i] & 1) << i;
    }

    return ch;
}
int decode_size_from_lsb(char *buffer)
{
    int size = 0;

    for (int i = 0; i < 32; i++)
    {
        size |= (buffer[i] & 1) << i;
    }

    return size;
}
Status decode_magic_string(DecodeInfo *decInfo)
{
    char img[8];
    char magic[3];

    for(int i = 0; i < 2; i++)
    {
        fread(img,8,1,decInfo->fptr_stego_image);
        magic[i] = decode_byte_from_lsb(img);
    }

    magic[2]='\0';

    if(strcmp(magic,"#*")==0)
        return e_success;

    return e_failure;
}
Status decode_secret_file_extn_size(int *size, DecodeInfo *decInfo)
{
    char buffer[32];

    fread(buffer, 32, 1, decInfo->fptr_stego_image);

    *size = decode_size_from_lsb(buffer);

    return e_success;
}
Status decode_secret_file_extn(char *extn, int size, DecodeInfo *decInfo)
{
    char img[8];

    for (int i = 0; i < size; i++)
    {
        fread(img, 8, 1, decInfo->fptr_stego_image);
        extn[i] = decode_byte_from_lsb(img);
    }

    extn[size] = '\0';

    return e_success;
}
Status decode_secret_file_size(long *size, DecodeInfo *decInfo)
{
    char buffer[32];

    fread(buffer, 32, 1, decInfo->fptr_stego_image);

    *size = decode_size_from_lsb(buffer);

    return e_success;
}
Status decode_secret_file_data(DecodeInfo *decInfo)
{
    char img[8];

    for (int i = 0; i < decInfo->size_secret_file; i++)
    {
        fread(img, 8, 1, decInfo->fptr_stego_image);

        char ch = decode_byte_from_lsb(img);

        fwrite(&ch, 1, 1, decInfo->fptr_output);
    }

    return e_success;
}
Status do_decoding(DecodeInfo *decInfo)
{
    if (open_decode_files(decInfo) == e_failure)
    {
        printf("ERROR: Opening decode files failed\n");
        return e_failure;
    }

    printf("INFO: Files opened successfully\n");

    /* Skip BMP Header */
    fseek(decInfo->fptr_stego_image, 54, SEEK_SET);

    if (decode_magic_string(decInfo) == e_failure)
    {
        printf("ERROR: Magic string mismatch\n");
        return e_failure;
    }

    printf("INFO: Magic string decoded successfully\n");

    int extn_size;

    decode_secret_file_extn_size(&extn_size, decInfo);
    printf("INFO: Extension size decoded\n");

    decode_secret_file_extn(decInfo->extn_secret_file, extn_size, decInfo);
    printf("INFO: Extension decoded = %s\n", decInfo->extn_secret_file);

    decode_secret_file_size(&decInfo->size_secret_file, decInfo);
    printf("INFO: Secret file size = %ld\n", decInfo->size_secret_file);

    decode_secret_file_data(decInfo);
    printf("INFO: Secret file data decoded successfully\n");

    fclose(decInfo->fptr_stego_image);
    fclose(decInfo->fptr_output);

    return e_success;
}
