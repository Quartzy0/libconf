#include "../include/libconf.h"
#include "timer.h"

int main() {
    TIMER_START(config);

    TIMER_START(init);
    Option **config;
    Option **configM;

    INIT_CONFIG(config);
    INIT_CONFIG(configM);

    ADD_OPT_LONG(configM, "breh", 12);
//    ADD_OPT_COMPOUND(config, "man", configM);
    ADD_OPT_DOUBLE(config, "dubl", 32.2);
    ADD_OPT_STR(config, "str", "test");
    ADD_OPT_BOOL(config, "bl", false);

    long arr_def[] = {33, 22, 44};
    ADD_OPT_ARRAY_LONG(configM, "arr", arr_def);

    double arrd_def[] = {55.0122, 233.23};
    ADD_OPT_ARRAY_DOUBLE(configM, "arrd", arrd_def);

    bool arrb_def[] = {true, false};
    ADD_OPT_ARRAY_BOOL(configM, "arrb", arrb_def);

    char *arrs_def[] = {"fakse", "yes", "falssse"};
    ADD_OPT_ARRAY_STR(config, "arrs", arrs_def, 3);

    Option **arr_opt_templ;
    INIT_CONFIG(arr_opt_templ);
    ADD_OPT_STR(arr_opt_templ, "ll", "lolllll");
    ADD_OPT_ARRAY_COMPOUND(config, "arrc", configM, NULL);
    TIMER_END(init);

    TIMER_START(read);
    readConfig(config, "debug/test.config");
    TIMER_END(read);

    TIMER_START(get);
    double dubl = 0;
    get(config, "dubl", &dubl);
    printf("dubl is: %f\n", dubl);

    char *str = NULL;
    get(config, "str", &str);
    printf("str is: %s\n", str);

    bool bl = NULL;
    get(config, "bl", &bl);
    printf("bl is: %s\n", bl ? "true" : "false");


    char **str_arr;
    size_t str_arr_len = 0;
    get_array(config, "arrs", &str_arr, str_arr_len);

    for (int i = 0; i < str_arr_len; ++i) {
        printf("arrs[%d]=%s\n", i, str_arr[i]);
    }

    Option ***opts_arr;
    size_t opts_arr_len = 0;
    get_array(config, "arrc", &opts_arr, opts_arr_len);
    printf("opts_arr length is %zu\n", opts_arr_len);
    for (int i = 0; i < opts_arr_len; ++i) {
        bool *bool_arr;
        size_t bool_len = 0;
        get_array(opts_arr[i], "arrb", &bool_arr, bool_len);

        for (int j = 0; j < bool_len; ++j) {
            printf("arrc[%d].arrb[%d]=%s\n", i, j, bool_arr[j] ? "true" : "false");
        }

        long *arr;
        size_t arr_len = 0;
        get_array(opts_arr[i], "arr", &arr, arr_len);

        for (int j = 0; j < arr_len; ++j) {
            printf("arrc[%d].arr[%d]=%ld\n", i, j, arr[j]);
        }

        double *double_arr;
        size_t double_len = 0;
        get_array(opts_arr[i], "arrd", &double_arr, double_len);

        for (int j = 0; j < double_len; ++j) {
            printf("arrc[%d].arrd[%d]=%f\n", i, j, double_arr[j]);
        }

        int breh = 0;
        get(opts_arr[i], "breh", &breh);
        printf("arrc[%d].breh is: %d\n", i, breh);
    }

    TIMER_END(get);

    TIMER_START(clean);
    cleanOptions(config);
    TIMER_END(clean);
    TIMER_END(config);
}
