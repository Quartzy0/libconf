#include "../include/libconf.h"

#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>

#ifndef _WIN32

#include <sys/stat.h>

#else
#include <Windows.h>
#endif

struct ConfigOptions **configs; //Hash map of multiple configs
struct ConfigOptions *singleConfig; //Only one config

//utility functions

char *strchrnul_(char *s, char c) {
    if (*s == '\0' || *(s - 1) == '\0') return NULL;
    char *val = strchr(s, c);
    return !val ? s + strlen(s) - 1 : val;
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
            if (*i == '#') {
                endValueIndex = i;
                break;
            }
        }
        if (!assignIndex) {
            buffer = lineEnd + 1;
            continue;
        }

        char *optName = NULL;
        trimn(lineBufTrim, assignIndex - lineBufTrim, &optName);

        struct Option *optOut = NULL;
        HASH_FIND(options, optName, optOut);
        if (!optOut) {
            fprintf(stderr, "Unrecognized option '%s' found: %s\n", optName, file);
            goto loopEnd;
        }

        switch (optOut->type) {
            case TEXT: {
                if (optOut->v_s != optOut->dv_s && optOut->v_s) free(optOut->v_s);

                char *doubleStart = strchr(assignIndex + 1, '"');
                char *singleStart = strchr(assignIndex + 1, '\'');
                bool single = singleStart < doubleStart && singleStart;
                if((!single && !doubleStart) || (single ? singleStart > lineEnd : doubleStart > lineEnd)) {
                    fprintf(stderr, "Error at option %s: String must start on the same line as the option definition with ' or \"\n", optName);
                    break;
                }
                char searchChar = single ? '\'' : '"';
                char *stringStart = single ? singleStart : doubleStart;
                stringStart += 1 + (*(stringStart + 1) == '\n');

                char *multiLineEnd = strchr(stringStart, searchChar);
                if(!multiLineEnd){
                    fprintf(stderr, "Error at option %s: String must end with ' or \"\n", optName);
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
                trimnp(assignIndex + 1, lineEnd, &start);
                long tempL = strtol(start, &endPtr, 10);
                if (endPtr == start) {
                    //error
                    fprintf(stderr, "Invalid integer for option %s: %s \n", optName, file);
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
                trimnp(assignIndex + 1, lineEnd, &start);
                double tempD = strtod(start, &endPtr);
                if (endPtr == start) {
                    //error
                    fprintf(stderr, "Invalid double for option %s: %s\n", optName, file);
                    optOut->v_d = optOut->dv_d;
                } else {
                    optOut->v_d = tempD;
                }
                optOut->valueSize = sizeof(double);
                break;
            }
            case BOOL: {
                char *start = NULL;
                trimnp(assignIndex + 1, lineEnd, &start);
                if (!strncasecmp(start, "true", 4) || !strncasecmp(start, "yes", 3)) {
                    optOut->v_b = true;
                    optOut->valueSize = sizeof(bool);
                    break;
                } else if (!(!strncasecmp(start, "false", 5) || !strncasecmp(start, "no", 2))) {
                    fprintf(stderr, "Invalid boolean value for option %s. Must be true, false, yes or no: %s\n",
                            optName, file);
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
                if (!compoundStart || compoundStart > strchr(lineEnd + 1, '\n')) {
                    fprintf(stderr, "Compound must start with '{' in option %s: %s\n", optName, file);
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
                    fprintf(stderr, "Compound must end with '}' in option %s: %s\n", optName, file);

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
            }
        }


        loopEnd:
        buffer = lineEnd + 1;
        loopEndR:
        free(optName);
    }
}

void readConfig_(struct ConfigOptions *config) {
    if (!config) {
        fprintf(stderr, "Config not yet initialized\n");
        return;
    }

    char *bufferOriginal = NULL;
    size_t length;
    FILE *fp = fopen(config->file, "r");
    if (!fp) {
        fprintf(stderr, "Error while opening file '%s': '%s'\n", config->file, strerror(errno));
        return;
    }

    length = getFileSize(config->file);
    if (length == 0) {
        fclose(fp);
        return;
    }

    bufferOriginal = malloc(length + 1);
    if (!bufferOriginal) {
        fprintf(stderr, "Error allocating memory for reading file '%s': '%s'\n", config->file, strerror(errno));
        fclose(fp);
        free(bufferOriginal);
        return;
    }
    if (!fread(bufferOriginal, 1, length, fp)) {
        fprintf(stderr, "Error while reading file '%s': '%s'\n", config->file, strerror(errno));
        fclose(fp);
        free(bufferOriginal);
        return;
    }
    fclose(fp);

    bufferOriginal[length] = '\0'; //Must be null terminated

    parseConfigWhole(config->options, config->file, bufferOriginal, length);

    free(bufferOriginal);
}

struct Option *get_(struct Option **options, char *optName) {
    if (!options) {
        fprintf(stderr, "Config not yet initialized\n");
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
        fprintf(stderr, "Option %s not found\n", optName);
        return NULL;
    }
    if (dotIndex) {
        if (opt->type != COMPOUND) {
            fprintf(stderr, "Only compound type options can have child options. Option %s is not compound.\n", name);
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
            fprintf(stderr, "Error while reallocating memory for generated config file: '%s'", strerror(errno));
            return NULL;
        }
    }

    HASH_ITER(options, opt) {
            if (opt->comment) {
                char *comment = opt->comment;
                char *commentEnd = comment;
                while ((commentEnd = strchr(commentEnd + 1, '\n'))) {
                    addIndent(buf, indent);
                    *(buf++) = '#';
                    size_t commentLen = commentEnd - comment;
                    strncpy(buf, comment, commentLen);
                    buf += commentLen;
                    *(buf++) = '\n';

                    comment = commentEnd + 1;
                }
                addIndent(buf, indent);
                *(buf++) = '#';
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
        fprintf(stderr, "Error occurred while trying to open/create file '%s': %s\n", config->file, strerror(errno));
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

    for (struct Option **t = options; t - options <= OPTION_HASHMAP_SIZE, opt = *t; ++t)
        do {
            if (!opt) continue;
            fprintf(stdout, "Option: %s", opt->name);
        } while (opt && (opt = opt->next));

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
