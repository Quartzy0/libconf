#define MULTI_CONFIG
#include "libconf.h"

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

#ifndef MULTI_CONFIG
    struct OptionOutline options[2] = {{"joe", INT, "Hello world!"}, {"mama", TEXT, "This is a comment too!"}};
    initConfig("test.conf", 2, options);

    readConfig();

    char* joe = get("joe");
    printf("joe is: %s\n", joe);

    cleanConfigs();
#else
    struct OptionOutline options[2] = {{"joe", INT, "Hello world!"}, {"mama", TEXT, "This is a comment too!"}};
    initConfig("conf1", "test.conf", 2, options);
    initConfig("conf2", "test_conf.conf", 2, options);

    readConfig("conf1");
    readConfig("conf2");

    char* joe = get("conf1", "joe");
    printf("joe is: %s\n", joe);

    char* mama = get("conf2", "mama");
    printf("mama is: %s\n", mama);

    cleanConfigs();
#endif
}