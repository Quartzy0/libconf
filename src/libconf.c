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
char *strrchr_(const char *end, size_t len, char c){
    for (size_t i = 0; i < len; ++i) {
        if(*(end-i)==c) return (char*) end-i;
    }
    return NULL;
}

//https://stackoverflow.com/a/146938/
bool isFile(const char *path){
    struct stat s;
    int status;
    if( (status = stat(path,&s)) == 0 )
    {
        return s.st_mode & S_IFREG;
    }
    else
    {
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

struct Option *outlineToOption(struct OptionOutline *in) {
    if (!in) return NULL;
    struct Option *option = malloc(sizeof(struct Option));
    option->name = strdup(in->name);
    option->comment = strdup(in->comment);
    ++optionCount;
    switch (in->type) {
        case TEXT: {
            if (in->dv_s) {
                option->dv_s = strdup(in->dv_s);
                option->v_s = option->dv_s;
                option->valueSize = strlen(option->v_s);
            }
            break;
        }
        case LONG: {
            option->dv_l = in->dv_l;
            option->v_l = option->dv_l;
            option->valueSize = sizeof(long);
            break;
        }
        case DOUBLE: {
            option->dv_d = in->dv_d;
            option->v_d = option->dv_d;
            option->valueSize = sizeof(double);
            break;
        }
        case BOOL: {
            option->dv_b = in->dv_b;
            option->v_b = option->dv_b;
            option->valueSize = sizeof(bool);
            break;
        }
        case COMPOUND: {
            option->v_v = malloc(OPTION_HASHMAP_SIZE * sizeof(struct Option *));
            for (int i = 0; i < in->compoundCount; ++i) {
                struct Option *opt1 = outlineToOption(&(in->dv_v[i]));
                HASH_ADD(option->v_v, opt1);
            }
            option->valueSize = sizeof(struct Option *);
            break;
        }
    }
    option->type = in->type;
    return option;
}

void initConfig_(struct ConfigOptions **configIn, char *file, size_t count, struct OptionOutline *options) {
    (*configIn) = malloc(sizeof(struct ConfigOptions));
    (*configIn)->file = strdup(file);
    (*configIn)->options = malloc(OPTION_HASHMAP_SIZE * sizeof(struct Option *));
    optionCount = 0;
    for (int i = 0; i < count; ++i) {
        struct Option *opt = outlineToOption(&(options[i]));
        HASH_ADD((*configIn)->options, opt);
    }
}

void parseConfigWhole(struct Option **options, const char *file, char *bufferOriginal, size_t length) {
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
            if (*i == '/' && *(i+1) == '/') {
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
                bool single = singleStart < doubleStart && singleStart;
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
                optOut->valueSize = multiLineEnd - stringStart;
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
                    fprintf(stderr, "Error at option %s:%s: Invalid integer \n", file, optName);
                    optOut->v_l = optOut->dv_l;
                } else {
                    optOut->v_l = tempL;
                }
                optOut->valueSize = sizeof(long);
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
                optOut->valueSize = sizeof(double);
                break;
            }
            case BOOL: {
                char *start = NULL;
                trimnp(assignIndex + 1, endValueIndex, &start);
                if (!strncasecmp(start, "true", 4) || !strncasecmp(start, "yes", 3)) {
                    optOut->v_b = true;
                    optOut->valueSize = sizeof(bool);
                    break;
                } else if (!(!strncasecmp(start, "false", 5) || !strncasecmp(start, "no", 2))) {
                    fprintf(stderr, "Error at option %s:%s: Invalid boolean. Must be true, false, yes or no\n",
                            file, optName);
                    optOut->v_b = optOut->dv_b;
                    optOut->valueSize = sizeof(bool);
                    break;
                }
                optOut->v_b = false;
                optOut->valueSize = sizeof(bool);
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
                optOut->valueSize = sizeof(struct Option *);
                buffer = strchrnul_(compoundEnd, '\n');
                if (!buffer) {
                    free(optName);
                    return;
                }
                lineEnd = buffer;
                break;
            }
            case ARRAY_BOOL: {
                char *arrayStart = strchr(assignIndex + 1, '[');
                if(!arrayStart || arrayStart > endValueIndex){
                    fprintf(stderr, "Error at option %s:%s: Array must start on the same line as the option definition with [\n", file, optName);
                    break;
                }
                char *arrayEnd = strchr(arrayStart + 1, ']');
                if(!arrayEnd){
                    fprintf(stderr, "Error at option %s:%s: Array must end with ]\n", file, optName);
                    break;
                }

                bool *arr = malloc(sizeof(bool) * ARRAY_ALLOCATION);
                size_t i = 0;
                size_t arraySize = ARRAY_ALLOCATION;
                char *nextElement = arrayStart + 1;
                char *currentElement = arrayStart + 1;
                do{
                    nextElement = strchr(nextElement + 1, ',');
                    if(nextElement>arrayEnd || !nextElement) nextElement = arrayEnd;
                    char *valueStart;
                    trimnp(currentElement, nextElement, &valueStart);
                    if(arraySize-2==i){
                        bool *tmp = realloc(arr, (arraySize + ARRAY_ALLOCATION) * sizeof(bool));
                        if(!tmp){
                            fprintf(stderr, "Error while reallocating memory for bool array: %s\n", strerror(errno));
                            free(arr);
                            buffer = arrayEnd + 1;
                            goto loopEndR;
                        }
                        arr = tmp;
                    }
                    arr[i] = !strncasecmp(valueStart, "true", 4) || !strncasecmp(valueStart, "yes", 3);
                    if (!arr[i] && !(!strncasecmp(valueStart, "false", 5) || !strncasecmp(valueStart, "no", 2))) { // If option is not true or false
                        fprintf(stderr, "Error at option %s:%s[%zu]: Invalid boolean. Must be true, false, yes or no\n",
                                file, optName, i);
                        free(arr);
                        buffer = arrayEnd + 1;
                        goto loopEndR;
                    }
                    i++;
                    currentElement = nextElement + 1;
                }while(nextElement!=arrayEnd);

                if(i!=arraySize){
                    bool *tmp = realloc(arr, i * sizeof(bool));
                    if(!tmp){
                        fprintf(stderr, "Error while reallocating memory for bool array: %s\n", strerror(errno));
                        free(arr);
                        buffer = arrayEnd + 1;
                        goto loopEndR;
                    }
                    arr = tmp;
                }
                optOut->v_a.a_b = arr;
                optOut->v_a.len = i;
                optOut->valueSize = i * sizeof(bool);
                break;
            }
        }


        loopEnd:
        buffer = lineEnd + 1;
        loopEndR:
        free(optName);
    }
}

size_t readFile(const char *filename, char **bufferOut){
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

    if((*bufferOut)[length-1]!='\n'){ //Ensure that string ends in new line (makes things nicer)
        length++;
        (*bufferOut)[length-1]='\n';
    }
    (*bufferOut)[length] = '\0'; //Must be null terminated
    return length;
}

size_t preprocessor(char *bufferOriginal, size_t bufferOriginalLen, char **bufferOut, char *dirPath){
    size_t dirPathLen = strlen(dirPath);
    char *bufferO = malloc(bufferOriginalLen);
    char *buffer = bufferO; // Increment this buffer in order to keep the original for realloc and measuring size
    size_t bufferLen = bufferOriginalLen;
    char *macroStart = bufferOriginal;
    char *prevMacroEnd = bufferOriginal;
    bool macroUsed = false;

    while ((macroStart = strchr(macroStart, '#')) && macroStart - bufferOriginal < bufferOriginalLen) {
        char *lineBegin = strrchr_(macroStart, prevMacroEnd - macroStart, '\n') + 1;
        if(lineBegin==NULL+1) { // 1 was added to lineBegin, so it will be NULL + 1 if it would usually be NULL
            lineBegin = prevMacroEnd;
        }
        for (;lineBegin<macroStart;lineBegin++) { //Macro has to be the only thing on the line
            if(!isspace(*lineBegin)) goto eol;
        }

        char *lineEnd = strchr(macroStart+1, '\n');
        if(!(strncmp(macroStart+1, "include", 7))){
            char *pathStart = memchr(macroStart+7, '"', lineEnd-macroStart);
            if(!pathStart){
                fprintf(stderr, "Error: macro #include must have a path specified in '\"'\n");
                goto eol;
            }
            pathStart++;
            char *pathEnd = memchr(pathStart, '"', lineEnd-(pathStart));
            if(!pathEnd){
                fprintf(stderr, "Error: macro #include must have a path specified in '\"'\n");
                goto eol;
            }
            char *fileName = malloc(((pathEnd-pathStart+dirPathLen + 1) * sizeof(char)));
            memcpy(fileName, dirPath, dirPathLen);
            memcpy(fileName+dirPathLen, pathStart, pathEnd-pathStart);
            fileName[pathEnd-pathStart+dirPathLen] = '\0'; //Must be null-terminated

            glob_t glob_result;
            memset(&glob_result, 0, sizeof(glob_result));

            int glob_return = glob(fileName, GLOB_TILDE, NULL, &glob_result);

            switch (glob_return) {
                case GLOB_NOSPACE:
                {
                    fprintf(stderr, "Error: glob() failed with return code GLOB_NOSPACE (Out of memory)\n");
                    globfree(&glob_result);
                    goto eol;
                }
                case GLOB_ABORTED:
                {
                    fprintf(stderr, "Error: glob() failed with return code GLOB_ABORTED (Read error)\n");
                    globfree(&glob_result);
                    goto eol;
                }
                case GLOB_NOMATCH:
                {
                    fprintf(stderr, "Error: No files found matching pattern '%s'\n", fileName);
                    globfree(&glob_result);
                    goto eol;
                }
                default:
                {
                    break;
                }
            }

            if(buffer - bufferO + (macroStart - prevMacroEnd) >= bufferLen){
                char *tmp = realloc(bufferO, bufferLen + (macroStart - prevMacroEnd) + 1); // +1 because why not
                if(!tmp){
                    fprintf(stderr, "Error: unable to reallocate more memory for macro processing buffer: %s\n", strerror(errno));
                    goto eol;
                }
                bufferLen+= (macroStart - prevMacroEnd) + 1;
            }

            memcpy(buffer, prevMacroEnd, macroStart - prevMacroEnd); // Copy everything leading up to the macro into new buffer
            buffer+= macroStart - prevMacroEnd;

            for (int i = 0; i < glob_result.gl_pathc; ++i){
                if(!isFile(glob_result.gl_pathv[i]))continue; //Continue if it's not a file
                char *fileBuf = NULL;
                size_t len = readFile(glob_result.gl_pathv[i], &fileBuf);
                if(!len){
                    fprintf(stderr, "Error: unable to read file: %s\n", glob_result.gl_pathv[i]);
                    continue;
                }

                //Get path to the parent directory used to find other files which may be included
                char *parentDirI = strrchr(fileName, '/') + 1;
                char *parentDir = strndup(fileName, parentDirI-fileName);

                char *fileProcessed = NULL;
                size_t len1 = preprocessor(fileBuf, len, &fileProcessed, parentDir);

                // First make sure new buffer has enough space
                if(buffer - bufferO + len >= bufferLen){
                    char *tmp = realloc(bufferO, bufferLen + len + 1); // +1 because why not
                    if(!tmp){
                        fprintf(stderr, "Error: unable to reallocate more memory for macro processing buffer: %s\n", strerror(errno));
                        goto eol;
                    }
                    size_t offset = buffer-bufferO;
                    bufferO = tmp;
                    buffer = bufferO + offset;
                    bufferLen+= len + 1;
                }


                memcpy(buffer, fileProcessed, len1); // Copy everything from included file into new buffer
                buffer+=len1;
            }

            globfree(&glob_result);

            prevMacroEnd = lineEnd;
            macroUsed = true;
        }
        eol:
        macroStart++;
    }
    if(!macroUsed){
        (*bufferOut) = bufferOriginal;
        return bufferOriginalLen;
    }

    // First make sure new buffer has enough space
    if((buffer-bufferO)+((bufferOriginal+bufferOriginalLen)-prevMacroEnd) >= bufferLen){
        char *tmp = realloc(bufferO, (buffer-bufferO)+((bufferOriginal+bufferOriginalLen)-prevMacroEnd) + 1); // +1 because why not
        if(!tmp){
            fprintf(stderr, "Error: unable to reallocate more memory for macro processing buffer: %s\n", strerror(errno));
            return buffer-bufferO;
        }
        size_t offset = buffer-bufferO;
        bufferO = tmp;
        buffer = bufferO + offset;
    }

    memcpy(buffer, prevMacroEnd, ((bufferOriginal+bufferOriginalLen)-prevMacroEnd)); // Copy everything leading up to the macro into new buffer
    buffer+= ((bufferOriginal+bufferOriginalLen)-prevMacroEnd);

    (*bufferOut) = bufferO;

    return buffer-bufferO;
}


void readConfig_(struct ConfigOptions *config) {
    if (!config) {
        fprintf(stderr, "Error: Config '%s' not yet initialized\n", config->file);
        return;
    }

    char *bufferOriginal = NULL;
    size_t length = readFile(config->file, &bufferOriginal);

    //Get path to the parent directory used to find other files which may be included
    char *parentDirI = strrchr(config->file, '/') + 1;
    char *parentDir = strndup(config->file, parentDirI-config->file);

    //Process macros before parsing file
    char *buffer = NULL;
    size_t len = preprocessor(bufferOriginal, length, &buffer, parentDir);

#ifndef NDEBUG
    FILE *fp = fopen("debug/preprocessor-output.txt", "w");
    fwrite(buffer, sizeof(char), len, fp);
    fclose(fp);
#endif

    parseConfigWhole(config->options, config->file, buffer, len);

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

void cleanConfig_(struct ConfigOptions *config) {
    if (!config) return;

    if (config->name) free(config->name);
    free(config->file);

    cleanOptions(config->options);

    free(config);
}

//Utility macro
#define addIndent(buf, indent) { for(int i = 0;i<(indent);++i) { *((buf)+i)='\t'; } (buf)+=(indent); }

char *generateDef(struct Option **options, char **obuffer, size_t bufferOffset, size_t bufferSize, size_t indent) {
    struct Option *opt;
    size_t bufSize = bufferSize;
    char *buf = (*obuffer) + bufferOffset;

    if (bufSize - bufferOffset >= NEW_CONFIG_RANDOM_THRESHOLD) {
        *obuffer = realloc(*obuffer, bufferOffset + NEW_CONFIG_BUFFER_SIZE);
        if (!*obuffer) {
            fprintf(stderr, "Error: Can't reallocate memory for generated config file: '%s'", strerror(errno));
            return NULL;
        }
    }

    HASH_ITER(options, opt) {
            if (opt->comment) {
                char *comment = opt->comment;
                char *commentEnd = comment;
                while ((commentEnd = strchr(commentEnd + 1, '\n'))) {
                    addIndent(buf, indent);
                    *(buf++) = '/';
                    *(buf++) = '/';
                    size_t commentLen = commentEnd - comment;
                    strncpy(buf, comment, commentLen);
                    buf += commentLen;
                    *(buf++) = '\n';

                    comment = commentEnd + 1;
                }
                addIndent(buf, indent);
                *(buf++) = '/';
                *(buf++) = '/';
                size_t commentLen = strlen(comment);
                strncpy(buf, comment, commentLen);
                buf += commentLen;
                *(buf++) = '\n';
            }
            addIndent(buf, indent);
            strcpy(buf, opt->name);
            buf += strlen(opt->name);
            *(buf++) = ' ';
            *(buf++) = '=';
            *(buf++) = ' ';

            //Insert value
            switch (opt->type) {
                case TEXT: {
                    *(buf++) = '"';
                    strcpy(buf, opt->v_s);
                    buf += opt->valueSize;
                    *(buf++) = '"';
                    break;
                }
                case LONG: {
                    buf += sprintf(buf, "%ld", opt->v_l);
                    break;
                }
                case DOUBLE: {
                    buf += sprintf(buf, "%f", opt->v_d);
                    break;
                }
                case BOOL: {
                    if (opt->v_b)
                        buf += sprintf(buf, "true");
                    else
                        buf += sprintf(buf, "false");
                    break;
                }
                case COMPOUND: {
                    if (opt->v_v) {
                        *(buf++) = '{';
                        *(buf++) = '\n';
                        buf = generateDef(opt->v_v, obuffer, buf - *obuffer, bufSize, indent + 1);
                        if (!buf) return NULL;
                        addIndent(buf, indent + 1);
                        *(buf - 1) = '}';
                    }
                    break;
                }
            }
            *(buf++) = '\n';
            *(buf++) = '\n';
        }
    return buf;
}

void generateDefault_(struct ConfigOptions *config) {
    FILE *fp = fopen(config->file, "w");
    if (!fp) {
        fprintf(stderr, "Error: Can't open/create file '%s': %s\n", config->file, strerror(errno));
        return;
    }

    //Allocate the larger amount of bytes: 1MB or Number of options * 256
    char *buffer = malloc(NEW_CONFIG_BUFFER_MIN_DIFF_PER_OPT * optionCount > NEW_CONFIG_BUFFER_SIZE ?
                          NEW_CONFIG_BUFFER_MIN_DIFF_PER_OPT * optionCount : NEW_CONFIG_BUFFER_SIZE);
    char *endBuf = generateDef(config->options, &buffer, 0, NEW_CONFIG_BUFFER_SIZE, 0);
    if (!endBuf)return;
    *endBuf = '\0';
    fputs(buffer, fp);
    fclose(fp);
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

void cleanOptions(struct Option **options) {
    if (!options)return;
    struct Option *opt;

    HASH_ITER(options, opt) {
            if (opt->type == TEXT) {
                free(opt->v_s);
                if (opt->v_s != opt->dv_s)
                    free(opt->dv_s); //Prevent double free in the case that the default and value are the same
            } else if (opt->type == COMPOUND) {
                cleanOptions(opt->v_v);
                if (opt->v_v != opt->dv_v) cleanOptions(opt->dv_v);
            }
            free(opt->name);
            free(opt->comment);
            free(opt);
        }
}
