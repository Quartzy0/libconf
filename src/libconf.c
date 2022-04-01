#include "../include/libconf.h"

#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <glob.h>

#ifndef _WIN32

#include <sys/stat.h>

#else
#include <Windows.h>
#endif

//utility functions

char *strchrnul_(char *s, char c) {
    if (*s == '\0' || *(s - 1) == '\0') return NULL;
    char *val = strchr(s, c);
    return !val ? s + strlen(s) - 1 : val;
}

//strrchr but the end pointer and the length are provided instead of the start pointer
//searches backwards to find the last occurrence
char *strrchr_(const char *end, size_t len, char c) {
    for (size_t i = 0; i < len; ++i) {
        if (*(end - i) == c) return (char *) end - i;
    }
    return NULL;
}

//https://stackoverflow.com/a/146938/
bool isFile(const char *path) {
    struct stat s;
    int status;
    if ((status = stat(path, &s)) == 0) {
        return s.st_mode & S_IFREG;
    } else {
        fprintf(stderr, "Error: error when calling stat() function: %d", status);
        return false;
    }
}

size_t trimn(const char *in, size_t n, char **out) {
    if (!in) return 0;
    const char *begin = in;
    const char *end = in + n - 1;
    if (begin == end) {
        if (isspace(*in)) return 0;
        *out = strdup(in);
        return strlen(in);
    }

    while (isspace(*begin))
        begin++;
    while (isspace(*end))
        end--;
    end++;
    *out = strndup(begin, end - begin);
    return end - begin;
}

void trimnp(char *in, const char *end, char **out) {
    if (!in) return;
    char *begin = in;

    while (isspace(*begin) && begin < end)
        begin++;
    *out = begin;
}

void parseConfigWhole(Option **options, const char *file, char *bufferOriginal, size_t length);

char *parseArray(ArrayOption *array, const char *file, const char *optName, const char *buffer, const char *bufferEnd) {
    if (!array)return NULL;
    char *arrayStart = strchr(buffer, '[');
    if (!arrayStart || arrayStart > bufferEnd) {
        fprintf(stderr,
                "Error at option %s:%s: Array must start on the same line as the option definition with [\n",
                file, optName);
        return NULL;
    }

    switch (array->type) {
        case BOOL: {
            char *arrayEnd = strchr(arrayStart + 1, ']');
            if (!arrayEnd) {
                fprintf(stderr, "Error at option %s:%s: Array must end with ]\n", file, optName);
                return NULL;
            }

            bool *arr = malloc(sizeof(bool) * ARRAY_ALLOCATION);
            size_t i = 0;
            size_t arraySize = ARRAY_ALLOCATION;
            char *nextElement = arrayStart + 1;
            char *currentElement = arrayStart + 1;
            do {
                nextElement = strchr(nextElement + 1, ',');
                if (nextElement > arrayEnd || !nextElement) nextElement = arrayEnd;
                char *valueStart;
                trimnp(currentElement, nextElement, &valueStart);
                if (arraySize - 2 == i) {
                    bool *tmp = realloc(arr, (arraySize + ARRAY_ALLOCATION) * sizeof(bool));
                    if (!tmp) {
                        fprintf(stderr, "Error while reallocating memory for bool array: %s\n", strerror(errno));
                        goto clean_b;
                    }
                    arr = tmp;
                }
                arr[i] = !strncasecmp(valueStart, "true", 4) || !strncasecmp(valueStart, "yes", 3);
                if (!arr[i] && !(!strncasecmp(valueStart, "false", 5) ||
                                 !strncasecmp(valueStart, "no", 2))) { // If option is not true or false
                    fprintf(stderr, "Error at option %s:%s[%zu]: Invalid boolean. Must be true, false, yes or no\n",
                            file, optName, i);
                    goto clean_b;
                }
                i++;
                currentElement = nextElement + 1;
            } while (nextElement != arrayEnd);

            if (i != arraySize) {
                bool *tmp = realloc(arr, i * sizeof(bool));
                if (!tmp) {
                    fprintf(stderr, "Error while reallocating memory for bool array: %s\n", strerror(errno));
                    goto clean_b;
                }
                arr = tmp;
            }
            array->a_b = arr;
            array->len = i;
            return arrayEnd + 1;
            clean_b:
            free(arr);
            return arrayEnd + 1;
        }
        case LONG: {
            char *arrayEnd = strchr(arrayStart + 1, ']');
            if (!arrayEnd) {
                fprintf(stderr, "Error at option %s:%s: Array must end with ]\n", file, optName);
                return NULL;
            }

            long *arr = malloc(sizeof(long) * ARRAY_ALLOCATION);
            size_t i = 0;
            size_t arraySize = ARRAY_ALLOCATION;
            char *nextElement = arrayStart + 1;
            char *currentElement = arrayStart + 1;
            do {
                nextElement = strchr(nextElement + 1, ',');
                if (nextElement > arrayEnd || !nextElement) nextElement = arrayEnd;
                char *valueStart;
                trimnp(currentElement, nextElement, &valueStart);
                if (arraySize - 2 == i) {
                    long *tmp = realloc(arr, (arraySize + ARRAY_ALLOCATION) * sizeof(long));
                    if (!tmp) {
                        fprintf(stderr, "Error while reallocating memory for long array: %s\n", strerror(errno));
                        goto clean_l;
                    }
                    arr = tmp;
                }
                char *endPtr = NULL;
                arr[i] = strtol(valueStart, &endPtr, 10);
                if (endPtr == valueStart) {
                    //error
                    fprintf(stderr, "Error at option %s:%s[%zu]: Invalid long: %s\n", file, optName, i,
                            strerror(errno));
                }
                i++;
                currentElement = nextElement + 1;
            } while (nextElement != arrayEnd);

            if (i != arraySize) {
                long *tmp = realloc(arr, i * sizeof(long));
                if (!tmp) {
                    fprintf(stderr, "Error while reallocating memory for long array: %s\n", strerror(errno));
                    goto clean_l;
                }
                arr = tmp;
            }
            array->a_l = arr;
            array->len = i;
            return arrayEnd + 1;
            clean_l:
            free(arr);
            return arrayEnd + 1;
        }
        case DOUBLE: {
            char *arrayEnd = strchr(arrayStart + 1, ']');
            if (!arrayEnd) {
                fprintf(stderr, "Error at option %s:%s: Array must end with ]\n", file, optName);
                return NULL;
            }

            double *arr = malloc(sizeof(double) * ARRAY_ALLOCATION);
            size_t i = 0;
            size_t arraySize = ARRAY_ALLOCATION;
            char *nextElement = arrayStart + 1;
            char *currentElement = arrayStart + 1;
            do {
                nextElement = strchr(nextElement + 1, ',');
                if (nextElement > arrayEnd || !nextElement) nextElement = arrayEnd;
                char *valueStart;
                trimnp(currentElement, nextElement, &valueStart);
                if (arraySize - 2 == i) {
                    double *tmp = realloc(arr, (arraySize + ARRAY_ALLOCATION) * sizeof(double));
                    if (!tmp) {
                        fprintf(stderr, "Error while reallocating memory for double array: %s\n", strerror(errno));
                        goto clean_d;
                    }
                    arr = tmp;
                }
                char *endPtr = NULL;
                arr[i] = strtod(valueStart, &endPtr);
                if (endPtr == valueStart) {
                    //error
                    fprintf(stderr, "Error at option %s:%s[%zu]: Invalid double: %s\n", file, optName, i,
                            strerror(errno));
                }
                i++;
                currentElement = nextElement + 1;
            } while (nextElement != arrayEnd);

            if (i != arraySize) {
                double *tmp = realloc(arr, i * sizeof(double));
                if (!tmp) {
                    fprintf(stderr, "Error while reallocating memory for double array: %s\n", strerror(errno));
                    goto clean_d;
                }
                arr = tmp;
            }
            array->a_d = arr;
            array->len = i;
            return arrayEnd + 1;
            clean_d:
            free(arr);
            return arrayEnd + 1;
        }
        case TEXT: {
            char **arr = malloc(sizeof(char *) * ARRAY_ALLOCATION);
            size_t i = 0;
            size_t arraySize = ARRAY_ALLOCATION;
            char *prevElement = arrayStart + 1;
            char *currentElement = arrayStart + 1;
            do {
                if (arraySize - 2 == i) {
                    char **tmp = realloc(arr, (arraySize + ARRAY_ALLOCATION) * sizeof(char *));
                    if (!tmp) {
                        fprintf(stderr, "Error while reallocating memory for string array: %s\n", strerror(errno));
                        goto clean_s;
                    }
                    arr = tmp;
                }

                char *doubleStart = strchr(currentElement, '"');
                char *singleStart = strchr(currentElement, '\'');
                char *possibleArrayEnd = strchr(prevElement, ']');
                if (!possibleArrayEnd) {
                    fprintf(stderr, "Error at option %s:%s: Array must end with ]\n", file, optName);
                    goto clean_s;
                }
                if ((!doubleStart || possibleArrayEnd < doubleStart) &&
                    (!singleStart || possibleArrayEnd < singleStart)) {
                    break;
                }
                bool single = (singleStart < doubleStart && singleStart) || (singleStart && !doubleStart);
                if ((!single && !doubleStart)) {
                    fprintf(stderr,
                            "Error at option %s:%s[%zu]: String must start with ' or \"\n",
                            file, optName, i);
                    break;
                }
                char searchChar = single ? '\'' : '"';
                char *stringStart = single ? singleStart : doubleStart;
                stringStart += 1 + (*(stringStart + 1) == '\n');

                char *multiLineEnd = strchr(stringStart, searchChar);
                if (!multiLineEnd) {
                    fprintf(stderr, "Error at option %s:%s: String must end with ' or \"\n", file, optName);
                    break;
                }

                if (*(multiLineEnd - 1) == '\n') multiLineEnd--;

                arr[i] = strndup(stringStart, multiLineEnd - stringStart);

                prevElement = currentElement;
                currentElement = strchr(multiLineEnd, ',') + 1;
                if (!currentElement)break;
                i++;
            } while (true);

            if (i != arraySize) {
                char **tmp = realloc(arr, i * sizeof(char *));
                if (!tmp) {
                    fprintf(stderr, "Error while reallocating memory for string array: %s\n", strerror(errno));
                    goto clean_s;
                }
                arr = tmp;
            }
            array->a_s = arr;
            array->len = i;
            return prevElement;
            clean_s:
            free(arr);
            return prevElement;
        }
        case COMPOUND: {
            Option ***arr = malloc(sizeof(Option **) * ARRAY_ALLOCATION);
            size_t i = 0;
            size_t arraySize = ARRAY_ALLOCATION;
            char *currentElement = arrayStart + 1;
            do {
                if (arraySize - 2 == i) {
                    Option ***tmp = realloc(arr, (arraySize + ARRAY_ALLOCATION) * sizeof(Option **));
                    if (!tmp) {
                        fprintf(stderr, "Error while reallocating memory for compound array: %s\n",
                                strerror(errno));
                        goto clean_c;
                    }
                    arr = tmp;
                }

                char *compoundStart = strchr(currentElement, '{');
                char *potentialArrayEnd = strchr(currentElement, ']');
                if (!compoundStart || compoundStart > potentialArrayEnd) {
                    break;
                }
                int openCount = 1;
                char *compoundEnd = NULL;
                for (char *j = compoundStart + 1; j < bufferEnd; ++j) {
                    if (*j == '{') openCount++;
                    if (*j == '}') openCount--;
                    if (!openCount) {
                        compoundEnd = j;
                        break;
                    }
                }
                if (openCount) {
                    fprintf(stderr, "Error at option %s:%s: Compound must end with '}'\n", file, optName);
                    goto clean_c;
                }

                arr[i] = malloc(sizeof(Option *) * OPTION_HASHMAP_SIZE);
                /*
                  Copies all options from the template to each element of the array. It's quite ugly
                  but seems to work. It should probably be checked over.
                  It's ugly but better than looping over the entire hashmap using HASH_ITER, then copying
                  every option, and re-adding it using HASH_ADD. This way the next element in the hashmap
                  linked list bucket can also be efficiently copied at the same time.
                 */
                for (Option **tmp = array->a_v.a_v_t; tmp - (array->a_v.a_v_t) <
                                                      OPTION_HASHMAP_SIZE; ++tmp) {
                    Option *o1 = *tmp;
                    if (!o1) continue;
                    Option *o1n = malloc(sizeof(*o1));
                    memcpy(o1n, o1, sizeof(*o1));
                    arr[i][tmp - array->a_v.a_v_t] = o1n;

                    Option *opt1p = o1n;
                    for (Option *opt1 = o1n->next; opt1; opt1 = opt1->next) {
                        Option *o1nnext = malloc(sizeof(*opt1));
                        memcpy(o1nnext, opt1, sizeof(*opt1));
                        opt1p->next = o1nnext;
                        opt1p = opt1;
                    }
                }
                parseConfigWhole(arr[i], file, compoundStart + 1, compoundEnd - compoundStart + 1);

                i++;
                currentElement = compoundEnd + 1;
            } while (true);

            if (i != arraySize) {
                Option ***tmp = realloc(arr, i * sizeof(Option **));
                if (!tmp) {
                    fprintf(stderr, "Error while reallocating memory for compound array: %s\n", strerror(errno));
                    goto clean_c;
                }
                arr = tmp;
            }
            array->a_v.a_v = arr;
            array->len = i;
            return currentElement;
            clean_c:
            free(arr);
            return currentElement;
        }
        case ARRAY: {
            const char *b = strchr(arrayStart + 1, '[');
            ArrayOption *arr = malloc(ARRAY_ALLOCATION * sizeof(ArrayOption));
            size_t arrlen = ARRAY_ALLOCATION;
            size_t i = 0;
            while (b && b < bufferEnd) {
                if (i - 2 == arrlen) {
                    ArrayOption *tmp = realloc(arr, (arrlen + ARRAY_ALLOCATION) * sizeof(ArrayOption));
                    arrlen += ARRAY_ALLOCATION;
                    if (!tmp) {
                        fprintf(stderr, "Error while reallocating memory for array array: %s\n", strerror(errno));
                        free(arr);
                        return NULL;
                    }
                    arr = tmp;
                }
                arr[i].type = array->a_a.t->type;
                b = parseArray(&(arr[i]), file, optName, b, bufferEnd);
                if (!b) {
                    free(arr);
                    return NULL;
                }
                char *next = strchr(b, '[');
                char *possibleEnd = strchr(b, ']');
                if (possibleEnd < next) { //end of array reached
                    if (i + 1 != arrlen) {
                        ArrayOption *tmp = realloc(arr, (i + 1) * sizeof(ArrayOption));
                        if (!tmp) {
                            fprintf(stderr, "Error while reallocating memory for array array: %s\n", strerror(errno));
                            free(arr);
                            return NULL;
                        }
                        arr = tmp;
                    }
                    array->a_a.a_a = arr;
                    array->len = i + 1;
                    return possibleEnd;
                }
                b = next;
                i++;
            }
            fprintf(stderr,
                    "Error at option %s:%s: Array in array must start on the same line as the option definition with [\n",
                    file, optName);
            free(arr);
            return NULL;
        }
    }
    return NULL;
}

void parseConfigWhole(Option **options, const char *file, char *bufferOriginal, size_t length) {
    char *buffer = bufferOriginal;
    char *lineEnd = buffer;

    while ((lineEnd = strchrnul_(lineEnd + 1, '\n')) && lineEnd - bufferOriginal < length) {
        char *lineBufTrim = NULL;
        trimnp(buffer, lineEnd, &lineBufTrim);
        if (!lineBufTrim) { //trim returns length of trimmed string. If the new string is 0, continue to the next line
            buffer = lineEnd + 1;
            continue;
        }

        char *assignIndex = NULL, *endValueIndex = lineEnd;
        for (char *i = lineBufTrim; i < lineEnd; ++i) {
            if (*i == '=' && !assignIndex) {
                assignIndex = i;
            }
            if (*i == '/' && *(i + 1) == '/') {
                endValueIndex = i;
                break;
            }
        }
        if (!assignIndex) { // If a '//' occurs before a '=', then assignIndex will be null
            buffer = lineEnd + 1;
            continue;
        }

        char *optName = NULL;
        trimn(lineBufTrim, assignIndex - lineBufTrim, &optName);

        struct Option *optOut = NULL;
        HASH_FIND(options, optName, optOut);
        if (!optOut) {
            fprintf(stderr, "Warning in %s: Unrecognized option '%s' found\n", file, optName);
            goto loopEnd;
        }

        switch (optOut->type) {
            case TEXT: {
                if (optOut->v_s != optOut->dv_s && optOut->v_s) free(optOut->v_s);

                char *doubleStart = strchr(assignIndex + 1, '"');
                char *singleStart = strchr(assignIndex + 1, '\'');
                bool single = (singleStart < doubleStart && singleStart) || (singleStart && !doubleStart);
                if ((!single && !doubleStart) || (single ? singleStart > endValueIndex : doubleStart > endValueIndex)) {
                    fprintf(stderr,
                            "Error at option %s:%s: String must start on the same line as the option definition with ' or \"\n",
                            file, optName);
                    break;
                }
                char searchChar = single ? '\'' : '"';
                char *stringStart = single ? singleStart : doubleStart;
                stringStart += 1 + (*(stringStart + 1) == '\n');

                char *multiLineEnd = strchr(stringStart, searchChar);
                if (!multiLineEnd) {
                    fprintf(stderr, "Error at option %s:%s: String must end with ' or \"\n", file, optName);
                    break;
                }

                buffer = strchrnul_(multiLineEnd, '\n') + 1;
                lineEnd = buffer;
                if (*(multiLineEnd - 1) == '\n') multiLineEnd--;

                optOut->v_s = strndup(stringStart, multiLineEnd - stringStart);
                if (buffer == 1 || !buffer) {
                    free(optName);
                    return;
                }
                goto loopEndR;
            }
            case LONG: {
                char *endPtr = NULL;
                char *start = NULL;
                trimnp(assignIndex + 1, endValueIndex, &start);
                long tempL = strtol(start, &endPtr, 10);
                if (endPtr == start) {
                    //error
                    fprintf(stderr, "Error at option %s:%s: Invalid long \n", file, optName);
                    optOut->v_l = optOut->dv_l;
                } else {
                    optOut->v_l = tempL;
                }
                break;
            }
            case DOUBLE: {
                char *endPtr = NULL;
                char *start = NULL;
                trimnp(assignIndex + 1, endValueIndex, &start);
                double tempD = strtod(start, &endPtr);
                if (endPtr == start) {
                    //error
                    fprintf(stderr, "Error at option %s:%s: Invalid double\n", file, optName);
                    optOut->v_d = optOut->dv_d;
                } else {
                    optOut->v_d = tempD;
                }
                break;
            }
            case BOOL: {
                char *start = NULL;
                trimnp(assignIndex + 1, endValueIndex, &start);
                if (!strncasecmp(start, "true", 4) || !strncasecmp(start, "yes", 3)) {
                    optOut->v_b = true;
                    break;
                } else if (!(!strncasecmp(start, "false", 5) || !strncasecmp(start, "no", 2))) {
                    fprintf(stderr, "Error at option %s:%s: Invalid boolean. Must be true, false, yes or no\n",
                            file, optName);
                    optOut->v_b = optOut->dv_b;
                    break;
                }
                optOut->v_b = false;
                break;
            }
            case COMPOUND: {
                char *compoundStart = strchr(assignIndex + 1, '{');
                if (!compoundStart || compoundStart > endValueIndex) {
                    fprintf(stderr, "Error at option %s:%s: Compound must start with '{'\n", file, optName);
                    optOut->v_v = optOut->dv_v;

                    free(optName);
                    return;
                }
                int openCount = 1;
                char *compoundEnd = NULL;
                for (char *i = compoundStart + 1; i < compoundStart + length; ++i) {
                    if (*i == '{') openCount++;
                    if (*i == '}') openCount--;
                    if (!openCount) {
                        compoundEnd = i;
                        break;
                    }
                }
                if (openCount) {
                    fprintf(stderr, "Error at option %s:%s: Compound must end with '}'\n", file, optName);

                    free(optName);
                    return;
                }

                parseConfigWhole(optOut->v_v, file, compoundStart + 1, compoundEnd - compoundStart + 1);
                buffer = strchrnul_(compoundEnd, '\n');
                if (!buffer) {
                    free(optName);
                    return;
                }
                lineEnd = buffer;
                break;
            }
            case ARRAY: {
                char *arrayEnd = parseArray(&(optOut->v_a), file, optName, assignIndex, bufferOriginal + length);
                if (!arrayEnd)break;
                lineEnd = arrayEnd;
                break;
            }
        }


        loopEnd:
        buffer = lineEnd + 1;
        loopEndR:
        free(optName);
    }
}

size_t readFile(const char *filename, char **bufferOut) {
    size_t length;
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "Error: Can't open file '%s': '%s'\n", filename, strerror(errno));
        return 0;
    }

    length = getFileSize(filename);
    if (length == 0) {
        fclose(fp);
        return 0;
    }

    (*bufferOut) = malloc(length + 2);
    if (!(*bufferOut)) {
        fprintf(stderr, "Error: Can't allocate memory for reading file '%s': '%s'\n", filename, strerror(errno));
        fclose(fp);
        free((*bufferOut));
        return 0;
    }
    if (!fread((*bufferOut), 1, length, fp)) {
        fprintf(stderr, "Error: Can't read file '%s': '%s'\n", filename, strerror(errno));
        fclose(fp);
        free((*bufferOut));
        return 0;
    }
    fclose(fp);

    if ((*bufferOut)[length - 1] != '\n') { //Ensure that string ends in new line (makes things nicer)
        length++;
        (*bufferOut)[length - 1] = '\n';
    }
    (*bufferOut)[length] = '\0'; //Must be null terminated
    return length;
}

size_t preprocessor(char *bufferOriginal, size_t bufferOriginalLen, char **bufferOut, char *dirPath) {
    size_t dirPathLen = strlen(dirPath);
    char *bufferO = malloc(bufferOriginalLen);
    char *buffer = bufferO; // Increment this buffer in order to keep the original for realloc and measuring size
    size_t bufferLen = bufferOriginalLen;
    char *macroStart = bufferOriginal;
    char *prevMacroEnd = bufferOriginal;
    bool macroUsed = false;

    while ((macroStart = strchr(macroStart, '#')) && macroStart - bufferOriginal < bufferOriginalLen) {
        char *lineBegin = strrchr_(macroStart, macroStart - prevMacroEnd, '\n') + 1;
        if (lineBegin == NULL + 1) { // 1 was added to lineBegin, so it will be NULL + 1 if it would usually be NULL
            lineBegin = prevMacroEnd;
        }
        for (; lineBegin < macroStart; lineBegin++) { //Macro has to be the only thing on the line
            if (!isspace(*lineBegin)) goto eol;
        }

        char *lineEnd = strchr(macroStart + 1, '\n');
        if (!(strncmp(macroStart + 1, "include", 7))) {
            char *pathStart = memchr(macroStart + 7, '"', lineEnd - macroStart);
            if (!pathStart) {
                fprintf(stderr, "Error: macro #include must have a path specified in '\"'\n");
                goto eol;
            }
            pathStart++;
            char *pathEnd = memchr(pathStart, '"', lineEnd - (pathStart));
            if (!pathEnd) {
                fprintf(stderr, "Error: macro #include must have a path specified in '\"'\n");
                goto eol;
            }
            char *fileName = malloc(((pathEnd - pathStart + dirPathLen + 1) * sizeof(char)));
            memcpy(fileName, dirPath, dirPathLen);
            memcpy(fileName + dirPathLen, pathStart, pathEnd - pathStart);
            fileName[pathEnd - pathStart + dirPathLen] = '\0'; //Must be null-terminated

            glob_t glob_result;
            memset(&glob_result, 0, sizeof(glob_result));

            int glob_return = glob(fileName, GLOB_TILDE, NULL, &glob_result);

            switch (glob_return) {
                case GLOB_NOSPACE: {
                    fprintf(stderr, "Error: glob() failed with return code GLOB_NOSPACE (Out of memory)\n");
                    globfree(&glob_result);
                    goto eol;
                }
                case GLOB_ABORTED: {
                    fprintf(stderr, "Error: glob() failed with return code GLOB_ABORTED (Read error)\n");
                    globfree(&glob_result);
                    goto eol;
                }
                case GLOB_NOMATCH: {
                    fprintf(stderr, "Error: No files found matching pattern '%s'\n", fileName);
                    globfree(&glob_result);
                    goto eol;
                }
                default: {
                    break;
                }
            }

            if (buffer - bufferO + (macroStart - prevMacroEnd) >= bufferLen) {
                char *tmp = realloc(bufferO, bufferLen + (macroStart - prevMacroEnd) + 1); // +1 because why not
                if (!tmp) {
                    fprintf(stderr, "Error: unable to reallocate more memory for macro processing buffer: %s\n",
                            strerror(errno));
                    goto eol;
                }
                bufferLen += (macroStart - prevMacroEnd) + 1;
            }

            memcpy(buffer, prevMacroEnd,
                   macroStart - prevMacroEnd); // Copy everything leading up to the macro into new buffer
            buffer += macroStart - prevMacroEnd;

            for (int i = 0; i < glob_result.gl_pathc; ++i) {
                if (!isFile(glob_result.gl_pathv[i]))continue; //Continue if it's not a file
                char *fileBuf = NULL;
                size_t len = readFile(glob_result.gl_pathv[i], &fileBuf);
                if (!len) {
                    fprintf(stderr, "Error: unable to read file: %s\n", glob_result.gl_pathv[i]);
                    continue;
                }

                //Get path to the parent directory used to find other files which may be included
                char *parentDirI = strrchr(fileName, '/') + 1;
                char *parentDir = strndup(fileName, parentDirI - fileName);

                char *fileProcessed = NULL;
                size_t len1 = preprocessor(fileBuf, len, &fileProcessed, parentDir);

                // First make sure new buffer has enough space
                if (buffer - bufferO + len >= bufferLen) {
                    char *tmp = realloc(bufferO, bufferLen + len + 1); // +1 because why not
                    if (!tmp) {
                        fprintf(stderr, "Error: unable to reallocate more memory for macro processing buffer: %s\n",
                                strerror(errno));
                        goto eol;
                    }
                    size_t offset = buffer - bufferO;
                    bufferO = tmp;
                    buffer = bufferO + offset;
                    bufferLen += len + 1;
                }


                memcpy(buffer, fileProcessed, len1); // Copy everything from included file into new buffer
                buffer += len1;
            }

            globfree(&glob_result);

            prevMacroEnd = lineEnd;
            macroUsed = true;
        }
        eol:
        macroStart++;
    }
    if (!macroUsed) {
        (*bufferOut) = bufferOriginal;
        return bufferOriginalLen;
    }

    // First make sure new buffer has enough space
    if ((buffer - bufferO) + ((bufferOriginal + bufferOriginalLen) - prevMacroEnd) >= bufferLen) {
        char *tmp = realloc(bufferO, (buffer - bufferO) + ((bufferOriginal + bufferOriginalLen) - prevMacroEnd) +
                                     1); // +1 because why not
        if (!tmp) {
            fprintf(stderr, "Error: unable to reallocate more memory for macro processing buffer: %s\n",
                    strerror(errno));
            return buffer - bufferO;
        }
        size_t offset = buffer - bufferO;
        bufferO = tmp;
        buffer = bufferO + offset;
    }

    memcpy(buffer, prevMacroEnd, ((bufferOriginal + bufferOriginalLen) -
                                  prevMacroEnd)); // Copy everything leading up to the macro into new buffer
    buffer += ((bufferOriginal + bufferOriginalLen) - prevMacroEnd);

    (*bufferOut) = bufferO;

    return buffer - bufferO;
}


void readConfig(struct Option **config, const char *filename) {
    if (!config) {
        fprintf(stderr, "Error: Config '%s' not yet initialized\n", filename);
        return;
    }

    char *bufferOriginal = NULL;
    size_t length = readFile(filename, &bufferOriginal);

    //Get path to the parent directory used to find other files which may be included
    char *parentDirI = strrchr(filename, '/') + 1;
    char *parentDir = strndup(filename, parentDirI - filename);

    //Process macros before parsing file
    char *buffer = NULL;
    size_t len = preprocessor(bufferOriginal, length, &buffer, parentDir);

#ifndef NDEBUG
    FILE *fp = fopen("debug/preprocessor-output.txt", "w");
    fwrite(buffer, sizeof(char), len, fp);
    fclose(fp);
#endif

    parseConfigWhole(config, filename, buffer, len);

    free(bufferOriginal);
    free(buffer);
}

struct Option *get_(struct Option **options, char *optName) {
    if (!options) {
        fprintf(stderr, "Error: Config not yet initialized\n");
        return NULL;
    }

    char *dotIndex = strchr(optName, '.');
    char *name = optName;
    if (dotIndex) {
        name = strndup(optName, dotIndex - optName);
    }

    struct Option *opt;
    HASH_FIND(options, name, opt);
    if (!opt) {
        fprintf(stderr, "Error: Option %s not found\n", optName);
        return NULL;
    }
    if (dotIndex) {
        if (opt->type != COMPOUND) {
            fprintf(stderr, "Error: Only compound type options can have child options. Option %s is not compound.\n",
                    name);
            return NULL;
        }
        return get_(opt->v_v, dotIndex + 1);
    }

    return opt;
}

size_t getFileSize(const char *filename) {
#ifndef _WIN32
    struct stat st;
    stat(filename, &st);
    return st.st_size;
#else
    OFSTRUCT* fileInfo;
    HFILE file = OpenFile(filename, fileInfo, OF_READ);
    return GetFileSize(file, NULL);
#endif
}

void cleanOptions(Option **options) {
    if (!options)return;
    Option *opt;

    HASH_ITER(options, opt) {
            if (opt->type == TEXT) {
                if (opt->v_s != opt->dv_s)
                    free(opt->v_s); // Default value does not need to be freed because it was made from a string literal
            } else if (opt->type == COMPOUND) {
                cleanOptions(opt->v_v);
                if (opt->v_v != opt->dv_v) cleanOptions(opt->dv_v);
            }
            free(opt);
        }
    free(options);
}
