#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

// https://www.delftstack.com/howto/c/read-binary-file-in-c/

typedef struct
{
    int32_t    mAs;
    int16_t    mA;
    int16_t    mV;
} dataset_t;

const char* filename = "history.dat";


int print_data(dataset_t data[], int items)
{
    int rows = 0;

    for (int i = 0; i < items; i++)
    {
        float Ah   = data[i].mAs / (1000.0 * 3600.0);
        float Ubus = data[i].mV  /  1000.0;
        float Ibus = data[i].mA  /  1000.0;

        if ( data[i].mAs )  // Skip empty rows
        {
            printf("ix=%d   V=%6.3f     A=%7.3f      Ah=%7.3f     mAs=%d\n", i, Ubus, Ibus, Ah, data[i].mAs);
            rows++;
        }
    }
    return rows;
}


int print_gnuplot(dataset_t data[], int items)
{
    #define  SAMPLE_PERIOD  10.0   // Seconds

    float time = 0.0;
    float dt   = SAMPLE_PERIOD / 3600.0;
    int   rows = 0;

    for (int i = 0; i < items; i++)
    {
        float Ah   = data[i].mAs / (1000.0 * 3600.0);
        float Ubus = data[i].mV  /  1000.0;
        float Ibus = data[i].mA  /  1000.0;

        if ( data[i].mAs )  // Skip empty rows
        {
            printf("%7.3f  %7.3f  %7.3f  %6.3f\n", time, Ah, Ibus, Ubus);
            rows++;
        }
        time += dt;
    }
    return rows;
}


char *file_read(int *filesize, const char *filename)
{
    FILE* in_file = fopen(filename, "rb");
    if (!in_file) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }
    struct stat sb;
    if (stat(filename, &sb) == -1) {
        perror("stat");
        exit(EXIT_FAILURE);
    }

    char *file_contents = malloc(sb.st_size);
    memset(file_contents, 0, sb.st_size);

    fread(file_contents, sb.st_size, 1, in_file);
    fclose(in_file);

    *filesize = sb.st_size;
    return file_contents;
}


int main(void)
{
    char *file_contents;
    int   file_size;
    int   data_items;
    int   data_rows;

    file_contents = file_read(&file_size, filename);
    data_items    = file_size / sizeof(dataset_t);

    printf("# file_size=%d  data_items=%d\n", file_size, data_items);

//  data_rows = print_data(    (dataset_t *)file_contents, data_items );
    data_rows = print_gnuplot( (dataset_t *)file_contents, data_items );

    printf("# data_rows =%d\n", data_rows);

    free(file_contents);
    exit(EXIT_SUCCESS);
}
