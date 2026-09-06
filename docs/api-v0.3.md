# libbgt v0.3 API 文档（文件存档、声音播放、错误诊断）

本文档描述 `libbgt` v0.3 新增的三块接口：文件存档（把游戏数据保存成
记事本能直接打开的文本文件）、声音播放（音效和背景音乐）与错误诊断
（错误历史与按序号查询）。存档见 §1–§6，声音见 §7–§9，错误诊断见
§10–§12。

v0.1 的基础接口见 [api-v0.md](api-v0.md)，v0.2 的图片、随机数与碰撞检测
接口见 [api-v0.2.md](api-v0.2.md)。存档完整示例见
`examples/12_storage.cpp`，声音完整示例见 `examples/13_sound.cpp`，错误
诊断完整示例见 `examples/14_errors.cpp`。

## 10. 错误历史模型

从 v0.3 起，库把最近发生的错误记成一份编号列表：

- 最多保留 **10 条**：更早的错误会被最老的挤出。
- 序号 **0 是最老的一条**，`bgt_error_count() - 1` 是最新的一条。
- `bgt_clear_error()` 清空整份历史。
- 错误码沿用 v0.1 的 `BGT_ERROR_*` 常量，没有新增。

三条查询铁律（实现与文档都以此为准）：

1. **查询不产生新错误**：`bgt_error_count()`、`bgt_error_code()`、
   `bgt_error_text()`、`bgt_has_error()` 任何情况都不会往历史里新增
   条目——查历史这个动作本身永远干净。
2. **越界静默**：序号越界（含负数、含历史为空）时，`bgt_error_code()`
   返回 0，`bgt_error_text()` 得到空串，`bgt_print_error()` 和
   `bgt_draw_error()` 什么都不做。
3. **截断静默**：`bgt_error_text()` 的数组放不下时按 UTF-8 字符边界
   安全截断（不会切在半个汉字中间），不记错误。

## 11. 函数清单

| 函数 | 说明 |
|---|---|
| `bool bgt_has_error()` | 历史里有至少一条错误时返回 true |
| `int bgt_error_count()` | 历史条数（0 到 10） |
| `int bgt_error_code(int index)` | 按序号取错误码；越界返回 `BGT_ERROR_NONE` |
| `int bgt_error_code()` | 最新一条的错误码；等价于 `bgt_error_code(bgt_error_count() - 1)` |
| `void bgt_error_text(int index, char out[], int out_size)` | 按序号把消息文本复制进 out 数组；放不下时按 UTF-8 边界截断 |
| `void bgt_print_error(int index)` | 按序号打印到控制台：`libbgt error 码: 消息` |
| `void bgt_print_error()` | 打印最新一条；v0.1 的无参用法保持不变 |
| `void bgt_draw_error(int x, int y, int size, int index)` | 按序号绘制；消息太长自动按窗口宽度换行，逐行向下画 |
| `void bgt_draw_error(int x, int y, int size)` | 绘制最新一条；v0.1 的无参用法保持不变 |
| `void bgt_clear_error()` | 清空整份历史 |

无参的三个形态（`bgt_error_code()`、`bgt_print_error()`、
`bgt_draw_error(x, y, size)`）是 v0.1 的老用法，内部就是取最新一条，
老程序不用改。

## 12. 教学建议

- **画整份历史就是三行循环**：

```cpp
for (int i = 0; i < bgt_error_count(); i = i + 1) {
    bgt_draw_error(40, 100 + i * 48, 16, i);
}
```

- 学生的游戏可以在角落常驻这样一个小面板，运行中出了什么错一眼可见；
  想按错误类型做处理，用 `bgt_error_code(i)` 循环统计。
- `bgt_error_text()` 把文本取进自己的数组后想怎么显示都可以——适合做
  "错误详情页"这类练习。
- 手改存档（见 §4）会产生连串读档错误时，历史列表正好用来观察"每一行
  坏行都被记了一条"。
