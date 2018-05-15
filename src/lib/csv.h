#pragma once

/**
 * @file csv.h
 * @author Scott Newman <scott.newman50@gmail.com>
 * @date Sun May 13, 2017
 *
 * @brief A simple and quick CSV parser.
 *
 * I wrote this CSV parser because I was having trouble finding a very simple,
 * efficient, and quick library to parse CSV documents in C. OK, I didn't look
 * too hard, but thought it might be fun to write my own.
 *
 * I had a few design goals in mind:
 * - Simple. I wanted a library that was very simple to use and very easy to
 *   incorporate into projects.
 * - Quick: I wanted a library that was very fast and could zip through large
 *   documents in a breeze.
 * - Small: I wanted a library that had a low memory footprint and could
 *   handle files of any size.
 *
 * The library can read from a file or from a string in memory. Each method
 * also allows the library to stream the document or make a copy of it in
 * memory. Each method has it own advantages and use cases.
 *
 * @section read_from_file Reading From a File
 * When reading from a file, you would typically stream it if the system does
 * not have a lot of memory available to it or if the CSV file is very large.
 * The library will initially allocate a buffer of 1024 bytes and will try to
 * reuse that buffer over and over again. If a line is not found within 1024
 * bytes, then the buffer will be expanded by 1024 bytes until a line is found.
 * This buffer will not be deallocated until csv_close() is called. This amount
 * can be changed by calling csv_set_read_size(). This usually isn't the
 * quickest method because the library needs to make sure an entire line exists
 * in the buffer before parsing it. However, this method scales very well and
 * should let you read a file of any size.
 *
 * @section read_from_string Reading From a String
 * When reading from a string, the library acts exactly the same whether or not
 * a copy of the string is made except for the extra strdup(). No changes to
 * string are ever made, so allocation should be used when the variable that
 * has the string will go out of scope.
 *
 * @section parsing_fields Parsing and Fields
 * When the library parses a line, it will only allocate memory on an as need
 * basis for the fields. The buffers for each field will be re-used unless
 * more room is needed. This greatly reduces the number of mallocs() needed and
 * the memory the fields is free()'d when csv_close() is called. When copying
 * fields, if no escaped fields are detected, then memcpy() will be used to try
 * and take advantage of any opitimizations offered by the compiler. If an
 * escaped field is detected, then a character by character copy has to be done
 * so the escape characters are not copied.
 *
 * @section usage Basic Usage
 * @code
 * #include <stdlib.h>
 * #include <stdio.h>
 * #include <csv.h>
 *
 * int main(int argc, char **argv) {
 *    csv_t *csv;
 *    csv_read_t ret;
 *
 *    csv = csv_init();
 *    if (csv == NULL) {
 *       printf("Out of memory\n");
 *       exit(1);
 *    }
 *
 *    if (!csv_open_file(csv, "/tmp/myfile.csv", 0)) {
 *       printf("Error opening file: %s\n", csv_error(csv));
 *    }
 *    else {
 *       while (1) {
 *          ret = csv_read(csv);
 *
 *          if (ret == CSV_READ_ERROR) {
 *             printf("Read error: %s\n", csv_error(csv));
 *             break;
 *          }
 *
 *          if (ret == CSV_READ_EOF) {
 *             break;
 *          }
 *
 *          printf("Field 0 is: %s\n", csv_get(csv, 0));
 *       }
 *
 *       csv_close(csv);
 *    }
 *
 *    csv_free(csv);
 *    return 0;
 * }
 * @endcode
 */

/**
 * The possible return values for csv_read().
 */
typedef enum {
    CSV_READ_OK,    /**< The read was successful. */
    CSV_READ_EOF,   /**< No more lines are available. */
    CSV_READ_ERROR  /**< An error occured. */
} csv_read_t;

/**
 *  Forward declartion typedef. The header file doesn't need to know what data
 *  members are in the CSV handle.
 */
typedef struct csv_t csv_t;

/**
 * @brief Allocates and initializes a CSV handle.
 *
 * Allocates memory for a CSV handles and initializes it for use. This function
 * must be called before any other function.
 * 
 * @return A pointer to the CSV handle, or <tt>NULL</tt> if not enough memory
 * was available.
 */
csv_t * csv_init();

/**
 * @brief Dellocates the memory used by the CSV handle.
 *
 * Deallocates a CSV handle previously allocated with csv_init(). This function
 * should be the last function called when you no longer need the CSV handle.
 *
 * @param[in] csv A CSV handle.
 */
void csv_free(csv_t *csv);

/**
 * @brief Sets the read size used by the CSV handle.
 *
 * Use this to tell the library how much data to read at a time when parsing
 * a file in streaming mode. Depending on how long the lines are in the CSV
 * document, different values may offer different performance benefits (or
 * penalties!). The default value is 1024.
 *
 * @param[in] csv A CSV handle.
 * @param[in] read_size The size of the read buffer.
 */
void csv_set_read_size(csv_t *csv, unsigned int read_size);

/**
 * @brief Sets whether or not the library should expect a header.
 *
 * Use this to tell the library whether or not the CSV document has a header.
 * This must be called before csv_open_file() or csv_open_string() is called.
 * By default, the library will look for a header.
 *
 * @param[in] csv A CSV handle.
 * @param[in] value 1 if the CSV document has a header, or 0 if not.
 */
void csv_set_header(csv_t *csv, int value);

/**
 * @brief Sets whether or not the library should left trim field values.
 *
 * Use this to tell the library whether or not CSV field values should be
 * left trimmed. This must be called before csv_open_file() or
 * csv_open_string(). By default, the library will not left trim values.
 *
 * @param[in] csv A CSV handle.
 * @param[in] value 1 if fields should be left trimmed, or 0 if not.
 */
void csv_set_left_trim(csv_t *csv, int value);

/**
 * @brief Sets whether or not the library should left right field values.
 *
 * Use this to tell the library whether or not CSV field values should be
 * right trimmed. This must be called before csv_open_file() or
 * csv_open_string(). By default, the library will not right trim values.
 *
 * @param[in] csv A CSV handle.
 * @param[in] value 1 if fields should be right trimmed, or 0 if not.
 */
void csv_set_right_trim(csv_t *csv, int value);

/**
 * @brief Sets whether or not the library should trim field values.
 *
 * Use this to tell the library wehther or not CSV field values should be
 * left and right trimmed. This must be called before csv_open_file() or
 * csv_open_string(). By default, the library will not trim values. Calling
 * this function is the same as calling csv_set_left_trim() and
 * csv_set_right_trim().
 *
 * @param[in] csv A CSV handle.
 * @param[in] value 1 if fields should be trimmed, or 0 if not.
 */
void csv_set_trim(csv_t *csv, int value);

/**
 * @brief Returns the error message from a previously failed operation.
 *
 * Use this function to get the error message after another csv function
 * failed. When an error occurs, an error message is stored into a buffer in
 * the CSV handle and is not cleared until csv_close() is called.
 *
 * @param[in] csv A CSV handle.
 * @return The error message.
 */
const char * csv_error(csv_t *csv);

/**
 * @brief Initializes the library with a file to parse from.
 *
 * Opens a file located at <tt>path</tt> to begin parsing from. The
 * <tt>allocate</tt> flag tells the library how to read from the file.
 *
 * If <tt>allocate</tt> is 0, then the file will be opened and the file
 * descriptor will remain open until csv_close() is called. This will put
 * the library into streaming mode and will only read from the file when more
 * data is needed. The library will read CSV_READ_SIZE bytes at a time and will
 * try to reuse the same buffer unless more room is needed, in which case a
 * another malloc() will be called. This method is suggested when reading very
 * large files or when memory is sparse.
 *
 * If <tt>allocate</tt> is 1, then the file will be opened, the entire contents
 * read into memory, and the file descriptor immediately closed. This method is
 * usually faster and suggested if enough memory is available.
 *
 * @param[in] csv A CSV handle.
 * @param[in] path A path to the CSV file.
 * @param[in] allocate 1 or enable allocation, 0 to disable.
 * @return 1 on success, 0 on failure.
 */
int csv_open_file(csv_t *csv, const char *path, int allocate);

/**
 * @brief Initialize the library with a string to parse from.
 *
 * Initializes the library with <tt>str</tt> to begin parsing from. The
 * <tt>allocate</tt> falgs tells the library whether a copy of <tt>str</tt>
 * should be made or not.
 *
 * If <tt>allocate</tt> is 0, then the library will not make a copy of
 * <tt>str</tt>. This means <tt>str</tt> must stay in scope until parsing is
 * finished.
 *
 * If <tt>allocate</tt> is 1, then <tt>str</tt> is duplicated using strdup().
 * The memory will be free()'ed once csv_close() is called.
 *
 * @param[in] csv A CSV handle.
 * @param[in] str The string to parse.
 * @param[in] allocate 1 to enable allocation, 0 to disable.
 * @return 1 on success, 0 on failure.
 */
int csv_open_str(csv_t *csv, const char *str, int allocate);

/**
 * @brief Closes the library and stops parsing.
 *
 * Frees all the memory and closes any file descriptors being used by the CSV
 * handle. After calling csv_close(), you may safely call csv_open() again.
 * This does not free the memory allocated for the CSV structure itself. Call
 * csv_free() for that.
 *
 * @param[in] csv A CSV handle.
 */
void csv_close(csv_t *csv);

/**
 * @brief Read the next line in the CSV document.
 *
 * Reads the next line in the CSV document and stores the values for retrieval.
 * If the CSV handle is reading from a file and in streaming mode, then the
 * library will read from the file if a new line is not found.
 *
 * @param[in] csv A CSV handle.
 * @return CSV_READ_OK if a line was successfully read, CSV_READ_EOF if there
 * are no more lines, or CSV_READ_ERROR is an error occured.
 */
csv_read_t csv_read(csv_t *csv);

/**
 * @brief Get a CSV field.
 *
 * Retreives a field from a previously read line. If the field is blank or the
 * index is greater than the number of a fields, NULL will be returned.
 *
 * @param[in] csv A CSV handle.
 * @param[in] index The index of the field.
 * @return The field value or NULL.
 */
const char * csv_get(csv_t *csv, unsigned int index);
