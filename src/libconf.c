#include "libconf.h"

#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

struct ConfigOptions* configs; //Hash map of multiple configs
struct ConfigOptions* singleConfig; //Only one config

//utility functions

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

void initConfig_(struct ConfigOptions** configIn, char* file, size_t count, struct OptionOutline* options){
    (*configIn) = malloc(sizeof(struct ConfigOptions));
    (*configIn)->file = strdup(file);
    (*configIn)->options = NULL;
    for (int i = 0; i < count; ++i){
        struct Option* option = malloc(sizeof(struct Option));
        option->name = strdup(options[i].name);
        option->comment = strdup(options[i].comment);
        switch (options[i].type) {
            case TEXT:
            {
                if (options[i].dv_s){
                    option->dv_s = strdup(options[i].dv_s);
                    option->v_s = option->dv_s;
                }
                break;
            }
            case NUMBER:
            {
                option->dv_l = options[i].dv_l;
                option->v_l = option->dv_l;
                break;
            }
            case DOUBLE:
            {
                option->dv_d = options[i].dv_d;
                option->v_d = option->dv_d;
                break;
            }
            case BOOL:
            {
                option->dv_b = options[i].dv_b;
                option->v_b = option->dv_b;
                break;
            }
        }
        option->type = options[i].type;
        HASH_ADD_STR((*configIn)->options, name, option);
    }
}

void readConfig_(struct ConfigOptions* config){
    if(!config){
        fprintf(stderr, "Config not yet initialized\n");
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
