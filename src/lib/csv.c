#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include "csv.h"

//#define CSV_DEBUG 1

#if defined(CSV_DEBUG)
# define CSV_PRINTF(fmt, ...)       \
    do {                            \
        printf(fmt, ##__VA_ARGS__); \
        fflush(stdout);             \
    } while (0)
#else
# define CSV_PRINTF(fmt, ...)
#endif

/**
 * Different types of modes that the CSV parser can be in.
 */
#define CSV_MODE_FILE_ALLOCATE     1    /**< Parse a file and load the entire file into memory. */
#define CSV_MODE_FILE_NO_ALLOCATE  2    /**< Parse a file but only read from the file as needed. This is also called streaming mode. */
#define CSV_MODE_STR_ALLOCATE      3    /**< Parse a string a make a copy of it in memory. */
#define CSV_MODE_STR_NO_ALLOCATE   4    /**< Parse a string but don't make a copy of it in memory. */

/**
 * Various flags that affect CSV parsing.
 */
#define CSV_FLAG_NO_HEADER  1   /**< The file doesn't have a header. */
#define CSV_FLAG_LEFT_TRIM  2   /**< Left trim CSV fields. */
#define CSV_FLAG_RIGHT_TRIM 4   /**< Right trim CSV fields. */

/**
 * A CSV field.
 */
typedef struct {
    char *str;              /**< The field value. */
    unsigned int capacity;  /**< The buffer size allocated to <tt>str</tt>. */
    unsigned int len;       /**< The field's string length. */
} csv_field_t;

/**
 * A CSV handle.
 */
struct csv_t {
    FILE *f;                    /**< The file descriptor when reading from a file. */
    char *buf;                  /**< The buffer used to store the CSV document. */
    char *buf_ptr;              /**< A pointer to the current position within <tt>buf</tt> during parsing. */
    unsigned int buf_capacity;  /**< The buffer size allocated to <tt>buf</tt>. */
    unsigned int buf_len;       /**< The buffer length of <tt>buf</tt>. */
    unsigned int line;          /**< The current line within the CSV document during parsing. */
    csv_field_t *fields;        /**< An array of fields use to storage field values during parsing. */
    unsigned int size;          /**< The number of fields. */
    unsigned int read_size;     /**< The amount of data to read when parsing a file in streaming mode. */
    int mode;                   /**< Which mode the CSV handle is using to parse the CSV document. */
    int flags;                  /**< Bitwise field of flags that effect parsing. */
    char error[64];             /**< An error string when an operation fails. */
};

/**
 * The return result when reading a field.
 */
typedef enum {
    CSV_READ_FIELD_OK,      /**< A field was read successfully. */
    CSV_READ_FIELD_EOL,     /**< No more fields exist on this line. */
    CSV_READ_FIELD_ERROR    /**< An error occured. */
} csv_read_field_t;

csv_t *
csv_init() {
    csv_t *csv;

    csv = calloc(1, sizeof(*csv));
    if (csv != NULL) {
        csv->read_size = 1024;
    }

    return csv;
}

static void
csv_free_helper(csv_t *csv) {
    unsigned int i;

    if (csv->f != NULL) {
        fclose(csv->f);
    }

    if (csv->mode != CSV_MODE_STR_NO_ALLOCATE) {
        if (csv->buf != NULL) {
            free(csv->buf);
        }
    }

    if (csv->fields != NULL) {
        for (i = 0; i < csv->size; i++) {
            if (csv->fields[i].str != NULL)
                free(csv->fields[i].str);
        }

        free(csv->fields);
    }

    csv->f = NULL;
    csv->buf = NULL;
    csv->buf_ptr = NULL;
    csv->buf_capacity = 0;
    csv->buf_len = 0;
    csv->line = 0;
    csv->fields = NULL;
    csv->size = 0;
    csv->mode = 0;
    csv->error[0] = '\0';
}

void
csv_free(csv_t *csv) {
    if (csv != NULL) {
        csv_free_helper(csv);
        free(csv);
    }
}

void
csv_set_read_size(csv_t *csv, unsigned int read_size) {
    csv->read_size = read_size;
}

static void
csv_set_flag(csv_t *csv, int flag, int on) {
    if (on) {
        csv->flags |= flag;
    }
    else {
        csv->flags &= ~flag;
    }
}

void
csv_set_header(csv_t *csv, int value) {
    csv_set_flag(csv, CSV_FLAG_NO_HEADER, value ? 0 : 1);
}

void
csv_set_left_trim(csv_t *csv, int value) {
    csv_set_flag(csv, CSV_FLAG_LEFT_TRIM, value);
}

void
csv_set_right_trim(csv_t *csv, int value) {
    csv_set_flag(csv, CSV_FLAG_RIGHT_TRIM, value);
}

void
csv_set_trim(csv_t *csv, int value) {
    csv_set_left_trim(csv, value);
    csv_set_right_trim(csv, value);
}

const char *
csv_error(csv_t *csv) {
    return csv->error;
}

int
csv_open_file(csv_t *csv, const char *path, int allocate) {
    unsigned long size;
    csv_read_t ret;

    csv->f = fopen(path, "r");
    if (csv->f == NULL) {
        strcpy(csv->error, strerror(errno));
        return 0;
    }

    if (allocate) {
        fseek(csv->f, 0, SEEK_END);
        size = ftell(csv->f);
        fseek(csv->f, 0, SEEK_SET);

        csv->buf = malloc(size + 1);
        if (csv->buf == NULL) {
            strcpy(csv->error, "Out of memory");
            fclose(csv->f);
            csv->f = NULL;
            return 0;
        }

        if (fread(csv->buf, sizeof(char), size, csv->f) != size) {
            snprintf(csv->error, sizeof(csv->error), "Read error: %s", strerror(errno));
            free(csv->buf);
            fclose(csv->f);
            csv->f = NULL;
            return 0;
        }

        csv->buf[size] = '\0';

        fclose(csv->f);
        csv->f = NULL;

        csv->mode = CSV_MODE_FILE_ALLOCATE;
    }
    else {
        csv->mode = CSV_MODE_FILE_NO_ALLOCATE;
    }

    csv->buf_ptr = csv->buf;

    //read the header
    ret = csv_read(csv);
    if (ret != CSV_READ_OK) {
        if (csv->f != NULL) {
            fclose(csv->f);
            csv->f = NULL;
        }
        return 0;
    }

    return 1;
}

int
csv_open_str(csv_t *csv, const char *str, int allocate) {
    csv_read_t ret;

    if (allocate) {
        csv->buf = strdup(str);
        if (csv->buf == NULL) {
            strcpy(csv->error, "Out of memory");
            return 0;
        }

        csv->mode = CSV_MODE_STR_ALLOCATE;
    }
    else {
        csv->buf = (char *)str; //we won't modify it, i promise ;)
        csv->mode = CSV_MODE_STR_NO_ALLOCATE;
    }

    csv->buf_ptr = csv->buf;

    //read the header
    ret = csv_read(csv);
    if (ret != CSV_READ_OK) {
        if (allocate) {
            free(csv->buf);
            csv->buf = NULL;
        }

        return 0;
    }

    return 1;
}

void
csv_close(csv_t *csv) {
    csv_free_helper(csv);
}

static int
csv_add_field(csv_t *csv, unsigned int index, char *str, unsigned int len, int escape, int quote, int first) {
    unsigned int i, j;

    if (first) {
        CSV_PRINTF("Adding header %u\n", index);
        ++csv->size;
    }
    else {
        if (len > 0 && !quote) {
            if (csv->flags & CSV_FLAG_LEFT_TRIM) {
                while (len > 0 && *str == ' ') {
                    ++str;
                    --len;
                }
            }

            if (csv->flags & CSV_FLAG_RIGHT_TRIM) {
                while (len > 0 && str[len - 1] == ' ') {
                    --len;
                }
            }
        }

        CSV_PRINTF("Adding field: index: %u, length: %u\n", index, len);

        if (len == 0) {
            if (csv->fields[index].capacity > 0)
                csv->fields[index].str[0] = '\0';
            csv->fields[index].len = 0;
        }
        else {
            if (len > csv->fields[index].capacity) {
                CSV_PRINTF("  New field allocation: capacity %u -> %u\n", csv->fields[index].capacity, len);

                if (csv->fields[index].str != NULL)
                    free(csv->fields[index].str);

                csv->fields[index].str = malloc(len + 1);
                if (csv->fields[index].str == NULL) {
                    strcpy(csv->error, "Out of memory");
                    return 0;
                }

                csv->fields[index].capacity = len;
            }

            if (escape) {
                for (i = j = 0; i < len; i++, j++) {
                    if (str[i] == '"' && str[i + 1] == '"') {
                        csv->fields[index].str[j] = '"';
                        ++i;
                    }
                    else {
                        csv->fields[index].str[j] = str[i];
                    }
                }

                csv->fields[index].str[j] = '\0';
                csv->fields[index].len = j;
            }
            else {
                memcpy(csv->fields[index].str, str, len);
                csv->fields[index].str[len] = '\0';
                csv->fields[index].len = len;
            }
        }

        CSV_PRINTF("  Value: '%s'\n", len == 0 ? "" : csv->fields[index].str);
    }

    return 1;
}

static csv_read_field_t
csv_read_field(csv_t *csv, unsigned int index, int first) {
    int quote = 0, escape = 0;
    char *end;

    //RFC 4180 states that spaces should be preserved
    //skip leading spaces
//    while (*csv->buf_ptr == ' ') {
//        ++csv->buf_ptr;
//    }

    if (*csv->buf_ptr == '"') {
        quote = 1;
        ++csv->buf_ptr;
    }

    end = csv->buf_ptr;

    while (1) {
        if (quote) {
            if (*end == '"') {
                if (*(end + 1) == '"') {
                    escape = 1;
                    end += 2;
                }
                else {
                    if (!csv_add_field(csv, index, csv->buf_ptr, end - csv->buf_ptr, escape, quote, first)) {
                        return CSV_READ_FIELD_ERROR;
                    }

                    ++end;

                    while (*end != '\0' && *end != ',' && *end != '\r' && *end != '\n')
                        ++end;

                    break;
                }
            }
            else {
                ++end;
            }
        }
        else {
            if (*end == '\0' || *end == ',' || *end == '\r' || *end == '\n') {
                if (!csv_add_field(csv, index, csv->buf_ptr, end - csv->buf_ptr, escape, quote, first)) {
                    return CSV_READ_FIELD_ERROR;
                }

                break;
            }
            else if (*end == '"') {
                quote = 1;
                ++end;
                csv->buf_ptr = end;
            }
            else {
                ++end;
            }
        }
    }

    if (*end == ',')
        ++end;
    csv->buf_ptr = end;

    if (*csv->buf_ptr == '\0' || *csv->buf_ptr == '\r' || *csv->buf_ptr == '\n') {
        while (*csv->buf_ptr == '\r' || *csv->buf_ptr == '\n') {
            ++csv->buf_ptr;
        }
//        if (*csv->buf_ptr == '\r')
//            ++csv->buf_ptr;
//        if (*csv->buf_ptr == '\n')
//            ++csv->buf_ptr;
        return CSV_READ_FIELD_EOL;
    }

    return CSV_READ_FIELD_OK;
}

static int
csv_read_line(csv_t *csv, int first) {
    csv_read_field_t ret;
    unsigned int index = 0;

    ++csv->line;

    CSV_PRINTF("Reading line %u\n", csv->line);

    while (1) {
        //read a field
        if (!first) {
            if (index == csv->size) {
                snprintf(csv->error, sizeof(csv->error), "Found more than %u fields on line %u", csv->size, csv->line);
                return 0;
            }
        }

        ret = csv_read_field(csv, index, first);
        if (ret == CSV_READ_FIELD_OK) {
            ++index;
            continue;
        }
        if (ret == CSV_READ_FIELD_EOL) {
            ++index;
            break;
        }
        if (ret == CSV_READ_FIELD_ERROR) {
            return 0;
        }

        snprintf(csv->error, sizeof(csv->error), "Unknown field read result: %d", ret);
        return 0;
    }

    if (first) {
        csv->fields = calloc(csv->size, sizeof(csv_field_t));
        if (csv->fields == NULL) {
            strcpy(csv->error, "Out of memory");
            return 0;
        }

        if (csv->flags & CSV_FLAG_NO_HEADER) {
            csv->buf_ptr = csv->buf;
            --csv->line;
        }
    }
    else {
        if (index != csv->size) {
            snprintf(csv->error, sizeof(csv->error), "Expected %u fields but found %u on line %u", csv->size, index, csv->line);
            return 0;
        }
    }

//    while (*csv->buf_ptr != '\0' && isspace(*csv->buf_ptr))
//        ++csv->buf_ptr;

    return 1;
}

static csv_read_t
csv_read_file_more(csv_t *csv) {
    unsigned int new_capacity, count;
    char *new_buf;

    if (csv->buf_len + csv->read_size >= csv->buf_capacity) {
        new_capacity = csv->buf_capacity + csv->read_size;

        CSV_PRINTF("Increasing buffer from %u to %u\n", csv->buf_capacity, new_capacity);

        new_buf = realloc(csv->buf, new_capacity + 1);
        if (new_buf == NULL) {
            strcpy(csv->error, "Out of memory");
            return CSV_READ_ERROR;
        }

        csv->buf = new_buf;
        csv->buf_capacity = new_capacity;
    }

    count = fread(csv->buf + csv->buf_len, sizeof(char), csv->read_size, csv->f);
    csv->buf_len += count;
    csv->buf[csv->buf_len] = '\0';

    if (count != csv->read_size) {
        if (ferror(csv->f)) {
            strcpy(csv->error, strerror(errno));
            return CSV_READ_ERROR;
        }

        if (count == 0) {
            return CSV_READ_EOF;
        }
    }

    return CSV_READ_OK;
}

csv_read_t
csv_read(csv_t *csv) {
    unsigned int i = 0;
    int has_line = 0;
    csv_read_t ret;

    if (csv->mode == CSV_MODE_FILE_NO_ALLOCATE) {
        if (csv->f == NULL) {
            strcpy(csv->error, "No file descriptor open");
            return CSV_READ_ERROR;
        }

        if (csv->buf_len == 0 && feof(csv->f)) {
            return CSV_READ_EOF;
        }

        while (1) {
            if (csv->buf_len > 0) {
                for (; i < csv->buf_len; i++) {
                    if (csv->buf[i] == '\0')
                        break;
                    if (csv->buf[i] == '\r' || csv->buf[i] == '\n') {
                        has_line = 1;
                        break;
                    }
                }

                if (has_line) {
                    break;
                }
            }

            ret = csv_read_file_more(csv);
            if (ret != CSV_READ_OK) {
                return ret;
            }
        }

        csv->buf_ptr = csv->buf;
        if (!csv_read_line(csv, csv->size == 0)) {
            return CSV_READ_ERROR;
        }

        memmove(csv->buf, csv->buf_ptr, csv->buf_capacity - (csv->buf_ptr - csv->buf));
        csv->buf_len -= (csv->buf_ptr - csv->buf);
        csv->buf[csv->buf_len] = '\0';

        return CSV_READ_OK;
    }
    else if (csv->mode == CSV_MODE_FILE_ALLOCATE || csv->mode == CSV_MODE_STR_ALLOCATE || csv->mode == CSV_MODE_STR_NO_ALLOCATE) {
        if (*csv->buf_ptr == '\0')
            return CSV_READ_EOF;

        if (!csv_read_line(csv, csv->size == 0))
            return CSV_READ_ERROR;

        return CSV_READ_OK;
    }

    snprintf(csv->error, sizeof(csv->error), "Unknown mode %d", csv->mode);
    return CSV_READ_ERROR;
}

const char *
csv_get(csv_t *csv, unsigned int index) {
    if (csv->fields == NULL) {
        return NULL;
    }

    if (index >= csv->size) {
        return NULL;
    }

    if (csv->fields[index].str == NULL || csv->fields[index].str[0] == '\0') {
        return NULL;
    }

    return csv->fields[index].str;
}
