# libbgt v0.2 API 文档（图片）

本文档描述 `libbgt` v0.2 新增的图片接口：从文件加载图片，然后以原始尺寸、
缩放、旋转和镜像翻转的方式绘制到窗口。所有函数沿用 v0.1 的基本约定
（`bgt_` 前缀、`snake_case`、整数坐标）。

v0.1 的基础接口（窗口、图形、文本、输入、时间、错误信息）见
[api-v0.md](api-v0.md)。v0.2 计划中的碰撞检测与随机数章节将在后续版本补充，
本文档只描述图片部分。

图片解码由随仓库提供的 SDL3_image 子模块完成，默认静态链接进示例程序，
运行时不需要附带额外的 DLL。完整示例见 `examples/09_images.cpp`。

## 1. 基本约定

图片在 libbgt 中用"图片编号"表示，学生不需要理解纹理或内存管理：

- 编号是一个 `int`，由 `bgt_load_image()` 返回，大于 0 表示有效图片。
- `BGT_IMAGE_NONE`（0）表示"没有图片"，例如加载失败时的返回值。
- 支持的图片格式：PNG、JPG、BMP。
- 图片文件路径按 UTF-8 处理，可以直接使用中文文件名。
- 图片由库统一管理，程序结束时自动释放；没有（也不需要）手动释放的函数。

## 2. 常量

```cpp
BGT_IMAGE_NONE
BGT_FLIP_NONE
BGT_FLIP_HORIZONTAL
BGT_FLIP_VERTICAL
```

### 2.1 图片编号常量

`BGT_IMAGE_NONE` 表示"没有图片"，`bgt_load_image()` 加载失败时返回它。
有效的图片编号一律大于 0，因此判断加载是否成功只需要一次比较：

```cpp
int hero = bgt_load_image("hero.png");

if (hero == BGT_IMAGE_NONE) {
    bgt_print_error();
}
```

### 2.2 翻转常量

`bgt_draw_image_flipped()` 使用以下常量选择镜像方向：

- `BGT_FLIP_NONE`：不翻转，保持原方向。
- `BGT_FLIP_HORIZONTAL`：左右镜像。
- `BGT_FLIP_VERTICAL`：上下镜像。

最典型的用法是让朝向一边的角色精灵转向另一边：

```cpp
int face = BGT_FLIP_NONE;
if (bgt_key_is_down(BGT_KEY_LEFT)) {
    face = BGT_FLIP_HORIZONTAL;
}
bgt_draw_image_flipped(hero, x, y, face);
```

## 3. 图片函数

```cpp
int bgt_load_image(const char filename[]);

int bgt_image_width(int image_id);
int bgt_image_height(int image_id);

void bgt_draw_image(int image_id, int x, int y);
void bgt_draw_image(int image_id, int x, int y, int width, int height);

void bgt_draw_image_rotated(int image_id, int x, int y, double angle);
void bgt_draw_image_rotated(int image_id, int x, int y, int width, int height,
                            double angle);

void bgt_draw_image_flipped(int image_id, int x, int y, int flip);
void bgt_draw_image_flipped(int image_id, int x, int y, int width, int height,
                            int flip);
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
- 应在主循环外加载图片，不要每帧调用（见第 4 节）。
- 同一文件多次加载会得到多个互相独立的编号。
- 图片由库统一管理，程序结束时自动释放，没有手动释放的函数。
- 关闭窗口后所有编号失效，重新打开窗口后需要重新加载。

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

参数：

- `image_id`：图片编号。

返回：

- 图片的原始宽度或原始高度。
- 编号无效时返回 0，不记录错误。

说明：

- 返回值是图片文件自带的像素尺寸，与绘制时是否缩放无关。

示例：

```cpp
int x = (bgt_window_width() - bgt_image_width(hero)) / 2;
int y = (bgt_window_height() - bgt_image_height(hero)) / 2;
bgt_draw_image(hero, x, y);
```

### `bgt_draw_image`

绘制图片，提供两个版本：省略宽高时按原始尺寸绘制，给出宽高时缩放到指定
大小。

```cpp
void bgt_draw_image(int image_id, int x, int y);
void bgt_draw_image(int image_id, int x, int y, int width, int height);
```

参数：

- `image_id`：图片编号。
- `x`、`y`：图片左上角，与 `bgt_fill_rect` 的定位方式一致。
- `width`、`height`：目标宽高，仅缩放版使用。

说明：

- 缩放版在 `width` 或 `height` 小于等于 0 时不绘制，也不记录错误。
- 图片绘制不受 `bgt_set_color()` 影响，画面颜色完全来自图片文件。
- 图片自带的透明部分会正确地与背景混合。
- 编号无效时不绘制，并记录 `BGT_ERROR_IMAGE`。

示例：

```cpp
bgt_draw_image(hero, 30, 30);
bgt_draw_image(hero, 200, 30, 256, 256);
```

### `bgt_draw_image_rotated`

绘制旋转后的图片，提供两个版本：省略宽高时按原始尺寸绘制，给出宽高时先
缩放到指定大小再旋转。

```cpp
void bgt_draw_image_rotated(int image_id, int x, int y, double angle);
void bgt_draw_image_rotated(int image_id, int x, int y, int width, int height,
                            double angle);
```

参数：

- `image_id`：图片编号。
- `x`、`y`：绘制区域左上角。
- `width`、`height`：目标宽高，仅缩放版使用。
- `angle`：旋转角度，单位是度，正值表示顺时针。

说明：

- 旋转中心是绘制区域的中心。
- 缩放版在 `width` 或 `height` 小于等于 0 时不绘制，也不记录错误。
- 编号无效时不绘制，并记录 `BGT_ERROR_IMAGE`。
- 典型用法是用 `bgt_total_time()` 让图片连续旋转。

示例：

```cpp
bgt_draw_image_rotated(hero, 200, 100, 45);

const double angle = bgt_total_time() * 90.0;
bgt_draw_image_rotated(hero, 400, 100, 128, 128, angle);
```

### `bgt_draw_image_flipped`

绘制镜像翻转后的图片，提供两个版本：省略宽高时按原始尺寸绘制，给出宽高时
先缩放到指定大小再翻转。

```cpp
void bgt_draw_image_flipped(int image_id, int x, int y, int flip);
void bgt_draw_image_flipped(int image_id, int x, int y, int width, int height,
                            int flip);
```

参数：

- `image_id`：图片编号。
- `x`、`y`：绘制区域左上角。
- `width`、`height`：目标宽高，仅缩放版使用。
- `flip`：翻转方向，使用 `BGT_FLIP_NONE`、`BGT_FLIP_HORIZONTAL` 或
  `BGT_FLIP_VERTICAL`；其他值按不翻转处理。

说明：

- `BGT_FLIP_HORIZONTAL` 左右镜像，`BGT_FLIP_VERTICAL` 上下镜像。
- 缩放版在 `width` 或 `height` 小于等于 0 时不绘制，也不记录错误。
- 编号无效时不绘制，并记录 `BGT_ERROR_IMAGE`。
- 典型用法是切换角色朝向，见 2.2 节的示例。

示例：

```cpp
bgt_draw_image_flipped(hero, 200, 300, BGT_FLIP_HORIZONTAL);
bgt_draw_image_flipped(hero, 350, 300, 128, 128, BGT_FLIP_VERTICAL);
```

## 4. 教师注记

- 在主循环外加载图片。每帧调用 `bgt_load_image()` 会持续消耗显存，还会为
  同一张图片拿到重复的编号。
- 图片编号在关闭窗口后失效，重新打开窗口后需要重新加载。
- 同一文件多次加载不会去重，每次调用都会得到一个独立编号；需要同一张图片
  的两个独立实例时，这正好符合需求。
- 图片绘制不响应 `bgt_set_color()`；图片的透明度来自文件自身的 alpha 通道。
- 用无效编号绘制会记录错误，因此可以用 `bgt_draw_error()` 把最近错误画在
  窗口里。学生最常见的 bug——加载失败后没有检查返回值就继续绘制——会以
  这种方式显现出来，便于课堂演示。

## 5. 新增常量清单

v0.2 新增的常量：

```cpp
BGT_IMAGE_NONE
BGT_FLIP_NONE
BGT_FLIP_HORIZONTAL
BGT_FLIP_VERTICAL
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
