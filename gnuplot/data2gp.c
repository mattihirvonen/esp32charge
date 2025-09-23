#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

// https://www.delftstack.com/howto/c/read-binary-file-in-c/

#define  SAMPLE_FILE    "history.dat"   // Default sample sample data file
#define  SAMPLE_PERIOD  10.0            // Default sample period in seconds

typedef struct
{
    int32_t    mAs;
    int16_t    mA;
    int16_t    mV;
} dataset_t;

typedef struct
{
    char       sample_file[64];
    float      sample_period;
} args_t;




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


int print_gnuplot(dataset_t data[], int items, args_t *args)
{
    float time = 0.0;
    float dt   = args->sample_period / 3600.0;
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
            time += dt;
        }
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


float parse_arg2float(char *arg)
{
    float value;

    sscanf(arg, "%f", &value);
  //printf("%s =  %f\n", __func__, value);
    return value;
}


void parse_args(args_t *args, int argc, char *argv[])
{
    int opt;

    // put ':' in the starting of the
    // string so that program can
    //distinguish between '?' and ':'
    while((opt = getopt(argc, argv, ":s:f:ilrx")) != -1)
    {
        switch(opt)
        {
            case 'i':
            case 'l':
            case 'r':
                printf("option: %c\n", opt);
                break;

            case 'f':
            //  printf("sample_file: %s\n", optarg);
                strcpy(args->sample_file, optarg);
                break;
            case 's':
                args->sample_period = parse_arg2float(optarg);
                break;
            case ':':
                printf("option needs a value\n");
                break;
            case '?':
                printf("unknown option: %c\n", optopt);
                exit(1);
                break;
        }
    }

    // optind is for the extra arguments
    // which are not parsed
    for(; optind < argc; optind++){
    //  printf("extra arguments: %s\n", argv[optind]);
        strcpy(args->sample_file, argv[optind]);
    }
}


int main(int argc, char *argv[])
{
    char   *file_contents;
    int     file_size;
    int     data_items;
    int     data_rows;
    args_t  args;

    strcpy(args.sample_file, SAMPLE_FILE);
    args.sample_period = SAMPLE_PERIOD;
    parse_args(&args, argc, argv);

    file_contents = file_read(&file_size, args.sample_file);
    data_items    = file_size / sizeof(dataset_t);

    printf("# file_name=%s  file_size=%d  data_items=%d\n", args.sample_file, file_size, data_items);

//  data_rows = print_data(    (dataset_t *)file_contents, data_items );
    data_rows = print_gnuplot( (dataset_t *)file_contents, data_items, &args);

    printf("# data_rows=%d\n", data_rows);

    free(file_contents);
    exit(EXIT_SUCCESS);
}
