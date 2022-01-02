#define MULTI_CONFIG
#define AUTO_GENERATE
#include "libconf.h"
#include "timer.h"

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

    struct OptionOutline options[3] = {
                                        {"joe", NUMBER, "Hello world!\nyoe", 12},
                                        {"mama", TEXT, "This is a comment too!", .dv_s="Wooooooo!!!\nNow this is some serious multi line creip"},
                                        {"haha", BOOL, "Funny bool test", .dv_b=true}
                                        };
#ifndef MULTI_CONFIG
    TIMER_START(config);

    TIMER_START(init);
    initConfig("debug/test1.conf", 3, options);
    TIMER_END(init);

    TIMER_START(read);
    readConfig();
    TIMER_END(read);

    TIMER_START(get);
    int joe = 0;
    get("joe", &joe);
    printf("joe is: %d\n", joe);

    char* mama = NULL;
    get("mama", &mama);
    printf("mama is: %s\n", mama);

    bool haha = NULL;
    get("haha", &haha);
    printf("haha is: %s\n", haha ? "true" : "false");
    TIMER_END(get);

    TIMER_START(clean);
    cleanConfigs();
    TIMER_END(clean);
    TIMER_END(config);
#else
    TIMER_START(config);
    TIMER_START(init);
    initConfig("conf1", "debug/test1.conf", 3, options);
    TIMER_END(init);

    TIMER_START(read);
    readConfig("conf1");
    TIMER_END(read);

    TIMER_START(get);
    int joe = 0;
    get("conf1", "joe", &joe);
//    printf("joe is: %d\n", joe);

    char* mama = NULL;
    get("conf1", "mama", &mama);
//    printf("mama is: %s\n", mama);

    bool haha = NULL;
    get("conf1", "haha", &haha);
//    printf("haha is: %s\n", haha ? "true" : "false");
    TIMER_END(get);

    TIMER_START(clean);
    cleanConfigs();
    TIMER_END(clean);
    TIMER_END(config);
#endif
}