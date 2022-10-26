#include <stdio.h>
#include <stdint.h>
#include <strings.h>
#include <stdlib.h>

#include "dav1d/dav1d.h"

#define BUFFER_SIZE 10

// gcc test.c -Idav1d/include -ldav1d -Ldav1d/lib

void
fake_dealloc(const uint8_t *data, void *cookie)
{
    return;
}

int
main(int argc, char **argv)
{
    if (argc < 2) {
        printf("Input filename argument expected\n");
        exit(1);
    }

    uint8_t *buffer = malloc(BUFFER_SIZE * sizeof(uint8_t));
    FILE *file = fopen(argv[1], "rb");

    Dav1dSettings settings = {0};
    dav1d_default_settings(&settings);

    Dav1dContext *context;
    int status = dav1d_open(&context, &settings);
    printf("open status = %i\n", status);

    Dav1dData data = {0};
    Dav1dPicture pic = {0};

    size_t num_read;
    while (1) {
        num_read = fread(buffer, 1, BUFFER_SIZE, file);
        if (num_read == 0) {
            break;
        }

        status = dav1d_data_wrap(&data, buffer, num_read, fake_dealloc, NULL);
        printf("data wrap status = %i\n", status);

        int b_draining = 0;
        do {
            status = dav1d_send_data(context, &data);
            if (status < 0 && status != DAV1D_ERR(EAGAIN)) {
                printf("Decoder feed error %d!\n", status);
                break;
            }

            int b_output_error = 0;
            do {
                status = dav1d_get_picture(context, &pic);
                if (status != DAV1D_ERR(EAGAIN)) {
                    printf("Decoder error %d!\n", status);
                    b_output_error = 1;
                    break;
                }
            } while (status == 0);

            if (b_output_error)
                break;

            /* on drain, we must ignore the 1st EAGAIN */
            if (!b_draining && (status == DAV1D_ERR(EAGAIN) || status == 0)) {
                b_draining = 1;
                status = 0;
            }
        } while (status == 0 || data.sz != 0);

        // status = dav1d_send_data(context, &data);
        // printf("send data status = %i\n", status);

        // status = dav1d_get_picture(context, &pic);
        // printf("get picture status = %i\n", status);
    }

    dav1d_close(&context);

    printf("%s\n", dav1d_version());

    free(buffer);
    fclose(file);
    return 0;
}