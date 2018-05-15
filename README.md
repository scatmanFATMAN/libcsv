# libcsv

Simple, fast, and lightweight CSV parser written in C.

## Features

* Small, fast, and lightweight.
* Handles Windows and Linux line endings.
* Stream files for extremely low memory use.
* Handle files without a header.
* Allow field values to automatically be trimmed.

## Installation

Installation on Linux can be done using the Makefile.
```
sudo make install
```

## Getting Started

The following example demonstrates how to parse a file in streaming mode.

```c
#include <stdlib.h>
#include <stdio.h>
#include <csv.h>

int main(int argc, char **argv) {
    csv_t *csv;
    csv_read_t ret;

    csv = csv_init();
    if (csv == NULL) {
       printf("Out of memory\n");
       exit(1);
    }

    if (!csv_open_file(csv, "/tmp/myfile.csv", 0)) {
       printf("Error opening file: %s\n", csv_error(csv));
    }
    else {
       while (1) {
          ret = csv_read(csv);

          if (ret == CSV_READ_ERROR) {
             printf("Read error: %s\n", csv_error(csv));
             break;
          }

          if (ret == CSV_READ_EOF) {
             break;
          }

          printf("Field 0 is: %s\n", csv_get(csv, 0));
       }

       csv_close(csv);
    }

    csv_free(csv);
    return 0;
}
```

## TODO

* Offer a mode to always allocate and deallocate storage for fields. This might be useful for systems that have low memory available.
* Handle UTF-8.
* Implement RFC 4180 completely.

## License

The MIT License (MIT). Please see [LICENSE](LICENSE) for more information.
