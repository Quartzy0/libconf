#ifndef LIBCONF_H
#define LIBCONF_H

#include <stdio.h>
#include <stdbool.h>
#include "vendor/uthash/uthash.h"

#define LINE_BUFFER_SIZE 1024

enum Type{
    BOOL,
    NUMBER,
    DOUBLE,
    TEXT,
    TEXT_BLOCK
};

struct OptionOutline{
    char* name; //Key
    enum Type type;
    char* comment;
    union {
        long dv_l;
        double dv_d;
        bool dv_b;
        char* dv_s;
        void* dv_v;
    };
    UT_hash_handle hh;
};

struct Option{
    char* name; //Key
    union {
        long v_l;
        double v_d;
        bool v_b;
        char* v_s;
        void* v_v;
    };
    enum Type type;
    char* comment;
    union {
        long dv_l;
        double dv_d;
        bool dv_b;
        char* dv_s;
        void* dv_v;
    };
    size_t line;
    UT_hash_handle hh;
};

struct ConfigOptions{
    char* name; //Key
    char* file;
    struct Option* options;
    UT_hash_handle hh;
};


void initConfig_(struct ConfigOptions** configIn, char* file, size_t count, struct OptionOutline* options);

void readConfig_(struct ConfigOptions* config);

struct Option* get_(struct ConfigOptions* config, char* optName);

void cleanConfig_(struct ConfigOptions* config);

#define SET_FROM_OPTION(out, o) do{ \
    *(out) = _Generic((*(out)),                                         \
                    long: (o)->v_l,                                   \
                    int: (o)->v_l,                                    \
                    long long: (o)->v_l,                              \
                    short: (o)->v_l,                                  \
                    double: (o)->v_d,                                 \
                    float: (o)->v_d,                                  \
                    char*: (o)->v_s,                                  \
                    bool: (o)->v_b,                                   \
                    default: (o)->v_v);                               \
}while(0)

#ifdef MULTI_CONFIG
extern struct ConfigOptions* configs; //Hash map of multiple configs
#define initConfig(confName, file, count, options) do{                      \
    struct ConfigOptions* c = NULL;                                         \
    if(configs){                                                            \
        HASH_FIND_STR(configs, confName, c);                                \
        if(c){                                                              \
            fprintf(stderr, "Config '%s' already initialized", confName);   \
            break;                                                          \
        }                                                                   \
    }                                                                       \
    initConfig_(&c, file, count, options);                                  \
    c->name = strdup(confName);                                             \
    HASH_ADD_STR(configs, name, c);                                         \
}while(0)
#define readConfig(confName) do{                                    \
    struct ConfigOptions* c;                                        \
    HASH_FIND_STR(configs, confName, c);                            \
    if(!c){                                                         \
        fprintf(stderr, "Config '%s' not yet initialized", confName);\
        break;                                                      \
    }                                                               \
    readConfig_(c);                                                 \
}while(0)
#define get(confName, optName, out) do{                             \
    struct ConfigOptions* c;                                        \
    HASH_FIND_STR(configs, confName, c);                            \
    if(!c){                                                         \
        fprintf(stderr, "Config '%s' not yet initialized", confName);\
        break;                                                      \
    }                                                               \
    struct Option* o = get_(c, optName);                            \
    SET_FROM_OPTION(out, o);                                        \
}while(0)
#define cleanConfigs() do{                                          \
    struct ConfigOptions* c, *tmp;                                  \
    HASH_ITER(hh, configs, c, tmp){                                 \
        HASH_DEL(configs, c);                                       \
        cleanConfig_(c);                                            \
    }                                                               \
}while(0)
#else
extern struct ConfigOptions* singleConfig; //Only one config
#define initConfig(file, count, options) initConfig_(&singleConfig, file, count, options)
#define readConfig() readConfig_(singleConfig)
#define get(optName) get_(singleConfig, optName)
#define cleanConfigs() cleanConfig_(singleConfig)
#endif

#endif