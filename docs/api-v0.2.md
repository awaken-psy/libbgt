# libbgt v0.2 API 文档（图片）

本文档描述 `libbgt` v0.2 新增的图片接口：从文件加载图片，并通过设置
每张图片的 RST 变换状态（RST = 平移-旋转-缩放）来控制它在画面上的样子。
所有函数沿用 v0.1 的基本约定（`bgt_` 前缀、`snake_case`、整数坐标）。

v0.1 的基础接口（窗口、图形、文本、输入、时间、错误信息）见
[api-v0.md](api-v0.md)。v0.2 计划中的碰撞检测与随机数章节将在后续版本补充，
本文档只描述图片部分。

图片解码由随仓库提供的 SDL3_image 子模块完成，默认静态链接进示例程序，
运行时不需要附带额外的 DLL。完整示例见 `examples/09_images.cpp`。

## 1. RST 变换模型

libbgt 的图片变换遵循图形学中经典的 RST 模型。每张图片有三个独立的
变换状态，绘制时自动合成：

```text
M = T × R × S
    │   │   │
    │   │   └─ 缩放：bgt_scale_image(id, sx, sy)
    │   └─ 旋转：bgt_rotate_image(id, angle)
    └─ 平移：bgt_translate_image(id, dx, dy) + 绘制位置 (x, y)
```

变换按 **先缩放（S），再旋转（R），最后平移（T）** 的固定顺序合成，
全部围绕图片的**局部原点**（左上角）进行。

三个核心事实：

1. **旋转和缩放围绕左上角**——变换后左上角位置不变，图片绕它伸缩转动。
2. **翻转就是负缩放**——`S(-1, 1)` 左右翻转，`S(1, -1)` 上下翻转。
   没有独立的翻转函数。
3. **变换是图片的属性**——设置一次持续生效，直到再次修改或清除。
   这与 `bgt_set_color()` 的工作方式相同。

翻转时图片会翻到原点的**另一侧**：左右翻转后图片出现在原点左侧，
上下翻转后图片出现在原点上方。这是严格 RST 的正确行为——局部原点始终
不动，变的是图片"身体"相对于原点的方向。

## 2. 基本约定

图片在 libbgt 中用"图片编号"表示，学生不需要理解纹理或内存管理：

- 编号是一个 `int`，由 `bgt_load_image()` 返回，大于 0 表示有效图片。
- `BGT_IMAGE_NONE`（0）表示"没有图片"，例如加载失败时的返回值。
- 支持的图片格式：PNG、JPG、BMP。
- 图片文件路径按 UTF-8 处理，可以直接使用中文文件名。
- 图片由库统一管理，程序结束时自动释放；没有（也不需要）手动释放的函数。
- 变换状态（T、R、S）也是图片的一部分：每个编号独立记忆自己的变换，
  互不影响。

## 3. 图片函数

```cpp
int bgt_load_image(const char filename[]);

int bgt_image_width(int image_id);
int bgt_image_height(int image_id);

void bgt_translate_image(int image_id, int dx, int dy);
void bgt_rotate_image(int image_id, double angle);
void bgt_scale_image(int image_id, double sx, double sy);
void bgt_clear_image_transform(int image_id);

void bgt_draw_image(int image_id, int x, int y);
```

### `bgt_load_image`

从文件加载一张图片，返回它的图片编号。

```cpp
int bgt_load_image(const char filename[]);
```

参数：

- `filename`：图片文件路径，按 UTF-8 处理。

返回：

- 成功返回一个大于 0 的图片编号。
- 失败返回 `BGT_IMAGE_NONE`，可用 `bgt_print_error()` 查看原因。

说明：

- 支持 PNG、JPG、BMP 三种常见格式。
- 窗口尚未打开时调用会失败并记录 `BGT_ERROR_NOT_OPEN`。
- 文件名为空、文件不存在或解码失败时记录 `BGT_ERROR_IMAGE`。
- 应在主循环外加载图片，不要每帧调用（见第 5 节）。
- 同一文件多次加载会得到多个互相独立的编号（包括各自的变换状态）。
- 图片由库统一管理，程序结束时自动释放，没有手动释放的函数。
- 关闭窗口后所有编号失效，重新打开窗口后需要重新加载。
- 新加载的图片变换状态为恒等（无旋转、无缩放、无偏移）。

示例：

```cpp
int hero = bgt_load_image("hero.png");

if (hero == BGT_IMAGE_NONE) {
    bgt_print_error();
    return 1;
}
```

### `bgt_image_width` / `bgt_image_height`

返回图片的原始尺寸，单位是像素。

```cpp
int bgt_image_width(int image_id);
int bgt_image_height(int image_id);
```

返回：

- 图片的原始宽度或原始高度。
- 编号无效时返回 0，不记录错误。

说明：

- 返回值是图片文件自带的像素尺寸，不受缩放状态影响。
- 想知道缩放后的显示尺寸：`bgt_image_width(hero) * 2`（放大 2 倍时）。

示例：

```cpp
int x = (bgt_window_width() - bgt_image_width(hero)) / 2;
int y = (bgt_window_height() - bgt_image_height(hero)) / 2;
bgt_draw_image(hero, x, y);
```

### `bgt_translate_image`（T：平移）

设置图片相对绘制位置的偏移量。

```cpp
void bgt_translate_image(int image_id, int dx, int dy);
```

参数：

- `image_id`：图片编号。
- `dx`、`dy`：相对绘制位置的偏移量。

说明：

- 实际平移位置 = `bgt_draw_image` 的 `(x, y)` + 这里的 `(dx, dy)`。
- 图片的局部原点（左上角）最终落在 `(x + dx, y + dy)`。
- 默认 `(0, 0)`（无偏移）。
- 编号无效时不修改并记录 `BGT_ERROR_IMAGE`。
- 适合做弹跳、后坐力等"逻辑位置不变、视觉位置微调"的动画。

示例：

```cpp
// 弹跳动画：逻辑位置 (400, 300) 不变，图片上下浮动
double t = bgt_total_time() * 3.0;
int bounce = static_cast<int>(std::sin(t) * 20.0);
bgt_translate_image(hero, 0, bounce);
bgt_draw_image(hero, 400, 300);
```

### `bgt_rotate_image`（R：旋转）

设置图片的旋转角度。

```cpp
void bgt_rotate_image(int image_id, double angle);
```

参数：

- `image_id`：图片编号。
- `angle`：旋转角度，单位是度，正值表示顺时针。

说明：

- 旋转围绕图片的局部原点（左上角）进行——旋转后左上角位置不变，
  图片整体绕它转动。
- 默认 0（不旋转）。
- 编号无效时不修改并记录 `BGT_ERROR_IMAGE`。
- 想让图片"绕自己中心转"需要平移补偿，属于后续课程的话题。

示例：

```cpp
// 风车：每秒转 90 度
bgt_rotate_image(pinhweel, bgt_total_time() * 90.0);
bgt_draw_image(pinhweel, 400, 200);
```

### `bgt_scale_image`（S：缩放）

设置图片的缩放倍率。负值表示翻转。

```cpp
void bgt_scale_image(int image_id, double sx, double sy);
```

参数：

- `image_id`：图片编号。
- `sx`：横向缩放因子。`1.0` 原始大小、`2.0` 放大一倍、`0.5` 缩小一半。
- `sy`：纵向缩放因子，含义同上。

说明：

- 缩放围绕图片的局部原点（左上角）——缩放后左上角位置不变，
  图片向右下方向伸缩。
- **负值表示翻转**：`sx < 0` 时左右翻转（图片翻到原点左侧），
  `sy < 0` 时上下翻转（图片翻到原点上方）。
- `S(-1, 1)` 左右翻转、`S(1, -1)` 上下翻转、`S(-1, -1)` 等价于旋转
  180 度。
- 翻转后图片的实际显示位置会移动到原点的另一侧，这是严格 RST 的
  正确行为——局部原点不动，图片的身体翻到另一边。
- 默认 `(1.0, 1.0)`（原始大小）。
- 编号无效时不修改并记录 `BGT_ERROR_IMAGE`。

示例：

```cpp
bgt_scale_image(hero, 2.0, 2.0);      // 放大一倍
bgt_draw_image(hero, 100, 100);

bgt_scale_image(hero, -1.0, 1.0);     // 左右翻转（翻到原点左侧）
bgt_draw_image(hero, 100, 100);

bgt_scale_image(hero, 1.0, 1.0);      // 恢复原始大小
```

### `bgt_clear_image_transform`

清除图片的全部变换状态，恢复为恒等变换。

```cpp
void bgt_clear_image_transform(int image_id);
```

说明：

- 一次调用等同 `bgt_translate_image(id, 0, 0)` +
  `bgt_rotate_image(id, 0)` + `bgt_scale_image(id, 1.0, 1.0)`。
- 清除后图片回到刚加载时的样子。
- 编号无效时不修改并记录 `BGT_ERROR_IMAGE`。

示例：

```cpp
bgt_clear_image_transform(hero);
bgt_draw_image(hero, 100, 100);   // 和刚加载时一模一样
```

### `bgt_draw_image`

在指定位置绘制图片，应用该图当前的全部变换状态。

```cpp
void bgt_draw_image(int image_id, int x, int y);
```

参数：

- `image_id`：图片编号。
- `x`、`y`：基准位置。图片的局部原点（左上角）最终落在
  `(x + dx, y + dy)`，其中 `(dx, dy)` 是 `bgt_translate_image` 设置的
  偏移。

说明：

- 变换的合成顺序固定为：先缩放（S），再旋转（R），最后平移（T）。
- 图片绘制不受 `bgt_set_color()` 影响，画面颜色完全来自图片文件。
- 图片自带的透明部分会正确地与背景混合。
- 编号无效时不绘制，并记录 `BGT_ERROR_IMAGE`。

示例：

```cpp
bgt_draw_image(hero, 100, 100);               // 应用当前变换
bgt_draw_image(hero, 300, 100);               // 同一编号、同一变换、不同位置
```

## 4. 变换的组合顺序

当 S、R、T 同时生效时，合成的数学含义是：

```text
M = T(x + dx, y + dy) × R(angle) × S(sx, sy)

v_screen = M × v_local    （v_local 是图片的局部顶点）
```

每个顶点按以下步骤变换：

1. **S（缩放）**：`v_scaled = (lx × sx, ly × sy)`——左上角不动，
   图片伸缩或翻转。
2. **R（旋转）**：`v_rotated = rotate(v_scaled, angle)`——绕左上角转。
3. **T（平移）**：`v_screen = v_rotated + (x + dx, y + dy)`——搬到
   最终位置。

因为顺序固定，所以改变缩放不会影响旋转角，改变旋转角不会影响缩放倍率。
三个状态完全正交，学生只需要理解"每张图有三个属性"。

## 5. 教师注记

- 在主循环外加载图片。每帧调用 `bgt_load_image()` 会持续消耗显存，还会为
  同一张图片拿到重复的编号。
- 图片编号在关闭窗口后失效，重新打开窗口后需要重新加载。
- 同一文件多次加载不会去重，每次调用都会得到一个独立编号——包括独立的
  变换状态。需要同一张图的"原始版"和"翻转版"同时显示时，加载两次即可。
- 变换状态是"图片的属性"这一设计与 `bgt_set_color()` 一致——设置后持续
  生效，直到修改或清除。可以借此强化"状态"这一课程核心概念。
- 翻转（负缩放）会导致图片显示位置移到原点另一侧。如果需要"原地翻转"，
  学生可以在翻转时同时平移 `translate_image(id, image_width, 0)`——
  这本身是一个好的课堂练习（理解负缩放的位置影响）。
- 旋转围绕左上角（严格 RST），不是图片中心。学生转 45 度会看到图片
  "甩出去"。这与后续课程讲 OpenGL 变换矩阵时的行为完全一致——属于
  刻意安排的知识铺垫。
- 用无效编号绘制会记录错误，因此可以用 `bgt_draw_error()` 把最近错误画在
  窗口里。学生最常见的 bug——加载失败后没有检查返回值就继续绘制——会以
  这种方式显现出来，便于课堂演示。

## 6. 新增常量清单

v0.2 新增的常量：

```cpp
BGT_IMAGE_NONE
```

v0.2 新增的错误码是 `BGT_ERROR_IMAGE`，追加在 v0.1 错误码序列的末尾：

```cpp
BGT_ERROR_NONE
BGT_ERROR_SDL
BGT_ERROR_TTF
BGT_ERROR_WINDOW
BGT_ERROR_RENDERER
BGT_ERROR_FONT
BGT_ERROR_NOT_OPEN
BGT_ERROR_IMAGE
```

错误码可通过 `bgt_error_code()` 读取。面向学生的示例通常只需要使用
`bgt_print_error()`。
