#include <stdio.h>
#include <sys/time.h>
#include "csv.h"

int
main(int argc, char **argv) {
    struct timeval start, stop;
    double ms;
    int code = 0;
    csv_read_t ret;
    csv_t *csv;

    if (argc < 2) {
        printf("Usage: csv_test [filename]\n");
        return 1;
    }

    printf("Opening %s\n", argv[1]);
    gettimeofday(&start, NULL);

    csv = csv_init();
    if (csv == NULL) {
        printf("Out of memory\n");
        return 1;
    }

    if (!csv_open_file(csv, argv[1], 0)) {
        printf("Failed to open file: %s\n", csv_error(csv));
        code = 1;
    }
    else {
        while (1) {
            ret = csv_read(csv);
            if (ret == CSV_READ_ERROR) {
                printf("Error: %s\n", csv_error(csv));
                code = 1;
                break;
            }
            if (ret == CSV_READ_EOF)
                break;
        }
    }

    csv_free(csv);

    gettimeofday(&stop, NULL);

    ms = (stop.tv_sec - start.tv_sec) * 1000.0;
    ms += (stop.tv_usec - start.tv_usec) / 1000.0;

    if (ms > 1000.0)
        printf("Parsed in %.2f seconds\n", ms / 1000.0);
    else
        printf("Parsed in %f milliseconds\n", ms);

    return code;
}
