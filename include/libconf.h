#ifndef LIBCONF_H
#define LIBCONF_H

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#define ARRAY_ALLOCATION 20 //elements in array (size in bytes depends on array type)

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

#define HASH_ADD(hashmap, value) do{                            \
    unsigned hash = FIRSTH;                                     \
    char *key = (value)->name;                                  \
    while (*key) {                                              \
        hash = (hash * A) ^ (key[0] * B);                       \
        key++;                                                  \
    }                                                           \
    hash %= OPTION_HASHMAP_SIZE;                                \
                                                                \
    DECLTYPE(hashmap) next = &(hashmap)[hash];                  \
    if (*next)                                                  \
        while (*(next = &((*next)->next))) {}                   \
    *next = value;                                              \
}while(0)

#define HASH_FIND(hashmap, key, out) do{                        \
    unsigned hash = FIRSTH;                                     \
    char* k = key;                                              \
    while (*k) {                                                \
        hash = (hash * A) ^ (k[0] * B);                         \
        k++;                                                    \
    }                                                           \
    hash %= OPTION_HASHMAP_SIZE;                                \
                                                                \
    (out) = (hashmap)[hash];                                    \
    while ((out) && strcmp(((out))->name, key) != 0) {          \
        (out) = ((out))->next;                                  \
    }                                                           \
}while(0)

enum Type {
    BOOL,
    LONG,
    DOUBLE,
    TEXT,
    COMPOUND,
    ARRAY
};

typedef struct ArrayOption {
    union {
        long *a_l;
        double *a_d;
        bool *a_b;
        char **a_s;
        struct {
            struct Option ***a_v; //Array of hash-tables (hash-table is ** and array is *)
            struct Option **a_v_t; //"Template" that all elements in the array will follow
        } a_v;
        struct {
            struct ArrayOption *a_a;
            struct ArrayOption *t;
        } a_a;
    };
    size_t len;
    enum Type type;
} ArrayOption;

typedef struct Option {
    char *name; //Key
    union {
        long v_l;
        double v_d;
        bool v_b;
        char *v_s;
        struct Option **v_v;
        struct ArrayOption v_a;
    };
    enum Type type;
    union {
        long dv_l;
        double dv_d;
        bool dv_b;
        char *dv_s;
        struct Option **dv_v;
        struct ArrayOption dv_a;
    };
    struct Option *next; //Used as a linked list in hash map
} Option;

size_t getFileSize(const char *filename);

void readConfig(Option **config, const char *filename);

struct Option *get_(Option **options, char *optName);

void cleanOptions(Option **options);

#define SET_FROM_OPTION(out, o) do{                             \
    *(out) = _Generic((*(out)),                                 \
                    long: (o)->v_l,                             \
                    int: (o)->v_l,                              \
                    long long: (o)->v_l,                        \
                    short: (o)->v_l,                            \
                    double: (o)->v_d,                           \
                    float: (o)->v_d,                            \
                    char*: (o)->v_s,                            \
                    bool: (o)->v_b,                             \
                    bool*: (o)->v_a.a_b,                        \
                    long*: (o)->v_a.a_l,                        \
                    double*: (o)->v_a.a_d,                      \
                    char**: (o)->v_a.a_s,                       \
                    Option***: (o)->v_a.a_v.a_v,                \
                    ArrayOption*: (o)->v_a.a_a.a_a,             \
                    default: (o)->v_v);                         \
}while(0)

#define INIT_CONFIG(conf) conf = malloc(OPTION_HASHMAP_SIZE * sizeof(struct Option *));

#define ADD_OPT_LONG(conf, name_in, default_v) do{              \
    struct Option *opt = malloc(sizeof(Option));                \
    opt->name = name_in;                                        \
    opt->type = LONG;                                           \
    opt->dv_l = (default_v);                                    \
    opt->v_l = opt->dv_l;                                       \
    HASH_ADD(conf, opt);                                        \
}while(0);

#define ADD_OPT_DOUBLE(conf, name_in, default_v) do{            \
    struct Option *opt = malloc(sizeof(Option));                \
    opt->name = name_in;                                        \
    opt->type = DOUBLE;                                         \
    opt->dv_d = (default_v);                                    \
    opt->v_d = opt->dv_d;                                       \
    HASH_ADD(conf, opt);                                        \
}while(0);

#define ADD_OPT_BOOL(conf, name_in, default_v) do{              \
    struct Option *opt = malloc(sizeof(Option));                \
    opt->name = name_in;                                        \
    opt->type = BOOL;                                           \
    opt->dv_b = (default_v);                                    \
    opt->v_b = opt->dv_b;                                       \
    HASH_ADD(conf, opt);                                        \
}while(0);

#define ADD_OPT_STR(conf, name_in, default_v) do{               \
    struct Option *opt = malloc(sizeof(Option));                \
    opt->name = name_in;                                        \
    opt->type = TEXT;                                           \
    opt->dv_s = (default_v);                                    \
    opt->v_s = opt->dv_s;                                       \
    HASH_ADD(conf, opt);                                        \
}while(0);

#define ADD_OPT_COMPOUND(conf, name_in, options) do{            \
    struct Option *opt = malloc(sizeof(Option));                \
    opt->name = name_in;                                        \
    opt->type = COMPOUND;                                       \
    opt->v_v = options;                                         \
    HASH_ADD(conf, opt);                                        \
}while(0)

#define ADD_OPT_ARRAY_LONG(conf, name_in, default_v) do{        \
    struct Option *opt = malloc(sizeof(Option));                \
    opt->name = name_in;                                        \
    opt->type = ARRAY;                                          \
    opt->dv_a.a_l = default_v;                                  \
    opt->dv_a.len = sizeof(default_v) / sizeof((default_v)[0]); \
    opt->dv_a.type = LONG;                                      \
    opt->v_a = opt->dv_a;                                       \
    HASH_ADD(conf, opt);                                        \
}while(0)

#define ADD_OPT_ARRAY_DOUBLE(conf, name_in, default_v) do{      \
    struct Option *opt = malloc(sizeof(Option));                \
    opt->name = name_in;                                        \
    opt->type = ARRAY;                                          \
    opt->dv_a.a_d = default_v;                                  \
    opt->dv_a.len = sizeof(default_v) / sizeof((default_v)[0]); \
    opt->dv_a.type = DOUBLE;                                    \
    opt->v_a = opt->dv_a;                                       \
    HASH_ADD(conf, opt);                                        \
}while(0)

#define ADD_OPT_ARRAY_BOOL(conf, name_in, default_v) do{        \
    struct Option *opt = malloc(sizeof(Option));                \
    opt->name = name_in;                                        \
    opt->type = ARRAY;                                          \
    opt->dv_a.a_b = default_v;                                  \
    opt->dv_a.len = sizeof(default_v) / sizeof((default_v)[0]); \
    opt->dv_a.type = BOOL;                                      \
    opt->v_a = opt->dv_a;                                       \
    HASH_ADD(conf, opt);                                        \
}while(0)

#define ADD_OPT_ARRAY_STR(conf, name_in, default_v, len_in) do{ \
    struct Option *opt = malloc(sizeof(Option));                \
    opt->name = name_in;                                        \
    opt->type = ARRAY;                                          \
    opt->dv_a.a_s = default_v;                                  \
    opt->dv_a.len = len_in;                                     \
    opt->dv_a.type = TEXT;                                      \
    opt->v_a = opt->dv_a;                                       \
    HASH_ADD(conf, opt);                                        \
}while(0)

#define ADD_OPT_ARRAY_COMPOUND(conf, name_in, template_v, default_v) do{\
    struct Option *opt = malloc(sizeof(Option));                \
    opt->name = name_in;                                        \
    opt->type = ARRAY;                                          \
    opt->dv_a.a_v.a_v = default_v;                              \
    opt->dv_a.a_v.a_v_t = template_v;                           \
    opt->dv_a.len = sizeof(default_v) / sizeof((default_v)[0]); \
    opt->dv_a.type = COMPOUND;                                  \
    opt->v_a = opt->dv_a;                                       \
    HASH_ADD(conf, opt);                                        \
}while(0)

#define ADD_OPT_ARRAY_ARRAY(conf, name_in, template_v, default_v, len_i) do{\
    struct Option *opt = malloc(sizeof(Option));                \
    opt->name = name_in;                                        \
    opt->type = ARRAY;                                          \
    opt->v_a.a_a.t = template_v;                                \
    opt->v_a.a_a.a_a = default_v;                               \
    opt->v_a.len = len_i;                                       \
    opt->v_a.type = ARRAY;                                      \
    HASH_ADD(conf, opt);                                        \
}while(0)

#define ARR_LONG(val, leni) (ArrayOption) {.type=LONG, .a_l=(val), .len=(leni)}

#define get(config, optName, out) do{                           \
    struct Option* o = get_(config, optName);                   \
    if(o) {                                                     \
        SET_FROM_OPTION(out, o);                                \
    }                                                           \
}while(0)
#define get_array(config, optName, out, size) do{               \
    struct Option* o = get_(config, optName);                   \
    if(o) {                                                     \
        SET_FROM_OPTION(out, o);                                \
        (size) = o->v_a.len;                                    \
    }                                                           \
}while(0)
#endif
