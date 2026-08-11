# libbgt

`libbgt` 是一个面向 C++ 初学者的轻量级 2D 图形库，基于 SDL3 与 SDL3_ttf 构建。

它的目标是在教学早期隐藏 SDL、字体、渲染器、纹理、事件循环和资源生命周期等复杂概念，让学生只使用普通函数、整数、布尔值和字符串字面量，就能完成窗口绘图、中文显示和简单交互程序。

## 设计原则

- 基础接口统一使用 `snake_case`。
- 基础接口统一使用 `bgt_` 前缀。
- 基础接口不要求使用类、对象、结构体或指针。
- 默认只管理一个窗口和一张隐式画布。
- 中文显示开箱可用，随库内置中文字体。
- 项目使用 CMake 构建。
- SDL3 与 SDL3_ttf 通过 Git Submodule 管理。
- SDL3、SDL3_ttf 及其字体依赖默认静态链接，生成的示例程序无需附带 SDL DLL。
- 首版只提供低层、直观的基础能力，不提供网格、场景、按钮等高阶封装。

## 最小示例

```cpp
#include "bgt.h"

int main()
{
    bgt_open_window(800, 600, "我的第一个图形程序");

    while (bgt_window_is_open()) {
        bgt_clear_screen(BGT_WHITE);

        bgt_set_color(BGT_BLUE);
        bgt_fill_circle(400, 300, 50);

        bgt_set_color(BGT_BLACK);
        bgt_draw_text(280, 220, "你好，图形世界！", 32);

        bgt_update_window();
    }

    bgt_close_window();
    return 0;
}
```

## 计划中的项目结构

```text
libbgt/
  CMakeLists.txt
  README.md
  docs/
    design.md
    api-v0.md
  include/
    bgt.h
  src/
    bgt.cpp
  assets/
    fonts/
      NotoSansSC-Regular.otf
  examples/
    01_hello.cpp
    02_shapes.cpp
    03_keyboard.cpp
    04_mouse.cpp
    05_chinese_text.cpp
  third_party/
    SDL/
    SDL_ttf/
```

## 依赖管理

依赖通过 Git Submodule 放在 `third_party/` 下。

```bash
git submodule add https://github.com/libsdl-org/SDL.git third_party/SDL
git submodule add https://github.com/libsdl-org/SDL_ttf.git third_party/SDL_ttf
git submodule update --init --recursive
```

## 构建方式

项目本身使用 CMake 构建。

```bash
cmake -S . -B build
cmake --build build
```

如改用系统安装的依赖，请确保其同时提供静态 CMake 目标，然后配置
`-DBGT_USE_SYSTEM_SDL=ON`。

当前仓库包含首版基础 API 实现、CMake 构建脚本和一个最小示例。中文字体文件由 `assets/fonts/NotoSansSC-Regular.otf` 提供，仓库使用者需要确保该文件存在。

## 文档

- [设计文档](docs/design.md)
- [首版 API 文档](docs/api-v0.md)

## 首版范围

首版 `v0.1` 目标是提供：

- 窗口创建、关闭与刷新。
- 基本图形绘制。
- UTF-8 中文文本绘制。
- 键盘输入。
- 鼠标输入。
- 时间与帧率辅助。
- 基础错误信息。

首版入口文件：

- `include/bgt.h`
- `src/bgt.cpp`
- `examples/01_hello.cpp`

首版暂不提供：

- 图片加载。
- 音频。
- 网格工具。
- UI 控件。
- 场景管理。
- Tilemap。
- 精灵系统。
- 多窗口。
- 面向对象封装。

## 教学定位

`libbgt` 不试图成为完整游戏引擎。它是一个教学用图形入口，重点是让学生尽早看到图形反馈，并把更高层次的封装留作后续课程练习。
