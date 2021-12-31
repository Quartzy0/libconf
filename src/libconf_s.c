#include "libconf.h"

#include <stdlib.h>

struct ConfigOptions* config;

size_t trim(char* in, char** out);

void initConfig_s(char* file, size_t count, struct OptionOutline* options){
    config = malloc(sizeof(struct ConfigOptions));
    config->file = strdup(file);
    for (int i = 0; i < count; ++i){
        struct Option* option = malloc(sizeof(struct Option));
        option->name = strdup(options[i].name);
        option->comment = strdup(options[i].comment);
        if (options[i].defaultValue) {
            option->defaultValue = strdup(options[i].defaultValue);
            option->value = strdup(options[i].defaultValue);
        }
        option->type = options[i].type;
        HASH_ADD_STR(config->options, name, option);
    }
}

void readConfig_s(){
    if(!config){
        fprintf(stderr, "Config not yet initialized");
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
        trim(lineBuf, &lineBufTrim);

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
            fprintf(stderr, "Unrecognized option '%s' found in config '%s'\n", optionName, config->file);
        }else{
            if (optOut->value) free(optOut->value);
            trim(strValueUntrimmed, &optOut->value);
            optOut->line = lineCount;
        }

        free(strValueUntrimmed);
        free(optionNameUntrimmed);
        free(optionName);
        free(lineBufTrim);
    }

    free(lineBuf);
    fclose(fp);
}

char *get_s(char* optName) {
    if(!config){
        fprintf(stderr, "Config not yet initialized");
    }

    struct Option* opt;
    HASH_FIND_STR(config->options, optName, opt);
    if (!opt){
        fprintf(stderr, "Option %s not found in config '%s'\n", optName, config->file);
        return NULL;
    }
    return opt->value;
}

void cleanConfigs_s(){
    free(config->name);
    free(config->file);

    struct Option* currentOption, *tmpOpt;

    HASH_ITER(hh, config->options, currentOption, tmpOpt) {
        free(currentOption->value);
        free(currentOption->name);
        free(currentOption->defaultValue);
        free(currentOption->comment);
        free(currentOption);
    }

    free(config);
}
