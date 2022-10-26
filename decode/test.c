#include <stdio.h>
#include <stdint.h>
#include <strings.h>
#include <stdlib.h>

#include "dav1d/dav1d.h"

#define BUFFER_SIZE 1000

// gcc test.c -Idav1d/include -ldav1d -Ldav1d/lib
// USE test_out.bin (or anything that binarizes test/av1.webm)

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

    int obu_sizes[] = {14, 27, 69, 40, 22};

    size_t num_read;
    num_read = fread(buffer, 1, BUFFER_SIZE, file);
    for (int i = 0; i < 5; i++) {
        if (num_read == 0) {
            break;
        }

        status =
            dav1d_data_wrap(&data, buffer, obu_sizes[i], fake_dealloc, NULL);
        printf("data wrap status = %i\n", status);
        buffer += obu_sizes[i];
        status = dav1d_send_data(context, &data);
        printf("data send status = %i\n", status);
    }

    status = dav1d_get_picture(context, &pic);
    printf("get picture status = %i\n", status);

    dav1d_close(&context);

    printf("%s\n", dav1d_version());

    free(buffer);
    fclose(file);
    return 0;
}