#ifndef LIBCONF_H
#define LIBCONF_H

#include <stdio.h>
#include "vendor/uthash/uthash.h"

#define LINE_BUFFER_SIZE 1024

void dumpFile(FILE* fp);

enum Type{
    INT,
    DOUBLE,
    TEXT,
    TEXT_BLOCK
};

struct OptionOutline{
    char* name; //Key
    enum Type type;
    char* comment;
    char* defaultValue;
    UT_hash_handle hh;
};

struct Option{
    char* name; //Key
    char* value;
    enum Type type;
    char* comment;
    char* defaultValue;
    size_t line;
    UT_hash_handle hh;
};

struct ConfigOptions{
    char* name; //Key
    char* file;
    struct Option* options;
    UT_hash_handle hh;
};

extern struct ConfigOptions* config;

void initConfig_s(char* file, size_t count, struct OptionOutline* options);

void readConfig_s();

char* get_s(char* optName);

void cleanConfigs_s();

extern struct ConfigOptions* configs;

void initConfig_m(char* name, char* file, size_t count, struct OptionOutline* options);

void readConfig_m(char* name);

char* get_m(char* confName, char* optName);

void cleanConfigs_m();

#ifdef MULTI_CONFIG
#define initConfig(name, file, count, options) initConfig_m(name, file, count, options)
#define readConfig(name) readConfig_m(name)
#define get(confName, optName) get_m(confName, optName)
#define cleanConfigs() cleanConfigs_m()
#else
#define initConfig(file, count, options) initConfig_s(file, count, options)
#define readConfig() readConfig_s()
#define get(optName) get_s(optName)
#define cleanConfigs() cleanConfigs_s()
#endif

#endif