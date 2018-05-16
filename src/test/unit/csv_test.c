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
    int success;
} test_condition_t;

typedef struct {
    unsigned int num;
    test_type_t type;
    const char *data;
    int allocate;
    int header;
    unsigned int read_size;
    int left_trim;
    int right_trim;
    const char *description;
    unsigned int num_conditions;
    test_condition_t *conditions;
} test_t;

static void
test_init(test_t *test, unsigned int num, test_type_t type, int allocate, int header, unsigned int read_size, int left_trim, int right_trim, const char *description, const char *data) {
    memset(test, 0, sizeof(*test));

    test->num = num;
    test->type = type;
    test->allocate = allocate;
    test->header = header;
    test->read_size = read_size;
    test->left_trim = left_trim;
    test->right_trim = right_trim;
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
    test->conditions[test->num_conditions].success = 0;

    ++test->num_conditions;
}

int
test_perform(test_t *test) {
    int success = 1;
    unsigned int i, row = 1;
    const char *value;
    csv_t *csv;
    csv_read_t ret;

    printf("Test %u: %s (", test->num, test->description);
    printf("Type: %s, ", test->type == TEST_TYPE_FILE ? "File" : "String");
    printf("Allocate: %s", test->allocate ? "Yes" : "No");
    if (test->type == TEST_TYPE_FILE && !test->allocate)
        printf(", Read Size: %u", test->read_size);
    printf(")\n");

    csv = csv_init();

    if (!test->header) {
        csv_set_header(csv, 0);
    }
    if (test->left_trim) {
        csv_set_left_trim(csv, 1);
    }
    if (test->right_trim) {
        csv_set_right_trim(csv, 1);
    }

    if (test->type == TEST_TYPE_FILE) {
        if (!test->allocate) {
            csv_set_read_size(csv, test->read_size);
        }

        if (!csv_open_file(csv, test->data, test->allocate)) {
            printf("  \033[1;31mFail: %s\033[0m\n", csv_error(csv));
            success = 0;
        }
    }
    else {
        if (!csv_open_str(csv, test->data, test->allocate)) {
            printf("  \033[1;31mFail: %s\033[0m\n", csv_error(csv));
            success = 0;
        }
    }

    if (success) {
        while (1) {
            ret = csv_read(csv);
            if (ret == CSV_READ_ERROR) {
                printf("    \033[1;31mFail: %s\033[0m\n", csv_error(csv));
                success = 0;
                break;
            }
            if (ret == CSV_READ_EOF) {
                break;
            }

            for (i = 0; i < test->num_conditions; i++) {
                if (test->conditions[i].row == row) {
                    value = csv_get(csv, test->conditions[i].col);

                    if (strcmp(value, test->conditions[i].value) == 0) {
                        test->conditions[i].success = 1;
                    }
                    else {
                        printf("    %u: \033[1;31mFail: '%s' != '%s'\033[0m\n", i + 1, value, test->conditions[i].value);
                        success = 0;
                    }
                }
            }

            ++row;
        }
    }

    csv_free(csv);

    if (success)
        printf(" \033[1;32mAll tests passed!\033[0m\n");

    return success;
}

int
main(int argc, char **argv) {
    test_t test1, test2, test3, test4, test5, test6, test7, test8, test9, test10;
    static const char * csv1 = "First,Last,Age,Sex\n"
                               "John,Smith,55,Male\n"
                               "Jane,Doe,43,Female";

    test_init(&test1, 1, TEST_TYPE_STR, 0, 1, 1024, 0, 0, "Test basic value retrieval", csv1);
    test_add_condition(&test1, 1, 0, "John");
    test_add_condition(&test1, 1, 1, "Smith");
    test_add_condition(&test1, 1, 2, "55");
    test_add_condition(&test1, 1, 3, "Male");
    test_add_condition(&test1, 2, 0, "Jane");
    test_add_condition(&test1, 2, 1, "Doe");
    test_add_condition(&test1, 2, 2, "43");
    test_add_condition(&test1, 2, 3, "Female");

    test_init(&test2, 2, TEST_TYPE_STR, 1, 1, 1024, 0, 0, "Test basic value retrieval", csv1);
    test_add_condition(&test2, 1, 0, "John");
    test_add_condition(&test2, 1, 1, "Smith");
    test_add_condition(&test2, 1, 2, "55");
    test_add_condition(&test2, 1, 3, "Male");
    test_add_condition(&test2, 2, 0, "Jane");
    test_add_condition(&test2, 2, 1, "Doe");
    test_add_condition(&test2, 2, 2, "43");
    test_add_condition(&test2, 2, 3, "Female");

    test_init(&test3, 3, TEST_TYPE_STR, 0, 1, 1024, 0, 0, "Quotes and escaping",
        "First,Last,Address\n"
        "\"John \"\"The Generic\"\"\",Smith,125 Basic Street\n"
        "Jane,\"Doe\",\"592 5th street, SW\"");
    test_add_condition(&test3, 1, 0, "John \"The Generic\"");
    test_add_condition(&test3, 1, 1, "Smith");
    test_add_condition(&test3, 1, 2, "125 Basic Street");
    test_add_condition(&test3, 2, 0, "Jane");
    test_add_condition(&test3, 2, 1, "Doe");
    test_add_condition(&test3, 2, 2, "592 5th street, SW");

    test_init(&test4, 4, TEST_TYPE_STR, 0, 1, 1024, 0, 0, "Preserve spaces",
        "First,Last,Address\n"
        " John ,    Smith,125 Basic Street  \n"
        "Jane   , Doe , 592 5th Street");
    test_add_condition(&test4, 1, 0, " John ");
    test_add_condition(&test4, 1, 1, "    Smith");
    test_add_condition(&test4, 1, 2, "125 Basic Street  ");
    test_add_condition(&test4, 2, 0, "Jane   ");
    test_add_condition(&test4, 2, 1, " Doe ");
    test_add_condition(&test4, 2, 2, " 592 5th Street");

    test_init(&test5, 5, TEST_TYPE_STR, 0, 0, 1024, 0, 0, "No header",
        "John,Smith,125 Basic Street\n"
        "Jane,Doe,592 5th Street");
    test_add_condition(&test5, 1, 0, "John");
    test_add_condition(&test5, 1, 1, "Smith");
    test_add_condition(&test5, 1, 2, "125 Basic Street");
    test_add_condition(&test5, 2, 0, "Jane");
    test_add_condition(&test5, 2, 1, "Doe");
    test_add_condition(&test5, 2, 2, "592 5th Street");

    test_init(&test6, 6, TEST_TYPE_STR, 0, 1, 1024, 1, 1, "Triming", 
        "First,Last,Address\n"
        "  John  ,  Smith,125 Basic Street  \n"
        "Jane  ,Doe,592 5th Street");
    test_add_condition(&test6, 1, 0, "John");
    test_add_condition(&test6, 1, 1, "Smith");
    test_add_condition(&test6, 1, 2, "125 Basic Street");
    test_add_condition(&test6, 2, 0, "Jane");
    test_add_condition(&test6, 2, 1, "Doe");
    test_add_condition(&test6, 2, 2, "592 5th Street");

    test_init(&test7, 7, TEST_TYPE_FILE, 0, 1, 1024, 0, 0, "File", "sample.csv");
    test_add_condition(&test7, 1, 0, "John"); 
    test_add_condition(&test7, 1, 1, "Smith"); 
    test_add_condition(&test7, 1, 2, "125 Basic Street"); 
    test_add_condition(&test7, 2, 0, "Jane"); 
    test_add_condition(&test7, 2, 1, "Doe"); 
    test_add_condition(&test7, 2, 2, "127 5th, Street"); 

    test_init(&test8, 8, TEST_TYPE_FILE, 1, 1, 1024, 0, 0, "File", "sample.csv");
    test_add_condition(&test8, 1, 0, "John"); 
    test_add_condition(&test8, 1, 1, "Smith"); 
    test_add_condition(&test8, 1, 2, "125 Basic Street"); 
    test_add_condition(&test8, 2, 0, "Jane"); 
    test_add_condition(&test8, 2, 1, "Doe"); 
    test_add_condition(&test8, 2, 2, "127 5th, Street"); 

    test_init(&test9, 9, TEST_TYPE_FILE, 0, 1, 3, 0, 0, "File: Small Buffer", "sample.csv");
    test_add_condition(&test9, 1, 0, "John"); 
    test_add_condition(&test9, 1, 1, "Smith"); 
    test_add_condition(&test9, 1, 2, "125 Basic Street"); 
    test_add_condition(&test9, 2, 0, "Jane"); 
    test_add_condition(&test9, 2, 1, "Doe"); 
    test_add_condition(&test9, 2, 2, "127 5th, Street"); 

    test_init(&test10, 10, TEST_TYPE_STR, 0, 1, 1024, 0, 0, "Miscellaneous",
        "First,Last,Address\n"
        "\n\n\n"
        "\"John\",\"Smith\"  , \"125 Basic Street\"\n"
        "Jane,Doe,592 5th Street\n\n");
    test_add_condition(&test10, 1, 0, "John");
    test_add_condition(&test10, 1, 1, "Smith");
    test_add_condition(&test10, 1, 2, "125 Basic Street");
    test_add_condition(&test10, 2, 0, "Jane");
    test_add_condition(&test10, 2, 1, "Doe");
    test_add_condition(&test10, 2, 2, "592 5th Street");

    test_perform(&test1);
    test_perform(&test2);
    test_perform(&test3);
    test_perform(&test4);
    test_perform(&test5);
    test_perform(&test6);
    test_perform(&test7);
    test_perform(&test8);
    test_perform(&test9);
    test_perform(&test10);

    test_free(&test1);
    test_free(&test2);
    test_free(&test3);
    test_free(&test4);
    test_free(&test5);
    test_free(&test8);
    test_free(&test9);
    test_free(&test10);

    return 0;
}
