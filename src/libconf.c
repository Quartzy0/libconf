#include "libconf.h"

#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

#ifndef _WIN32
#include <sys/stat.h>
#else
#include <Windows.h>
#endif

struct ConfigOptions* configs; //Hash map of multiple configs
struct ConfigOptions* singleConfig; //Only one config

//utility functions

char* strchrnul_(char* s, char c){
    if (*s=='\0' || *(s-1)=='\0') return NULL;
    char* val = strchr(s, c);
    return !val ? s + strlen(s) - 1 : val;
}

/* Returns the new size of the string */
size_t trim(const char* in, char** out){
    if(!in) return 0;
    const char* begin = in;
    const char* end = in + strlen(in)-1;
    if (begin==end) {
        if (isspace(*in)) return 0;
        *out = strdup(in);
        return strlen(in);
    }

    while(isspace(*begin))
        begin++;
    while(isspace(*end))
        end--;
    end++;
    *out = strndup(begin, end-begin);
    return end-begin;
}

size_t trimn(const char* in, size_t n, char** out){
    if(!in) return 0;
    const char* begin = in;
    const char* end = in + n-1;
    if (begin==end) {
        if (isspace(*in)) return 0;
        *out = strdup(in);
        return strlen(in);
    }

    while(isspace(*begin))
        begin++;
    while(isspace(*end))
        end--;
    end++;
    *out = strndup(begin, end-begin);
    return end-begin;
}

void trimnp(char* in, const char* end, char** out){
    if(!in) return;
    char* begin = in;

    while(isspace(*begin) && begin<end)
        begin++;
    *out = begin;
}

struct Option* outlineToOption(struct OptionOutline* in){
    if (!in) return NULL;
    struct Option* option = malloc(sizeof(struct Option));
    option->name = strdup(in->name);
    option->comment = strdup(in->comment);
    switch (in->type) {
        case TEXT:
        {
            if (in->dv_s){
                option->dv_s = strdup(in->dv_s);
                option->v_s = option->dv_s;
            }
            break;
        }
        case NUMBER:
        {
            option->dv_l = in->dv_l;
            option->v_l = option->dv_l;
            break;
        }
        case DOUBLE:
        {
            option->dv_d = in->dv_d;
            option->v_d = option->dv_d;
            break;
        }
        case BOOL:
        {
            option->dv_b = in->dv_b;
            option->v_b = option->dv_b;
            break;
        }
        case COMPOUND:
        {
            struct OptionOutline* opt, *tmp;

            HASH_ITER(hh, in->dv_v, opt, tmp){
                struct Option* opt1 = outlineToOption(opt);
                HASH_ADD_STR(option->dv_v, name, opt1);
            }
            break;
        }
    }
    option->type = in->type;
    return option;
}

void initConfig_(struct ConfigOptions** configIn, char* file, size_t count, struct OptionOutline* options){
    (*configIn) = malloc(sizeof(struct ConfigOptions));
    (*configIn)->file = strdup(file);
    (*configIn)->options = NULL;
    for (int i = 0; i < count; ++i){
        struct Option* opt = outlineToOption(&(options[i]));
        HASH_ADD_STR((*configIn)->options, name, opt);
    }
}

void parseConfigWhole(struct ConfigOptions* config, char* bufferOriginal, size_t length){
    char* buffer = bufferOriginal;
    char* lineEnd = buffer;

    while ((lineEnd = strchrnul_(lineEnd + 1, '\n')) && lineEnd-bufferOriginal<length){
        char* lineBufTrim = NULL;
        trimnp(buffer, lineEnd, &lineBufTrim);
        if(!lineBufTrim){ //trim returns length of trimmed string. If the new string is 0, continue to the next line
            buffer = lineEnd + 1;
            continue;
        }

        char* assignIndex = NULL, *endValueIndex = lineEnd;
        for (char* i = lineBufTrim; i < lineEnd; ++i){
            if (*i=='=' && !assignIndex) {
                assignIndex = i;
            }
            if (*i=='#'){
                endValueIndex = i;
                break;
            }
        }
        if (!assignIndex){
            buffer = lineEnd + 1;
            continue;
        }

        char* optName = NULL;
        size_t optNameLen = trimn(lineBufTrim, assignIndex-lineBufTrim, &optName);
        char* optStr = NULL;

        struct Option* optOut = NULL;
        HASH_FIND_STR(config->options, optName, optOut);
        if (!optOut) {
            fprintf(stderr, "Unrecognized option '%s' found: %s\n", optName, config->file);
            goto loopEnd;
        }

        size_t optStrLen = trimn(assignIndex+1, endValueIndex-(assignIndex), &optStr);

        switch (optOut->type) {
            case TEXT:
            {
                if (optOut->v_s != optOut->dv_s && optOut->v_s) free(optOut->v_s);

                char* multiLineStart = strchr(assignIndex + 1, '"');
                if (!multiLineStart) goto singleNoQuotes;
                multiLineStart += 1 + (*(multiLineStart+1)=='\n');
                char* multiLineEnd = strchr(multiLineStart, '"');
                if (!multiLineEnd) goto singleNoQuotes;
                if (*(multiLineEnd-1)=='\n') multiLineEnd--;

                optOut->v_s = strndup(multiLineStart, multiLineEnd-multiLineStart);
                break;

                singleNoQuotes:
                {
                    trimn(assignIndex+1, lineEnd-assignIndex, &optOut->v_s);
                }
                break;
            }
            case NUMBER:
            {
                char* endPtr = NULL;
                char* start = NULL;
                trimnp(assignIndex+1, lineEnd, &start);
                long tempL = strtol(start, &endPtr, 10);
                if (endPtr==start){
                    //error
                    fprintf(stderr, "Invalid integer for option %s: %s \n", optName, config->file);
                    optOut->v_l = optOut->dv_l;
                }else{
                    optOut->v_l = tempL;
                }
                break;
            }
            case DOUBLE:
            {
                char* endPtr = NULL;
                char* start = NULL;
                trimnp(assignIndex+1, lineEnd, &start);
                double tempD = strtod(start, &endPtr);
                if (endPtr==optStr){
                    //error
                    fprintf(stderr, "Invalid double for option %s: %s\n", optName, config->file);
                    optOut->v_d = optOut->dv_d;
                }else{
                    optOut->v_d = tempD;
                }
                break;
            }
            case BOOL:
            {
                char* start = NULL;
                trimnp(assignIndex+1, lineEnd, &start);
                if (!strncasecmp(start, "true", 4) || !strncasecmp(start, "yes", 3)){
                    optOut->v_b = true;
                    break;
                }else if (!(!strncasecmp(start, "false", 5) || !strncasecmp(start, "no", 2))){
                    fprintf(stderr, "Invalid boolean value for option %s. Must be true, false, yes or no: %s\n", optName, config->file);
                    optOut->v_b = optOut->dv_b;
                    break;
                }
                optOut->v_b = false;
                break;
            }
            case COMPOUND:
            {
                char* compoundStart = strchr(assignIndex+1, '{');
                if (!compoundStart){
                    fprintf(stderr, "Compound must start with '{': %s\n", config->file);
                    optOut->v_v = optOut->dv_v;

                    free(optName);
                    free(optStr);
                    free(lineBufTrim);
                    return;
                }
                char* compoundEnd = strchr(assignIndex+1, '}');
                if (!compoundEnd){
                    fprintf(stderr, "Compound must end with '}': %s\n", config->file);
                    optOut->v_v = optOut->dv_v;

                    free(optName);
                    free(optStr);
                    free(lineBufTrim);
                    return;
                }


            }
        }


        loopEnd:
        free(optName);
        free(optStr);
        buffer = lineEnd + 1;
    }
}

void readConfig_(struct ConfigOptions* config){
    if(!config){
        fprintf(stderr, "Config not yet initialized\n");
        return;
    }

    if (getFileSize(config->file)<=MAX_READ){
        char* bufferOriginal = NULL;
        size_t length;
        FILE* fp = fopen(config->file, "r");
        if (!fp){
            fprintf(stderr, "Error while opening file '%s': '%s'", config->file, strerror(errno));
            return;
        }

        if(fseek(fp, 0, SEEK_END)){
            fprintf(stderr, "Error while seeking end of file '%s': '%s'", config->file, strerror(errno));
            return;
        }
        length = ftell(fp);

        if (fseek(fp, 0, SEEK_SET)){
            fprintf(stderr, "Error while seeking start of file '%s': '%s'", config->file, strerror(errno));
            return;
        }

        bufferOriginal = malloc(length + 1);
        if (!bufferOriginal){
            fprintf(stderr, "Error allocating memory for reading file '%s': '%s'", config->file, strerror(errno));
            return;
        }
        if(!fread(bufferOriginal, 1, length, fp)){
            fprintf(stderr, "Error while reading file '%s': '%s'", config->file, strerror(errno));
            return;
        }
        fclose(fp);

        bufferOriginal[length] = '\0'; //Must be null terminated

        parseConfigWhole(config, bufferOriginal, length);

        free(bufferOriginal);
        return;
    }

    FILE* fp = fopen(config->file, "r"); //Open file from config with read access
    if (!fp){
        fprintf(stderr, "Config file %s not found\n", config->file);
        return;
    }
    char* lineBuf = (char*) malloc(LINE_BUFFER_SIZE);
    int len = LINE_BUFFER_SIZE;
    size_t lineCount = 0; //Line count used to print useful error messages

    while(fgets(lineBuf, len, fp)){
        lineCount++;
        char* lineBufTrim = NULL;
        if(!trim(lineBuf, &lineBufTrim)){ //trim returns length of trimmed string. If the new string is 0, continue to the next line
            if (lineBufTrim) free(lineBufTrim);
            continue;
        }

        char* optionName = NULL;

        char* commentIndex = strchr(lineBufTrim, '#');
        if (commentIndex){
            *commentIndex = '\0';
            commentIndex++;
        }
        char* assignIndex = strchr(lineBufTrim, '=');
        if (!commentIndex && !assignIndex){
            fprintf(stderr, "Assignment operator ('=') not found: %s:%zu\n", config->file, lineCount);

            free(optionName);
            free(lineBufTrim);
            free(lineBuf);
            fclose(fp);
            return;
        }
        if (!assignIndex)continue;
        if (lineBufTrim==assignIndex){
            fprintf(stderr, "Option name can not be empty: %s:%zu\n", config->file, lineCount);

            free(optionName);
            free(lineBufTrim);
            free(lineBuf);
            fclose(fp);
            return;
        }

        char* optionNameUntrimmed = strndup(lineBufTrim, assignIndex-lineBufTrim);
        char* strValueUntrimmed = strdup(assignIndex+1);


        trim(optionNameUntrimmed, &optionName);
        struct Option* optOut = NULL;
        HASH_FIND_STR(config->options, optionName, optOut);
        if (!optOut){
            fprintf(stderr, "Unrecognized option '%s' found: %s:%zu\n", optionName, config->file, lineCount);
        }else{
            char* optStr = NULL;
            size_t optStrLen = trim(strValueUntrimmed, &optStr);
            switch (optOut->type) {
                case TEXT:
                {
                    if (optOut->v_s != optOut->dv_s && optOut->v_s) free(optOut->v_s);
                    if (*optStr=='"' && *(optStr+1)=='"' && *(optStr+2)=='"'){
                        if (optStrLen>3 && *(optStr+optStrLen-1)=='"' && *(optStr+optStrLen-2)=='"' && *(optStr+optStrLen-3)=='"'){
                            *(optStr+optStrLen-3) = '\0';
                            optOut->v_s = strndup(optStr+3, optStrLen-3);
                            break;
                        }
                        char* buf = NULL;
                        if(!(buf = malloc(MULTI_LINE_BUFFER_MIN_SIZE))){
                            fprintf(stderr, "Error while allocating space for multi-line buffer: '%s'", strerror(errno));
                            break;
                        }
                        buf[0] = '\0';
                        size_t bufLen = 0;
                        size_t bufMaxLen = MULTI_LINE_BUFFER_MIN_SIZE;
                        if (optStrLen>3) {
                            strcat(buf, optStr+3);
                            bufLen = optStrLen-3;
                            *(buf + bufLen - 1) = '\n';
                            *(buf + bufLen) = '\0';
                        }

                        while(fgets(lineBuf, len, fp)){
                            lineCount++;
                            size_t lineLen = strlen(lineBuf);
                            if (bufLen+lineLen>=bufMaxLen){
                                char* newBuf = realloc(buf, bufMaxLen+MULTI_LINE_BUFFER_MIN_SIZE);
                                if (!newBuf){
                                    fprintf(stderr, "Error while reallocating memory for multi-line buffer: '%s'",
                                            strerror(errno));
                                    if (*(buf + bufLen - 1)=='\n') *(buf + bufLen - 1) = '\0';
                                    optOut->v_s = strdup(buf);
                                    free(buf);
                                    goto endSwitch;
                                }
                                bufMaxLen+=MULTI_LINE_BUFFER_MIN_SIZE;
                                buf = newBuf;
                            }
                            if (*(lineBuf+lineLen-2)=='"' && *(lineBuf+lineLen-3)=='"' && *(lineBuf+lineLen-4)=='"'){
                                if (lineLen<=4) break;
                                strncat(buf, lineBuf, lineLen-4);
                                bufLen+=lineLen-4;
                                break;
                            }
                            strcat(buf, lineBuf);
                            bufLen+=lineLen;
                        }
                        if (*(buf + bufLen - 1)=='\n') *(buf + bufLen - 1) = '\0';
                        optOut->v_s = strdup(buf);
                        free(buf);
                        break;
                    }
                    if ((*optStr=='"' && *(optStr + optStrLen - 1)=='"') ||
                        (*optStr=='\'' && *(optStr + optStrLen - 1)=='\'')){
                        *(optStr + optStrLen - 1) = '\0';
                        optOut->v_s = strndup(optStr+1, optStrLen-2);
                    }else
                        optOut->v_s = strdup(optStr);
                    break;
                }
                case NUMBER:
                {
                    char* endPtr = NULL;
                    long tempL = strtol(optStr, &endPtr, 10);
                    if (endPtr==optStr){
                        //error
                        fprintf(stderr, "Invalid integer '%s': %s:%zu\n", strerror(errno), config->file, lineCount);
                        optOut->v_l = optOut->dv_l;
                    }else{
                        optOut->v_l = tempL;
                    }
                    break;
                }
                case DOUBLE:
                {
                    char* endPtr = NULL;
                    double tempD = strtod(optStr, &endPtr);
                    if (endPtr==optStr){
                        //error
                        fprintf(stderr, "Invalid double '%s': %s:%zu\n", strerror(errno), config->file, lineCount);
                        optOut->v_d = optOut->dv_d;
                    }else{
                        optOut->v_d = tempD;
                    }
                    break;
                }
                case BOOL:
                {
                    if (!strcasecmp(optStr, "true") || !strcasecmp(optStr, "yes")){
                        optOut->v_b = true;
                        break;
                    }else if (!(!strcasecmp(optStr, "false") || !strcasecmp(optStr, "no"))){
                        fprintf(stderr, "Invalid boolean value '%s'. Must be true, false, yes or no. %s:%zu\n", optStr, config->file, lineCount);
                        optOut->v_b = optOut->dv_b;
                        break;
                    }
                    optOut->v_b = false;
                    break;
                }
            }
                endSwitch:
            optOut->line = lineCount;
            free(optStr);
        }

        free(strValueUntrimmed);
        free(optionNameUntrimmed);
        free(optionName);
        free(lineBufTrim);
    }

    free(lineBuf);
    fclose(fp);
}

struct Option* get_(struct ConfigOptions* config, char* optName) {
    if(!config){
        fprintf(stderr, "Config not yet initialized\n");
        return NULL;
    }

    struct Option* opt;
    HASH_FIND_STR(config->options, optName, opt);
    if (!opt){
        fprintf(stderr, "Option %s not found in config '%s'\n", optName, config->file);
        return NULL;
    }
    return opt;
}

void cleanConfig_(struct ConfigOptions* config){
    if (!config) return;

    if (config->name) free(config->name);
    free(config->file);

    struct Option* currentOption, *tmpOpt;

    HASH_ITER(hh, config->options, currentOption, tmpOpt) {
        HASH_DEL(config->options, currentOption);

        if(currentOption->type==TEXT){
            free(currentOption->v_s);
            if (currentOption->v_s!=currentOption->dv_s) free(currentOption->dv_s); //Prevent double free in the case that the default and value are the same
        }
        free(currentOption->name);
        free(currentOption->comment);
        free(currentOption);
    }

    free(config);
}

void generateDefault_(struct ConfigOptions *config) {
    FILE* fp = fopen(config->file, "w");
    if (!fp){
        fprintf(stderr, "Error occurred while trying to open/create file '%s': %s", config->file, strerror(errno));
        return;
    }

    bool nfirst = false; //Used to not insert a new line at the first line
    size_t lineCount = 1;

    struct Option* opt, *tmp;
    HASH_ITER(hh, config->options, opt, tmp){
        if (opt->type==TEXT && !opt->dv_s)continue;

        if (nfirst){
            fputc('\n', fp);
            lineCount++;
        } else
            nfirst = true;

        char* comment = opt->comment;
        char* commentEnd = comment + strlen(comment);
        char* commentNext = NULL;
        while((commentNext = strchr(comment, '\n'))){
            char buf[commentNext-comment+3];
            snprintf(buf, commentNext-comment+3, "#%s\n", comment);
            fputs(buf, fp);
            comment = commentNext+1;
            lineCount++;
        }
        char buf[commentEnd-comment+3];
        snprintf(buf, commentEnd-comment+3, "#%s\n", comment);
        fputs(buf, fp);
        lineCount++;

        lineCount++;
        fputs(opt->name, fp);
        fputc('=', fp);
        switch (opt->type) {
            case TEXT:
            {
                if (opt->dv_s){
                    if (strchr(opt->dv_s, '\n')){
                        fprintf(fp, "\"\"\"\n%s\n\"\"\"", opt->dv_s);
                    }else{
                        fprintf(fp, "\"%s\"", opt->dv_s);
                    }
                }
                break;
            }
            case NUMBER:
            {
                fprintf(fp, "%ld", opt->dv_l);
                break;
            }
            case DOUBLE:
            {
                fprintf(fp, "%f", opt->dv_d);
                break;
            }
            case BOOL:
            {
                fputs(opt->dv_b ? "true" : "false", fp);
                break;
            }
        }
        fputc('\n', fp);
    }

    fclose(fp);
}

size_t getFileSize(const char* filename) {
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
