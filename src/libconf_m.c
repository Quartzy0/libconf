#include "libconf.h"

#include <stdlib.h>
#include <ctype.h>

struct ConfigOptions* configs;

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

void dumpFile(FILE* fp){
    char* lineBuf = (char*) malloc(LINE_BUFFER_SIZE);
    int len = LINE_BUFFER_SIZE;

    while(fgets(lineBuf, len, fp)){
        printf("%s", lineBuf);
    }
}

void initConfig_m(char* name, char* file, size_t count, struct OptionOutline* options){
    struct ConfigOptions* c;

    //See if config with that name already exists
    HASH_FIND_STR(configs, name, c);
    if (!c){ //Only add of doesn't already exist
        c = malloc(sizeof(struct ConfigOptions));
        c->name = strdup(name);
        c->file = strdup(file);
        for (int i = 0; i < count; ++i){
            struct Option* option = malloc(sizeof(struct Option));
            option->name = strdup(options[i].name);
            option->comment = strdup(options[i].comment);
            if (options[i].defaultValue) {
                option->defaultValue = strdup(options[i].defaultValue);
                option->value = strdup(options[i].defaultValue);
            }
            option->type = options[i].type;
            HASH_ADD_STR(c->options, name, option);
        }
        HASH_ADD_STR(configs, name, c);
    }else{ //Warn user of name conflict
        fprintf(stderr, "Config with the name %s already exists\n", name);
    }
}

void readConfig_m(char* name){
    struct ConfigOptions* c;

    HASH_FIND_STR(configs, name, c);
    if(!c){
        fprintf(stderr, "Config with the name %s doesn't exist\n", name); //Warn user of invalid config name
        return;
    }

    FILE* fp = fopen(c->file, "r"); //Open file from config with read access
    if (!fp){
        fprintf(stderr, "Config file %s not found\n", c->file);
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
            fprintf(stderr, "Assignment operator ('=') not found: %s:%zu\n", c->file, lineCount);

            free(optionName);
            free(lineBufTrim);
            free(lineBuf);
            fclose(fp);
            return;
        }
        if (!assignIndex)continue;
        if (lineBufTrim==assignIndex){
            fprintf(stderr, "Option name can not be empty: %s:%zu\n", c->file, lineCount);

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
        HASH_FIND_STR(c->options, optionName, optOut);
        if (!optOut){
            fprintf(stderr, "Unrecognized option '%s' found in config '%s'\n", optionName, c->file);
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

char *get_m(char *confName, char* optName) {
    struct ConfigOptions* c;
    HASH_FIND_STR(configs, confName, c);
    if (!c){
        fprintf(stderr, "Config %s not found\n", confName);
        return NULL;
    }

    struct Option* opt;
    HASH_FIND_STR(c->options, optName, opt);
    if (!opt){
        fprintf(stderr, "Option %s not found in config '%s'\n", optName, c->file);
        return NULL;
    }
    return opt->value;
}

void cleanConfigs_m(){
    struct ConfigOptions *c, *tmp;

    HASH_ITER(hh, configs, c, tmp) {
        HASH_DEL(configs, c); //Remove from hash table
        free(c->name);
        free(c->file);

        struct Option* currentOption, *tmpOpt;

        HASH_ITER(hh, c->options, currentOption, tmpOpt) {
            free(currentOption->value);
            free(currentOption->name);
            free(currentOption->defaultValue);
            free(currentOption->comment);
            free(currentOption);
        }

        free(c);          //And free memory
    }
}
