#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "csv.h"

typedef enum {
    TEST_TYPE_FILE,
    TEST_TYPE_STR
} test_type_t;

typedef struct {
    unsigned int row;
    unsigned int col;
    const char *value;
} test_condition_t;

typedef struct {
    unsigned int num;
    test_type_t type;
    const char *data;
    int allocate;
    const char *description;
    unsigned int num_conditions;
    test_condition_t *conditions;
} test_t;

static void
test_init(test_t *test, unsigned int num, test_type_t type, int allocate, const char *description, const char *data) {
    memset(test, 0, sizeof(*test));

    test->num = num;
    test->type = type;
    test->allocate = allocate;
    test->description = description;
    test->data = data;
}

static void
test_free(test_t *test) {
    if (test->conditions != NULL)
        free(test->conditions);
}

static void
test_add_condition(test_t *test, unsigned int row, unsigned int col, const char *value) {
    test->conditions = realloc(test->conditions, sizeof(test_condition_t) * (test->num_conditions + 1));
    test->conditions[test->num_conditions].row = row;
    test->conditions[test->num_conditions].col = col;
    test->conditions[test->num_conditions].value = value;

    ++test->num_conditions;
}

int
test_perform(test_t *test) {
    int success = 1;
    unsigned int i, row = 1;
    const char *value;
    csv_t *csv;
    csv_read_t ret;

    printf("Test %u: %s\n", test->num, test->description);
    printf("  Type: %s\n", test->type == TEST_TYPE_FILE ? "File" : "String");
    printf("  Allocate: %s\n", test->allocate ? "Yes" : "No");

    csv = csv_init();

    if (test->type == TEST_TYPE_FILE) {
        if (!csv_open_file(csv, test->data, test->allocate)) {
            printf("    Fail: %s\n", csv_error(csv));
            success = 0;
        }
    }
    else {
        if (!csv_open_str(csv, test->data, test->allocate)) {
            printf("    Fail: %s\n", csv_error(csv));
            success = 0;
        }
    }

    if (success) {
        while (1) {
            ret = csv_read(csv);
            if (ret == CSV_READ_ERROR) {
                printf("    Fail: %s\n", csv_error(csv));
                break;
            }
            if (ret == CSV_READ_EOF) {
                break;
            }

            for (i = 0; i < test->num_conditions; i++) {
                if (test->conditions[i].row == row) {
                    value = csv_get(csv, test->conditions[i].col);

                    if (strcmp(value, test->conditions[i].value) == 0) {
                        printf("    %u: Pass\n", i + 1);
                    }
                    else {
                        printf("    %u: Fail: '%s' != '%s'\n", i + 1, value, test->conditions[i].value);
                        success = 0;
                    }
                }
            }

            ++row;
        }
    }

    csv_free(csv);

    if (success)
        printf("  Success!\n");

    return success;
}

int
main(int argc, char **argv) {
    test_t test1, test2, test3;
    static const char * csv1 = "First,Last,Age,Sex\n"
                               "John,Smith,55,Male\n"
                               "Jane,Doe,43,Female";

    test_init(&test1, 1, TEST_TYPE_STR, 0, "Test basic value retrieval", csv1);
    test_add_condition(&test1, 1, 0, "John");
    test_add_condition(&test1, 1, 1, "Smith");
    test_add_condition(&test1, 1, 2, "55");
    test_add_condition(&test1, 1, 3, "Male");
    test_add_condition(&test1, 2, 0, "Jane");
    test_add_condition(&test1, 3, 1, "Doe");
    test_add_condition(&test1, 4, 2, "43");
    test_add_condition(&test1, 5, 3, "Female");

    test_init(&test2, 2, TEST_TYPE_STR, 1, "Test basic value retrieval", csv1);
    test_add_condition(&test2, 1, 0, "John");
    test_add_condition(&test2, 1, 1, "Smith");
    test_add_condition(&test2, 1, 2, "55");
    test_add_condition(&test2, 1, 3, "Male");
    test_add_condition(&test2, 2, 0, "Jane");
    test_add_condition(&test2, 3, 1, "Doe");
    test_add_condition(&test2, 4, 2, "43");
    test_add_condition(&test2, 5, 3, "Female");

    test_init(&test3, 3, TEST_TYPE_STR, 0, "Quote test",
        "First,Last,Address\n"
        "\"John \"\"The Generic\"\"\",Smith,125 Basic Street\n"
        "Jane,\"Doe\",\"592 5th street, SW\"");
    test_add_condition(&test3, 1, 0, "John \"The Generic\"");
    test_add_condition(&test3, 1, 1, "Smith");
    test_add_condition(&test3, 1, 2, "125 Basic Street");
    test_add_condition(&test3, 2, 0, "Jane");
    test_add_condition(&test3, 2, 1, "Doe");
    test_add_condition(&test3, 2, 2, "592 5th street, SW");

    test_perform(&test1);
    test_perform(&test2);
    test_perform(&test3);

    test_free(&test1);
    test_free(&test2);
    test_free(&test3);

    return 0;
}
