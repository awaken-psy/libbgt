# libbgt v0.3 API 文档（文件存档）

本文档描述 `libbgt` v0.3 新增的文件存档接口：把游戏数据保存成普通文本
文件，关掉程序再重新运行时读回来。存档由"节-键-值"三层组成，文件就是
记事本能直接打开的 INI 风格文本——学生能亲眼看到自己存的档，也能手改。

所有函数沿用既有约定（`bgt_` 前缀、`snake_case`、整数参数）。

v0.1 的基础接口见 [api-v0.md](api-v0.md)，v0.2 的图片、随机数与碰撞检测
接口见 [api-v0.2.md](api-v0.2.md)。存档完整示例见
`examples/12_storage.cpp`。

## 1. 存档模型：节-键-值

内存里有一张存档表：**节名 → 键名 → 值**。

```cpp
bgt_set_int("最高分", "best", 120);      // 往 [最高分] 节里放一个键
bgt_save("save.txt");                    // 整表写进文件
```

```cpp
bgt_load("save.txt");                    // 文件整表读进内存
int best = bgt_get_int("最高分", "best", 0);   // 取不出来时给默认值
```

三个动词分工：`bgt_load`/`bgt_save` 负责**文件**，`bgt_set_*`/`bgt_get_*`
负责**内存表**。存档一词的本义就是"收集状态，写盘；下次运行，读回"。

- `bgt_load("save.txt")` 文件不存在 → 空表、不报错（第一次运行的常态）。
- `bgt_get_*` 节或键不存在 → 返回默认值，不报错。默认值是 API 的一部分。
- `bgt_set_*` 同一个 (节, 键) 再写一次就覆盖（跨类型也覆盖）。

## 2. 函数清单

### 2.1 文件

| 函数 | 说明 |
|---|---|
| `bool bgt_load(const char filename[])` | 把存档文件整表读进内存（替换原内存表） |
| `bool bgt_save(const char filename[])` | 把内存表整表写进文件（先写临时文件再替换，防写坏） |
| `bool bgt_file_exists(const char filename[])` | 判断文件是否存在，常用于首次运行检测 |

### 2.2 写入（内存表）

| 函数 | 说明 |
|---|---|
| `void bgt_set_int(const char section[], const char key[], int value)` | 存整数 |
| `void bgt_set_double(const char section[], const char key[], double value)` | 存小数 |
| `void bgt_set_string(const char section[], const char key[], const char value[])` | 存字符串（首尾空白会被去掉；去掉首尾空白后仍含换行会记错不存入） |

### 2.3 读取（内存表）

| 函数 | 说明 |
|---|---|
| `int bgt_get_int(const char section[], const char key[], int default_value)` | 读整数 |
| `double bgt_get_double(const char section[], const char key[], double default_value)` | 读小数 |
| `void bgt_get_string(const char section[], const char key[], char out[], int out_size, const char default_value[])` | 读字符串进 `out` 数组 |

`bgt_get_string` 的用法：

```cpp
char name[32];
bgt_get_string("玩家", "name", name, 32, "无名");   // 没有这个键时是 "无名"
```

字符串放不进 `out_size` 时只写入放得下的部分（保证不切半个中文）并记录
错误——看到错误就把数组开大。

## 3. 类型转换

存档表里存的是值的**文本**。读取时按类型解析：

| 存入的值 | 按 int 读 | 按 double 读 | 按字符串读 |
|---|---|---|---|
| `100`（整数） | `100` | `100.0` ✅ 整数可以当小数用 | `"100"` |
| `45.5`（小数） | ❌ 默认值+错误 | `45.5` | `"45.5"` |
| `张三`（字符串） | ❌ 默认值+错误 | ❌ 默认值+错误 | `张三` |

窄方向读不出来（给默认值并记录错误）；宽方向自然成立，与 C++ 里
"整数能赋给 double"的直觉一致。

## 4. 存档文件格式

`bgt_save("save.txt")` 写出的是纯文本（UTF-8，行尾 `\n`）：

```ini
[最高分]
best=120

[玩家]
name=张三

[盘面]
board=53..7....
```

- 每行 `键=值`；`[节名]` 单独一行；空行分隔各节；`#` 开头的整行是注释
  （库不写注释，但读的时候会跳过——学生可以在自己存档里写备注）。
- 节和键都按字典序输出，同一文件每次保存结果完全一样。
- 小数用最短表示：`45.5` 存 `45.5`，`0.1` 存 `0.1`，读回精确不丢。
- 手改时可用 `键 = 值` 带空格（读取时自动去掉首尾空白）；行尾是
  Windows 记事本的 `\r\n` 也没问题；记事本另存为 UTF-8 带的文件头
  标记（BOM）也能读。
- 格式不对的行会被跳过并记录错误，其他行照常读入——改坏了能发现，
  但整个存档不会因此作废。
- 节名不能含 `[`、`]`；键名不能含 `=`，且**不能以 `#` 或 `[` 开头**（这样的键写进文件后会读不回来）；字符串值去掉首尾空白后仍不能含换行。

## 5. 错误处理

存档错误统一使用错误码 `BGT_ERROR_STORAGE`，可用 `bgt_has_error()` /
`bgt_error_code()` / `bgt_print_error()` 查询（用法与 v0.1 相同）：

| 场景 | 行为 |
|---|---|
| `bgt_load` 文件不存在 | 空表，**无错误**（首次运行常态） |
| `bgt_load` 有格式不对的行 | 跳过该行，其余照常，记一次错误，返回 `false` |
| `bgt_get_*` 节或键不存在 | 返回默认值，**无错误** |
| 按窄类型读（小数按 int 读等） | 返回默认值 + 记错误 |
| `bgt_get_string` 放不下 | 截断到完整字符 + 记错误 |
| `bgt_set_*` 节名/键名不合法 | 记错误，不存入 |
| `bgt_save` 写失败 | 返回 `false` + 记错误 |

## 6. 教学建议

- **先手写存档，再写程序读它**：存档格式就是文本 INI——可以让学生在
  记事本里手写一个存档（比如 `[玩家] hp=80`），再写程序 `bgt_load` +
  `bgt_get_int` 读出来，直观建立"文件-内存表"的映射。
- **对拍练习**：可以让学生先用 `bgt_file_exists` + 屏幕提示实现
  "第一次运行向导"，再对比自己手写的 if/else 版本。
- **错误是可见的**：手改存档改坏一行，程序里 `bgt_load` 返回 false、
  `bgt_print_error` 能打印原因——这是讲"数据校验"的好素材。
