#define MULTI_CONFIG
#include "libconf.h"
#include <time.h>

struct Config{
    char* joe;
};

struct conf_element
{
    char* key;
    union 
    {
        int v_i;
        float v_f;
        void* v_ptr;
        /* data */
    };
    int type;
};

enum Epic_Enum
{
    option_1,
    option_2,
    option_count,
};

struct conf_element SOME_ARRAY[option_count] = 
{
    [option_1] = {.key = "op1"},
    [option_2] = {.key = "option2"},
};



int main(){

    /*for (enum Epic_Enum i = 0; i < option_count; ++i)
    {
        if (!strcmp("epic string", SOME_ARRAY[i].key))
        {
            //  Našel
        }
    }*/

    struct OptionOutline options[3] = { {"joe", NUMBER, "Hello world!"},
                                        {"mama", TEXT, "This is a comment too!"},
                                        {"haha", BOOL, "Funny bool test"}};
#ifndef MULTI_CONFIG
    initConfig("test.conf", 2, options);

    readConfig();

    char* joe = get("joe");
    printf("joe is: %s\n", joe);

    cleanConfigs();
#else
    clock_t timeStart = clock();
    initConfig("conf1", "test.conf", 3, options);
    initConfig("conf2", "test_conf.conf", 3, options);
    clock_t timeInitConfig = clock();

    readConfig("conf1");
    readConfig("conf2");
    clock_t timeReadConfig = clock();

    int joe = 0;
    get("conf1", "joe", &joe);
    printf("joe is: %d\n", joe);

    char* mama = NULL;
    get("conf2", "mama", &mama);
    printf("mama is: %s\n", mama);

    bool haha = NULL;
    get("conf1", "haha", &haha);
    printf("haha is: %s\n", haha ? "true" : "false");
    clock_t timeGetVars = clock();

    cleanConfigs();
    clock_t timeClean = clock();

    printf("Init: %f ms, Read: %f ms, Get: %f ms, Clean: %f ms, Total: %f ms\n", ((double) timeInitConfig-timeStart)/CLOCKS_PER_SEC*1000,
           ((double) timeReadConfig-timeInitConfig)/CLOCKS_PER_SEC*1000, ((double)timeGetVars-timeReadConfig)/CLOCKS_PER_SEC*1000,
           ((double)timeClean-timeGetVars)/CLOCKS_PER_SEC*1000, ((double)timeClean-timeStart)/CLOCKS_PER_SEC*1000);

#endif
}