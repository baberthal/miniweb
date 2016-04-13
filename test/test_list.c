#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#include <cmocka.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "cspec_list.h"

static int setup(void **state)
{
    CSList *list = cs_list_create();
    if (list == NULL) {
        return -1;
    }
    *state = list;
    return 0;
}

static int teardown(void **state)
{
    cs_list_destroy(*state);
    return 0;
}

static void test_create(void **state)
{
    CSList *list = *state;
    assert_int_equal(List_count(list), 0);
}

static void test_push_pop(void **state)
{
    CSList *list = *state;
    assert_int_equal(List_count(list), 0);
    char *test1 = "test data 1";
    char *test2 = "test data 2";
    char *test3 = "test data 3";

    cs_list_push(list, test1);
    assert_string_equal(List_last(list), test1);

    cs_list_push(list, test2);
    assert_string_equal(List_last(list), test2);

    cs_list_push(list, test3);
    assert_string_equal(List_last(list), test3);
    assert_int_equal(List_count(list), 3);

    char *val = cs_list_pop(list);
    assert_ptr_equal(val, test3);

    val = cs_list_pop(list);
    assert_ptr_equal(val, test2);

    val = cs_list_pop(list);
    assert_ptr_equal(val, test1);
    assert_int_equal(List_count(list), 0);
}

static void test_unshift(void **state)
{
    CSList *list = *state;
    assert_int_equal(List_count(list), 0);
    char *test1 = "test data 1";
    char *test2 = "test data 2";
    char *test3 = "test data 3";

    cs_list_unshift(list, test1);
    assert_string_equal(List_first(list), test1);
    cs_list_unshift(list, test2);
    assert_string_equal(List_first(list), test2);
    cs_list_unshift(list, test3);
    assert_string_equal(List_first(list), test3);
    assert_int_equal(List_count(list), 3);
}

static int extra_setup(void **state)
{
    CSList *list = cs_list_create();
    char *test1 = "test data 1";
    char *test2 = "test data 2";
    char *test3 = "test data 3";
    if (list == NULL) {
        return -1;
    }

    cs_list_unshift(list, test1);
    cs_list_unshift(list, test2);
    cs_list_unshift(list, test3);
    if (List_count(list) != 3) {
        return -1;
    }

    *state = list;

    return 0;
}

static void test_remove(void **state)
{
    CSList *list = *state;
    assert_int_equal(List_count(list), 3);
    char *val = cs_list_remove(list, list->first->next);
    assert_string_equal(val, "test data 2");
    assert_int_equal(List_count(list), 2);
    assert_string_equal(List_first(list), "test data 3");
    assert_string_equal(List_last(list), "test data 1");
}

static void test_shift(void **state)
{
    CSList *list = *state;
    assert_int_equal(List_count(list), 3);

    char *val = cs_list_shift(list);
    assert_string_equal(val, "test data 3");

    val = cs_list_shift(list);
    assert_string_equal(val, "test data 2");

    val = cs_list_shift(list);
    assert_string_equal(val, "test data 1");
    assert_int_equal(List_count(list), 0);
}

static int malloc_setup(void **state)
{
    CSList *list = cs_list_create();
    if (list == NULL) return -1;

    char *test_data_1 = calloc(12, sizeof(char));
    if (test_data_1 == NULL) return -1;

    char *test_data_2 = calloc(12, sizeof(char));
    if (test_data_2 == NULL) return -1;

    char *test_data_3 = calloc(12, sizeof(char));
    if (test_data_3 == NULL) return -1;

    strcpy(test_data_1, "test data 1");
    strcpy(test_data_2, "test data 2");
    strcpy(test_data_3, "test data 3");

    cs_list_push(list, test_data_1);
    cs_list_push(list, test_data_2);
    cs_list_push(list, test_data_3);

    if (List_count(list) != 3) return -1;

    *state = list;

    return 0;
}

static int malloc_teardown(void **state)
{
    cs_list_clear_destroy(*state);
    return 0;
}

static void test_extras(void **state)
{
    CSList *list = *state;
    assert_int_equal(List_count(list), 3);
}

static void str_apply(void *strp)
{
    int i = 0;
    char *str = (char *)strp;
    while (str[i]) {
        str[i] = toupper(str[i]);
        i++;
    }
}

static void test_apply_f(void **state)
{
    CSList *list = *state;
    assert_int_equal(List_count(list), 3);
    cs_list_apply(list, str_apply);
    assert_string_equal(List_first(list), "TEST DATA 1");
}

static void test_apply_b(void **state)
{
    CSList *list = *state;
    assert_int_equal(List_count(list), 3);
    cs_list_apply_blk(list, ^(void *strp) {
        int i = 0;
        char *str = (char *)strp;
        while (str[i]) {
            str[i] = toupper(str[i]);
            i++;
        }
    });
    assert_string_equal(List_first(list), "TEST DATA 1");
}

static void test_clear(void **state)
{
    CSList *list = *state;
    cs_list_clear(list);
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_create, setup, teardown),
        cmocka_unit_test_setup_teardown(test_push_pop, setup, teardown),
        cmocka_unit_test_setup_teardown(test_unshift, setup, teardown),
        cmocka_unit_test_setup_teardown(test_remove, extra_setup, teardown),
        cmocka_unit_test_setup_teardown(test_shift, extra_setup, teardown),
        cmocka_unit_test_setup_teardown(test_extras,
                                        malloc_setup,
                                        malloc_teardown),
        cmocka_unit_test_setup_teardown(test_apply_f,
                                        malloc_setup,
                                        malloc_teardown),
        cmocka_unit_test_setup_teardown(test_apply_b,
                                        malloc_setup,
                                        malloc_teardown),
        cmocka_unit_test_setup_teardown(test_clear, malloc_setup, teardown),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
