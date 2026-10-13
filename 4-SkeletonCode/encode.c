#include <stdio.h>
#include <string.h>
#include "encode.h"
#include "types.h"
#include "common.h"
#include <stdlib.h> 

/* Function Definitions */
//OperationType check_operation_type(char *argv[])
//{
  //  if (strcmp(argv[1], "-e") == 0)
    //{
      //  return e_encode;
    //}
    //else if (strcmp(argv[1], "-d") == 0)
    //{
      //  return e_decode;
    //}
    //else
    //{
      //  return e_unsupported;
    //}
//}

/* Get image size
 * Input: Image file ptr
 * Output: width * height * bytes per pixel (3 in our case)
 * Description: In BMP Image, width is stored in offset 18,
 * and height after that. size is 4 bytes
 */
uint get_image_size_for_bmp(FILE *fptr_image)
{
    uint width, height;
    // Seek to 18th byte
    fseek(fptr_image, 18, SEEK_SET);

    // Read the width (an int)
    fread(&width, sizeof(int), 1, fptr_image);
    printf("width = %u\n", width);

    // Read the height (an int)
    fread(&height, sizeof(int), 1, fptr_image);
    printf("height = %u\n", height);

    // Return image capacity
    return width * height * 3;
}

/* 
 * Get File pointers for i/p and o/p files
 * Inputs: Src Image file, Secret file and
 * Stego Image file
 * Output: FILE pointer for above files
 * Return Value: e_success or e_failure, on file errors
 */
Status open_files(EncodeInfo *encInfo)
{
    // Src Image file
    encInfo->fptr_src_image = fopen(encInfo->src_image_fname, "rb");
    // Do Error handling
    if (encInfo->fptr_src_image == NULL)
    {
    	perror("fopen");
    	fprintf(stderr, "ERROR: Unable to open file %s\n", encInfo->src_image_fname);

    	return e_failure;
    }

    // Secret file
    encInfo->fptr_secret = fopen(encInfo->secret_fname, "rb");
    // Do Error handling
    if (encInfo->fptr_secret == NULL)
    {
    	perror("fopen");
    	fprintf(stderr, "ERROR: Unable to open file %s\n", encInfo->secret_fname);

    	return e_failure;
    }

    // Stego Image file
    encInfo->fptr_stego_image = fopen(encInfo->stego_image_fname, "wb");
    // Do Error handling
    if (encInfo->fptr_stego_image == NULL)
    {
    	perror("fopen");
    	fprintf(stderr, "ERROR: Unable to open file %s\n", encInfo->stego_image_fname);

    	return e_failure;
    }

    // No failure return e_success
    return e_success;
}
Status read_and_validate_encode_args(char *argv[], EncodeInfo *encInfo)
{
    /* input files .bmp */
    if (strstr(argv[2], ".bmp") != NULL)
    {
        encInfo->src_image_fname = argv[2];

    }
    else
    {
        return e_failure;
    }

    /* input Secret.txt */
    if (strstr(argv[3], ".txt") != NULL)
    {
        encInfo->secret_fname = argv[3];
    }
    else
    {
        return e_failure;
    }

    /* Get secret file extension */
    char *ext = strrchr(argv[3], '.');
    if (ext == NULL)
    {
        return e_failure;
    }
    strcpy(encInfo->extn_secret_file, ext);

    /* Output image */
    if (argv[4] != NULL)
    {
        if (strstr(argv[4], ".bmp") != NULL)
            encInfo->stego_image_fname = argv[4];
        else
            return e_failure;
    }
    else
    {
        encInfo->stego_image_fname = "default.bmp";
    }

    return e_success;
}
uint get_file_size(FILE *fptr)
{
    fseek(fptr,0,SEEK_END);

    uint size = ftell(fptr);

    rewind(fptr);

    return size;
}
Status check_capacity(EncodeInfo *encInfo)
{
    encInfo->image_capacity = get_image_size_for_bmp(encInfo->fptr_src_image);
    encInfo->size_secret_file = get_file_size(encInfo->fptr_secret);
    if (encInfo->image_capacity > 16 + 32 + 32 + 32 + encInfo->size_secret_file * 8)
    {
        return e_success;
    }
    else
    {
        return e_failure;
    }
}
Status copy_bmp_header(FILE *fptr_src_image, FILE *fptr_dest_image)
{
    char buffer[54];
    rewind(fptr_src_image);
    fread(buffer, 54, 1, fptr_src_image); //Read 54 bytes from source image
    fwrite(buffer, 54, 1, fptr_dest_image); //Write 54 bytes to destination image
    return e_success;
}
Status encode_byte_to_lsb(char data, char *image_buffer)
{
    for(int i = 0; i < 8; i++)
    {
        image_buffer[i] = (image_buffer[i] & 0xFE) | ((data >> i ) & 1); 
    }
    return e_success;
}
Status encode_data_to_image(char *data, int size, EncodeInfo *encInfo)
{
    //read 8 bytes
    for(int i = 0; i < size; i++)
    {
        fread(encInfo->image_data, 8, 1, encInfo->fptr_src_image);
        //byte to lsb
        encode_byte_to_lsb(data[i], encInfo->image_data);
        //write encoded data to output
        fwrite(encInfo->image_data, 8, 1, encInfo->fptr_stego_image);
    }
    return e_success;
}

Status encode_magic_string(char *magic_string, EncodeInfo *encInfo)
{
    encode_data_to_image(magic_string, strlen(magic_string), encInfo);
    return e_success;
}
Status encode_size_to_lsb(int size, EncodeInfo *encInfo)
{
    char str[32];
    fread(str, 32, 1, encInfo->fptr_src_image);
    //encode size (int -32 bits) in 32 bytes
    for(int i = 0; i < 32; i++)
    {
        str[i] = (str[i] & 0xFE) | ((size >> i ) & 1); 
    }
    fwrite(str, 32, 1, encInfo->fptr_stego_image);
    return e_success;
}
Status encode_secret_file_extn_size(int size, EncodeInfo *encInfo)
{
    encode_size_to_lsb(size, encInfo);
    return e_success;
}
Status encode_secret_file_extn( char *file_extn, EncodeInfo *encInfo)
{
    encode_data_to_image(file_extn, strlen(file_extn), encInfo);
    return e_success;
}
Status encode_secret_file_size(long file_size, EncodeInfo *encInfo)
{
    encode_size_to_lsb(file_size, encInfo);
    return e_success;
}
Status encode_secret_file_data(EncodeInfo *encInfo)
{
    char data[encInfo->size_secret_file];
    rewind(encInfo->fptr_secret);
    fread(data, 1, encInfo->size_secret_file, encInfo->fptr_secret);
    encode_data_to_image(data, encInfo->size_secret_file, encInfo);
    return e_success;
}
Status copy_remaining_img_data(EncodeInfo *encInfo)
{
    int size = 54 + encInfo->image_capacity - ftell(encInfo->fptr_src_image);
    char *buffer = (char *)malloc(size);
    if (buffer == NULL)
    {
        printf("Memory allocation failed\n");
        return e_failure;
    }
    fread(buffer, 1, size, encInfo->fptr_src_image);
    fwrite(buffer, 1, size, encInfo->fptr_stego_image);
    free(buffer);
    return e_success;
}
Status do_encoding(EncodeInfo *encInfo)
{
    /* Open all files */
    if (open_files(encInfo) == e_failure)
    {
        printf("ERROR: Open files failed\n");
        return e_failure;
    }

    printf("INFO: Files opened successfully\n");

    /* Check capacity */
    if (check_capacity(encInfo) == e_success)
    {
        printf("INFO: Check capacity is successful\n");
        if (copy_bmp_header(encInfo -> fptr_src_image, encInfo -> fptr_stego_image) == e_success)
        {
            printf("INFO: Copy bmp header is successful\n");
            if(encode_magic_string(MAGIC_STRING, encInfo) == e_success)
            {
                printf("INFO: Magic string encoded successfully\n");
                strcpy(encInfo->extn_secret_file, strstr(encInfo->secret_fname, "."));
                if(encode_secret_file_extn_size(strlen(encInfo->extn_secret_file), encInfo) == e_success)
                {
                    printf("INFO: Secret file extension size encoded successfully\n");
                    if(encode_secret_file_extn(encInfo->extn_secret_file, encInfo) == e_success)
                    {
                        printf("INFO: Secret file extension encoded successfully\n");
                        if(encode_secret_file_size(encInfo->size_secret_file, encInfo) == e_success)
                        {
                            printf("INFO: Secret file size encoded successfully\n");
                            if(encode_secret_file_data(encInfo) == e_success)
                            {
                                printf("INFO: Secret file data encoded successfully\n");
                                if(copy_remaining_img_data(encInfo) == e_success)
                                {
                                    printf("INFO: Remaining image data copied successfully\n");
                                }
                                else
                                {
                                    printf("ERROR: Copy remaining image data failed\n");
                                    return e_failure;
                                }
                            }
                            else
                            {
                                printf("ERROR: Secret file data encoding failed\n");
                                return e_failure;
                            }
                        }
                        else
                        {
                            printf("ERROR: Secret file size encoding failed\n");
                            return e_failure;
                        }
                    }
                    else
                    {
                        printf("ERROR: Secret file extension encoding failed\n");
                        return e_failure;
                    }
                }
                else
                {
                    printf("ERROR: Secret file extension size encoding failed\n");
                    return e_failure;
                }
            }
            else
            {
                printf("ERROR: Magic string encoding failed\n");
                return e_failure;
            }
        }
        else
        {
            printf("ERROR: Copy bmp header failed\n");
            return e_failure;
        }
    }
    else
    {
        printf("ERROR: Check capacity failed\n");
        return e_failure;
    }

    return e_success;
}