// libbgt 文件存档 API 的测试（纯函数，无需打开窗口）。
// 用显式检查而非 assert：Release（NDEBUG）下 assert 是空操作，
// 显式检查在任何构建配置下都真正生效。
// 用 -DBGT_BUILD_TESTS=ON 配置后由 ctest 运行。

#include "bgt.h"

#include <cstdio>
#include <cstring>

// NOLINTBEGIN(readability-magic-numbers)

namespace {

int g_failed_checks = 0;

// 显式检查宏：失败时打印表达式与位置，任何构建配置下都生效。
#define BGT_CHECK(condition)                                            \
    do {                                                                \
        if (!(condition)) {                                             \
            std::fprintf(stderr, "FAILED: %s (%s:%d)\n", #condition,    \
                         __FILE__, __LINE__);                           \
            g_failed_checks = g_failed_checks + 1;                      \
        }                                                               \
    } while (false)

bool text_equals(const char *a, const char *b)
{
    return std::strcmp(a, b) == 0;
}

} // namespace

int main()
{
    // 1) 默认值：节或键不存在时返回默认值，且不产生错误。
    bgt_clear_error();
    BGT_CHECK(bgt_get_int("不存在的节", "x", 7) == 7);
    BGT_CHECK(bgt_get_double("不存在的节", "x", 0.5) == 0.5);
    char buffer[16] = {};
    bgt_get_string("不存在的节", "x", buffer, 16, "默认名");
    BGT_CHECK(text_equals(buffer, "默认名"));
    BGT_CHECK(!bgt_has_error());

    // 2) 三类型往返（纯内存）。
    bgt_set_int("玩家", "hp", -3);
    bgt_set_double("进度", "time", 0.1);
    bgt_set_string("玩家", "name", "  张三  "); // 值首尾空白会被去掉
    bgt_set_string("玩家", "note", "a=b 与 空格");
    bgt_set_string("玩家", "empty", "");
    BGT_CHECK(bgt_get_int("玩家", "hp", 0) == -3);
    BGT_CHECK(bgt_get_double("进度", "time", 0.0) == 0.1);
    char name[16] = {};
    bgt_get_string("玩家", "name", name, 16, "?");
    BGT_CHECK(text_equals(name, "张三"));
    char note[24] = {};
    bgt_get_string("玩家", "note", note, 24, "?");
    BGT_CHECK(text_equals(note, "a=b 与 空格"));
    char empty[4] = {};
    bgt_get_string("玩家", "empty", empty, 4, "?");
    BGT_CHECK(text_equals(empty, ""));

    // 3) 覆盖：同一个 (节, 键) 再次 set 会替换旧值（跨类型也替换）。
    bgt_set_int("玩家", "hp", 100);
    BGT_CHECK(bgt_get_int("玩家", "hp", 0) == 100);
    bgt_set_string("玩家", "hp", "甲");
    char swapped[8] = {};
    bgt_get_string("玩家", "hp", swapped, 8, "?");
    BGT_CHECK(text_equals(swapped, "甲"));
    bgt_clear_error();
    BGT_CHECK(bgt_get_int("玩家", "hp", 9) == 9); // “甲”按整数读 → 默认值
    BGT_CHECK(bgt_error_code() == BGT_ERROR_STORAGE);

    // 4) 宽窄转换矩阵：整数可以按小数读，反向不行；字符串拿原文。
    bgt_clear_error();
    bgt_set_int("进度", "count", 120);
    BGT_CHECK(bgt_get_double("进度", "count", 0.0) == 120.0); // 宽转 OK
    BGT_CHECK(!bgt_has_error());
    bgt_set_double("进度", "time2", 45.5);
    BGT_CHECK(bgt_get_int("进度", "time2", 0) == 0); // 窄转不行
    BGT_CHECK(bgt_error_code() == BGT_ERROR_STORAGE);
    bgt_clear_error();
    char raw[8] = {};
    bgt_get_string("进度", "count", raw, 8, "?");
    BGT_CHECK(text_equals(raw, "120"));

    // 5) 跨节同名键互不干扰。
    bgt_set_int("a", "x", 1);
    bgt_set_int("b", "x", 2);
    BGT_CHECK(bgt_get_int("a", "x", 0) == 1);
    BGT_CHECK(bgt_get_int("b", "x", 0) == 2);

    // 6) 名字校验：set 与 get 都执行；空名、含 = 的键、含 [ ] 的节名
    //    都会记错，set 不存入、get 返回默认值。
    bgt_clear_error();
    bgt_set_int("", "x", 1);
    BGT_CHECK(bgt_has_error());
    bgt_clear_error();
    bgt_set_int("节", "bad=key", 1);
    BGT_CHECK(bgt_has_error());
    BGT_CHECK(bgt_get_int("节", "bad=key", 5) == 5); // 没存进去
    bgt_clear_error();
    bgt_set_int("坏[节]", "x", 1);
    BGT_CHECK(bgt_has_error());
    bgt_clear_error();
    BGT_CHECK(bgt_get_int("", "x", 4) == 4); // get 同样校验并记错
    BGT_CHECK(bgt_has_error());
    bgt_clear_error();

    // 7) 截断：放不下时按 UTF-8 边界截断并记错；放得下则完整拷贝。
    bgt_set_string("截断", "long", "张三");
    char small[5] = {}; // 放得下 1 个中文字符 + NUL
    bgt_get_string("截断", "long", small, 5, "?");
    BGT_CHECK(text_equals(small, "张"));
    BGT_CHECK(bgt_has_error());
    bgt_clear_error();
    char fits[8] = {}; // “张三”共 6 字节，加 NUL 放得进 8
    bgt_get_string("截断", "long", fits, 8, "?");
    BGT_CHECK(text_equals(fits, "张三"));
    BGT_CHECK(!bgt_has_error());
    bgt_get_string("截断", "long", nullptr, 0, "?"); // 无处可写 → 仅记错
    BGT_CHECK(bgt_has_error());
    bgt_clear_error();

    if (g_failed_checks > 0) {
        std::fprintf(stderr, "bgt_test_storage: %d checks failed\n",
                     g_failed_checks);
        return 1;
    }
    std::puts("bgt_test_storage: all tests passed");
    return 0;
}

// NOLINTEND(readability-magic-numbers)
