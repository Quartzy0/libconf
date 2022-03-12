#define AUTO_GENERATE
#include "libconf.h"
#include "timer.h"

int main() {
    struct OptionOutline compOpts2[2] = {
            {"lol",   LONG, "no i disagrre", .dv_l=12},
            {"whyyy", TEXT, "and another one", .dv_s="nother one!"}
    };
    struct OptionOutline compOptions[3] = {
            {"test",           TEXT,     "jaja", .dv_s="very funny default value"},
            {"noyes",          BOOL,     "jerrry", .dv_b=false},
            {"compound_test2", COMPOUND, "lolanotherone", .dv_v=compOpts2, 2}
    };
    struct OptionOutline options[5] = {
            {"joe", LONG, "Hello world!\nyoe", 12},
            {"mama", TEXT, "This is a comment too!", .dv_s="Wooooooo!!!\nNow this is some serious multi line creip"},
            {"haha", BOOL, "Funny bool test", .dv_b=true},
            {"bruh", ARRAY_BOOL, "Funny man"},
            {"compound_test", COMPOUND, "lolllllllu", .dv_v=compOptions, 3}
    };
    struct OptionOutline testOpt = {"test", TEXT, "Long text be like\nwoooowooo\nscarrryyyy", .dv_s="lipsum"};

    TIMER_START(config);
    TIMER_START(init);
    ConfigOptions *config;
    initConfig(config, "debug/test.config", 5, options);
    TIMER_END(init);

    TIMER_START(read);
    readConfig(config);
    TIMER_END(read);

    TIMER_START(get);
    int joe = 0;
    get(config, "joe", &joe);
    printf("joe is: %d\n", joe);

    char* mama = NULL;
    get(config, "mama", &mama);
    printf("mama is: %s\n", mama);

    bool haha = NULL;
    get(config, "haha", &haha);
    printf("haha is: %s\n", haha ? "true" : "false");

    bool noyes = NULL;
    get(config, "compound_test.noyes", &noyes);
    printf("noyes is: %s\n", noyes ? "true" : "false");

    char* test = NULL;
    get(config, "compound_test.test", &test);
    printf("test is: %s\n", test);

    char* whyyy = NULL;
    get(config, "compound_test.compound_test2.whyyy", &whyyy);
    printf("whyyy is: %s\n", whyyy);

    int lol = 0;
    get(config, "compound_test.compound_test2.lol", &lol);
    printf("lol is: %d\n", lol);

    bool *bool_arr;
    size_t bool_len = 0;
    get_array(config, "bruh", &bool_arr, bool_len);

    for (int i = 0; i < bool_len; ++i){
        printf("bruh[%d]=%d\n", i, bool_arr[i]);
    }

    TIMER_END(get);

    TIMER_START(clean);
    cleanConfigs(config);
    TIMER_END(clean);
    TIMER_END(config);
}