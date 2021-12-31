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
    for (int i = 0; i < count; ++i){
        struct Option* option = malloc(sizeof(struct Option));
        option->name = strdup(options[i].name);
        option->comment = strdup(options[i].comment);
        switch (options[i].type) {
            case TEXT:
            case TEXT_BLOCK:
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
            free(lineBufTrim);
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
            trim(strValueUntrimmed, &optStr);
            switch (optOut->type) {
                case TEXT:
                {
                    if (optOut->v_s) free(optOut->v_s);
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

    free(config->name);
    free(config->file);

    struct Option* currentOption, *tmpOpt;

    HASH_ITER(hh, config->options, currentOption, tmpOpt) {
        HASH_DEL(config->options, currentOption);

        if(currentOption->type==TEXT || currentOption->type==TEXT_BLOCK){
            free(currentOption->v_s);
            free(currentOption->dv_s);
        }
        free(currentOption->name);
        free(currentOption->comment);
        free(currentOption);
    }

    free(config);
}
