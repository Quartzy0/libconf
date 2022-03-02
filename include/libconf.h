#ifndef LIBCONF_H
#define LIBCONF_H

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#define NEW_CONFIG_BUFFER_SIZE 1024*1024 //1MB
#define NEW_CONFIG_BUFFER_MIN_DIFF_PER_OPT 256 //bytes
#define NEW_CONFIG_RANDOM_THRESHOLD 200 //bytes

/* These macros use decltype or the earlier __typeof GNU extension.
   As decltype is only available in newer compilers (VS2010 or gcc 4.3+
   when compiling c++ source) this code uses whatever method is needed
   or, for VS2008 where neither is available, uses casting workarounds. */
#if !defined(DECLTYPE) && !defined(NO_DECLTYPE)
#if defined(_MSC_VER)   /* MS compiler */
#if _MSC_VER >= 1600 && defined(__cplusplus)  /* VS2010 or newer in C++ mode */
#define DECLTYPE(x) decltype(x)
#else                   /* VS2008 or older (or VS2010 in C mode) */
#define NO_DECLTYPE
#endif
#elif defined(__BORLANDC__) || defined(__ICCARM__) || defined(__LCC__) || defined(__WATCOMC__)
#define NO_DECLTYPE
#else                   /* GNU, Sun and other compilers */
#define DECLTYPE(x) __typeof(x)
#endif
#endif

#define HASH_ITER(hashmap, o) for (DECLTYPE(hashmap) tmp = hashmap;tmp-(hashmap)<OPTION_HASHMAP_SIZE;++tmp) for((o) = *tmp;o;(o)=(o)->next)

#define OPTION_HASHMAP_SIZE 50

//https://stackoverflow.com/a/8317622
#define A 54059 /* a prime */
#define B 76963 /* another prime */
#define FIRSTH 37 /* also prime */

#define HASH_ADD(hashmap, value) do{            \
    unsigned hash = FIRSTH;                     \
    char *key = (value)->name;                  \
    while (*key) {                              \
        hash = (hash * A) ^ (key[0] * B);       \
        key++;                                  \
    }                                           \
    hash %= OPTION_HASHMAP_SIZE;                \
                                                \
    DECLTYPE(hashmap) next = &(hashmap)[hash];  \
    if (*next)                                  \
        while (*(next = &((*next)->next))) {}   \
    *next = value;                              \
}while(0)

#define HASH_FIND(hashmap, key, out) do{                \
    unsigned hash = FIRSTH;                             \
    char* k = key;                                      \
    while (*k) {                                        \
        hash = (hash * A) ^ (k[0] * B);                 \
        k++;                                            \
    }                                                   \
    hash %= OPTION_HASHMAP_SIZE;                        \
                                                        \
    (out) = (hashmap)[hash];                            \
    while ((out) && strcmp(((out))->name, key) != 0) {  \
        (out) = ((out))->next;                          \
    }                                                   \
}while(0)

enum Type {
    BOOL,
    LONG,
    DOUBLE,
    TEXT,
    COMPOUND,
    ARRAY_BOOL,
    ARRAY_LONG,
    ARRAY_DOUBLE,
    ARRAY_TEXT,
    ARRAY_COMPOUND,
    ARRAY_ARRAY
};

struct OptionOutline {
    char *name; //Key
    enum Type type;
    char *comment;
    union {
        long dv_l;
        double dv_d;
        bool dv_b;
        char *dv_s;
        struct OptionOutline *dv_v;
    };
    size_t compoundCount;
};

struct Option {
    char *name; //Key
    union {
        long v_l;
        double v_d;
        bool v_b;
        char *v_s;
        struct Option **v_v;
    };
    size_t valueSize;
    enum Type type;
    char *comment;
    union {
        long dv_l;
        double dv_d;
        bool dv_b;
        char *dv_s;
        struct Option **dv_v;
    };
    size_t line;
    struct Option *next; //Used as a linked list in hash map
};

struct ConfigOptions {
    char *name; //Key
    char *file;
    struct Option **options;
    struct ConfigOptions *next; //Used as a linked list in hash map
};

size_t optionCount;

size_t getFileSize(const char *filename);

void initConfig_(struct ConfigOptions **configIn, char *file, size_t count, struct OptionOutline *options);

void readConfig_(struct ConfigOptions *config);

void generateDefault_(struct ConfigOptions *config);

struct Option *get_(struct Option **options, char *optName);

void cleanConfig_(struct ConfigOptions *config);

void cleanOptions(struct Option **options);

#define SET_FROM_OPTION(out, o) do{                                         \
    *(out) = _Generic((*(out)),                                             \
                    long: (o)->v_l,                                         \
                    int: (o)->v_l,                                          \
                    long long: (o)->v_l,                                    \
                    short: (o)->v_l,                                        \
                    double: (o)->v_d,                                       \
                    float: (o)->v_d,                                        \
                    char*: (o)->v_s,                                        \
                    bool: (o)->v_b,                                         \
                    default: (o)->v_v);                                     \
}while(0)

#ifdef MULTI_CONFIG
extern struct ConfigOptions **configs; //Hash map of multiple configs
#define generateDefault(confName) do{                                       \
    struct ConfigOptions* c;                                                \
    HASH_FIND(configs, confName, c);                                        \
    if(!c){                                                                 \
        fprintf(stderr, "Config '%s' not yet initialized", confName);       \
        break;                                                              \
    }                                                                       \
    generateDefault_(c);                                                    \
}while(0)
#ifdef AUTO_GENERATE
#define initConfig(confName, file, count, options) do{ \
    if(!configs) configs = malloc(sizeof(struct ConfigOptions*) * OPTION_HASHMAP_SIZE);\
    struct ConfigOptions* c = NULL;                                         \
    if(configs){                                                            \
        HASH_FIND(configs, confName, c);                                    \
        if(c){                                                              \
            fprintf(stderr, "Config '%s' already initialized", confName);   \
            break;                                                          \
        }                                                                   \
    }                                                                       \
    initConfig_(&c, file, count, options);                                  \
    c->name = strdup(confName);                                             \
    HASH_ADD(configs, c);                                                   \
    FILE* fp = fopen(file, "r");                                            \
    if(!fp) generateDefault_(c);                                            \
    else fclose(fp);                                                        \
}while(0)
#else
#define initConfig(confName, file, count, options) do{ \
    if(!configs) configs = malloc(sizeof(struct ConfigOptions*) * OPTION_HASHMAP_SIZE);\
    struct ConfigOptions* c = NULL;                                         \
    if(configs){                                                            \
        HASH_FIND(configs, confName, c);                                    \
        if(c){                                                              \
            fprintf(stderr, "Config '%s' already initialized", confName);   \
            break;                                                          \
        }                                                                   \
    }                                                                       \
    initConfig_(&c, file, count, options);                                  \
    c->name = strdup(confName);                                             \
    HASH_ADD(configs, c);                                                   \
}while(0)
#endif
#define readConfig(confName) do{                                            \
    struct ConfigOptions* c;                                                \
    HASH_FIND(configs, confName, c);                                        \
    if(!c){                                                                 \
        fprintf(stderr, "Config '%s' not yet initialized", confName);       \
        break;                                                              \
    }                                                                       \
    readConfig_(c);                                                         \
}while(0)
#define get(confName, optName, out) do{                                     \
    struct ConfigOptions* c;                                                \
    HASH_FIND(configs, confName, c);                                        \
    if(!c){                                                                 \
        fprintf(stderr, "Config '%s' not yet initialized", confName);       \
        break;                                                              \
    }                                                                       \
    struct Option* o = get_(c->options, optName);                           \
    if(o){                                                                  \
        SET_FROM_OPTION(out, o);                                            \
    }                                                                       \
}while(0)
#define cleanConfigs() do{                                                  \
    struct ConfigOptions* c;                                                \
    HASH_ITER(configs, c){                                                  \
        cleanConfig_(c);                                                    \
    }                                                                       \
    free(configs);                                                          \
}while(0)
#else
extern struct ConfigOptions *singleConfig; //Only one config
#define generateDefault() generateDefault_(singleConfig)
#ifdef AUTO_GENERATE
#define initConfig(file, count, options) do{                                \
    initConfig_(&singleConfig, file, count, options);                       \
    singleConfig->name = NULL;                                              \
    FILE* fp = fopen(file, "r");                                            \
    if(!fp) generateDefault();                                              \
    else fclose(fp);                                                        \
}while(0)
#else
#define initConfig(file, count, options) do{                                \
    initConfig_(&singleConfig, file, count, options);                       \
    singleConfig->name = NULL;                                              \
}while(0)
#endif
#define readConfig() readConfig_(singleConfig)
#define get(optName, out) do{                                               \
    struct Option* o = get_(singleConfig->options, optName);                \
    if(o) {                                                                 \
        SET_FROM_OPTION(out, o);                                            \
    }                                                                       \
}while(0)
#define cleanConfigs() cleanConfig_(singleConfig)
#endif

#endif