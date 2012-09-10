#include "config_parse.h"
#include <stdarg.h>
#include <setjmp.h>
#include <cmockery.h>

//config_rw_tool *p = create_config_tool(NULL);
config_rw_tool *p;
char buf[100];

void test_init_and_reload(void **state)
{
    config_property pt = {SECTION_DEFAULT, SEPARATOR_DEFAULT, COMMENT_DEFAULT};
    assert_int_equal(p->init(p, "test.conf", &pt, NULL), CFG_TRUE);
    assert_int_equal(p->init(p, "test.conf", &pt, NULL), CFG_TRUE);
    assert_int_equal(p->load(p, NULL), CFG_TRUE);
}

void test_about_item(void **state)
{
    struct list_head *itemlist;
    config_item *item;
    int i = 0;
    assert_int_equal(p->get_item(p, NULL, "tangfu", buf, 100), CFG_TRUE);
    assert_string_equal(buf, "192.168.1.1");
    assert_int_equal(p->get_item(p, NULL, "name", buf, 100), CFG_TRUE);
    assert_string_equal(buf, "tangfuqiang");
    assert_int_equal(p->get_item_cnt(p, NULL), 3);
    assert_int_equal(p->get_item_cnt(p, "12"), 1);
    assert_int_equal(p->get_item_cnt(p, "23"), 2);
    itemlist = p->get_item_list_self(p, NULL);
    assert_true(itemlist == NULL);
    itemlist = p->get_item_list_self(p, "23");
    assert_true(itemlist != NULL);
    list_for_each_entry(item, itemlist, list) {
        if(i == 1) {
            assert_string_equal(item->item, "name");
            assert_string_equal(item->value, "tangfuqiang");
        } else if(i == 2) {
            assert_string_equal(item->item, "server");
            assert_string_equal(item->value, "172.24.0.104");
        } else {
            ;
        }
    }
}

void test_about_section(void **state)
{
    struct list_head *seclist;
    config_section *section;
    int i = 0;
    assert_int_equal(p->get_section_cnt(p), 3);
    seclist = p->get_section_list_self(p);
    assert_true(seclist != NULL);
    list_for_each_entry(section, seclist, list) {
        if(i == 1) {
            assert_string_equal(section->section, "12");
        } else if(i == 2) {
            assert_string_equal(section->section, "23");
        } else if(i == 3) {
            assert_string_equal(section->section, "q2");
        } else {
            ;
        }
    }
}

int main(int argc, char *argv[])
{
    p = create_config_tool(NULL);
    UnitTest tests[] = {
        unit_test(test_init_and_reload),
        unit_test(test_about_item),
        unit_test(test_about_section)
    };
    return run_tests(tests);
}
