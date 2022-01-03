#define MULTI_CONFIG
#define AUTO_GENERATE
#include "libconf.h"
#include "timer.h"

int main(){
    struct OptionOutline compOpts2[2] = {
            {"lol", NUMBER, "no i disagrre", .dv_l=12},
            {"whyyy", TEXT, "and another one", .dv_s="nother one!"}
    };
    struct OptionOutline compOptions[3] = {
            {"test", TEXT, "jaja", .dv_s="very funny default value"},
            {"noyes", BOOL, "jerrry", .dv_b=false},
            {"compound_test2", COMPOUND, "lolanotherone", .dv_v=compOpts2, 2}
    };
    struct OptionOutline options[4] = {
                                        {"joe", NUMBER, "Hello world!\nyoe", 12},
                                        {"mama", TEXT, "This is a comment too!", .dv_s="Wooooooo!!!\nNow this is some serious multi line creip"},
                                        {"haha", BOOL, "Funny bool test", .dv_b=true},
                                        {"compound_test", COMPOUND, "lolllllllu", .dv_v=compOptions,3}
                                        };
    struct OptionOutline testOpt = {"test", TEXT, "Long text be like", .dv_s="lipsum"};
#ifndef MULTI_CONFIG
    TIMER_START(config);

    TIMER_START(init);
    initConfig("debug/test1.config", 3, options);
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
    initConfig("conf1", "debug/large.config", 1, &testOpt);
    TIMER_END(init);

    TIMER_START(read);
    readConfig("conf1");
    TIMER_END(read);

    TIMER_START(get);
//    int joe = 0;
//    get("conf1", "joe", &joe);
//    printf("joe is: %d\n", joe);
//
//    char* mama = NULL;
//    get("conf1", "mama", &mama);
//    printf("mama is: %s\n", mama);
//
//    bool haha = NULL;
//    get("conf1", "haha", &haha);
//    printf("haha is: %s\n", haha ? "true" : "false");
//
//    bool noyes = NULL;
//    get("conf1", "compound_test.noyes", &noyes);
//    printf("noyes is: %s\n", noyes ? "true" : "false");
//
//    char* test = NULL;
//    get("conf1", "compound_test.test", &test);
//    printf("test is: %s\n", test);
//
//    char* whyyy = NULL;
//    get("conf1", "compound_test.compound_test2.whyyy", &whyyy);
//    printf("whyyy is: %s\n", whyyy);
//
//    int lol = 0;
//    get("conf1", "compound_test.compound_test2.lol", &lol);
//    printf("lol is: %d\n", lol);
    TIMER_END(get);

    TIMER_START(clean);
    cleanConfigs();
    TIMER_END(clean);
    TIMER_END(config);
#endif
}