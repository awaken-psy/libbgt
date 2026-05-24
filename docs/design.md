# libbgt 设计文档

## 1. 项目目标

`libbgt` 是一个面向 C++ 初学者的轻量级 2D 图形库，基于 SDL3 与 SDL3_ttf 实现。

项目目标是为教学早期提供一个足够简单的图形编程入口。学生不需要理解 SDL、渲染器、纹理、字体缓存、事件队列、资源释放和图形学管线，就能创建窗口、绘制图形、显示中文文本并处理简单键盘鼠标输入。

核心目标：

- 使用简单：基础接口只使用普通函数。
- 概念简单：默认只有一个窗口、一张隐式画布、一个主循环。
- 中文友好：随库内置中文字体，默认支持 UTF-8 中文显示。
- 工程可控：项目使用 CMake 构建，SDL3 与 SDL3_ttf 通过 Git Submodule 管理。
- 适合教学：不把高阶封装提前提供给学生，把抽象、封装和设计练习留给课程后半段。
- 可渐进扩展：基础函数式接口稳定后，可以在其上构建现代 C++ 封装层。

## 2. 教学约束

基础版本 API 必须满足：

- 不要求学生定义或使用类与对象。
- 不要求学生定义或使用结构体。
- 不要求学生使用指针。
- 不要求学生手动管理图形资源生命周期。
- 不要求学生理解 SDL 事件、字体、纹理、Surface 或 Renderer。
- 对外参数尽量使用 `int`、`double`、`bool`、颜色常量和字符串字面量。
- 所有基础函数统一使用 `snake_case`。
- 所有基础函数统一使用 `bgt_` 前缀。
- 所有基础常量统一使用 `BGT_` 前缀。

说明：

- 头文件中的字符串参数统一使用 `const char text[]` 这类数组写法，教学文档只展示字符串字面量用法，例如 `"你好"`。
- 库内部可以使用类、结构体、指针、RAII、容器和 SDL 类型，但这些不进入基础 API。
- 后续现代 C++ 封装层可以作为独立接口或课程练习存在，不影响基础 API。

## 3. 最小使用模型

基础版本采用单例式隐式图形环境。

学生只需要理解：

- `bgt_open_window` 创建窗口。
- `bgt_clear_screen` 清空画面。
- `bgt_set_color` 设置当前画笔颜色。
- `bgt_draw_*` 绘制轮廓。
- `bgt_fill_*` 绘制填充图形。
- `bgt_draw_text` 显示中文或英文文字。
- `bgt_key_down`、`bgt_mouse_x`、`bgt_mouse_y` 获取输入。
- `bgt_update_window` 刷新画面并处理事件。
- `bgt_window_is_open` 判断程序是否继续运行。

示例：

```cpp
#include "bgt.h"

int main()
{
    bgt_open_window(800, 600, "移动小球");

    int x = 400;
    int y = 300;

    while (bgt_window_is_open()) {
        if (bgt_key_down(BGT_KEY_LEFT))  x = x - 5;
        if (bgt_key_down(BGT_KEY_RIGHT)) x = x + 5;
        if (bgt_key_down(BGT_KEY_UP))    y = y - 5;
        if (bgt_key_down(BGT_KEY_DOWN))  y = y + 5;

        bgt_clear_screen(BGT_WHITE);
        bgt_set_color(BGT_RED);
        bgt_fill_circle(x, y, 30);
        bgt_update_window();
    }

    bgt_close_window();
    return 0;
}
```

## 4. API 分层

### 4.1 基础函数式接口

面向刚学习变量、条件、循环、函数和数组的学生。

特点：

- 全局函数。
- `bgt_` 前缀。
- `snake_case` 命名。
- 整数颜色常量。
- 整数按键常量。
- 隐式窗口与隐式画布。
- 不暴露指针、结构体、对象或 SDL 类型。

建议文件：

- `include/bgt.h`
- `src/bgt.cpp`

### 4.2 进阶函数式接口

面向已经学习数组、函数拆分、多文件编译和基本项目组织的学生。

可增加：

- 图片资源 ID。
- 简单图片绘制。
- 碰撞检测。
- 随机数。
- 存档读写。

仍然保持 `bgt_` 前缀和不暴露 SDL 类型。

### 4.3 现代 C++ 封装接口

作为高级接口或课程练习。

可包含：

- `bgt::Window`
- `bgt::Color`
- `bgt::Image`
- `bgt::Font`
- `bgt::Vec2`
- RAII 资源管理。
- 枚举类。
- 命名空间。
- 更明确的错误处理模型。

该层不是首版目标。

## 5. 首版范围

首版 `v0.1` 只提供最低必要能力：

- 窗口创建、关闭、刷新。
- 屏幕清空。
- 当前颜色设置。
- 基本几何图形。
- 中文文本绘制。
- 键盘输入。
- 鼠标输入。
- 时间和延时。
- 基础错误信息。

首版明确不提供：

- 网格绘制、单元格填充一类高阶教学封装。
- 图片加载和精灵系统。
- 音频。
- UI 控件。
- 场景管理。
- Tilemap。
- 存档系统。
- 多窗口。
- 相机、视口、坐标变换。

高阶功能不在首版提供的原因：

- 网格、按钮、场景和资源管理适合作为学生练习。
- 首版 API 越小，越容易讲清楚。
- 库只负责降低图形入口门槛，不代替课程中的抽象训练。

## 6. 命名规范

函数命名：

```text
bgt_open_window
bgt_window_is_open
bgt_clear_screen
bgt_set_color
bgt_draw_line
bgt_fill_circle
bgt_draw_text
```

常量命名：

```text
BGT_WHITE
BGT_BLACK
BGT_RED
BGT_KEY_LEFT
BGT_MOUSE_LEFT
```

文件命名：

```text
bgt.h
bgt.cpp
```

内部实现命名不强制暴露给学生，但建议保持一致，避免文档和源码割裂。

## 7. 坐标与颜色模型

### 7.1 坐标

采用屏幕坐标系：

- 左上角为 `(0, 0)`。
- x 轴向右增加。
- y 轴向下增加。
- 所有基础绘图函数使用整数坐标。

### 7.2 颜色

颜色对外表现为整数值。

基础接口提供预定义颜色常量：

```cpp
BGT_BLACK
BGT_WHITE
BGT_RED
BGT_GREEN
BGT_BLUE
BGT_YELLOW
BGT_CYAN
BGT_MAGENTA
BGT_GRAY
BGT_LIGHT_GRAY
BGT_DARK_GRAY
BGT_ORANGE
BGT_PINK
BGT_PURPLE
BGT_BROWN
BGT_TRANSPARENT
```

自定义颜色通过函数创建：

```cpp
int bgt_rgb(int r, int g, int b);
int bgt_rgba(int r, int g, int b, int a);
```

教学时可以先只使用颜色常量，后续再引入 RGB。

## 8. 文本与中文支持

中文显示是本项目的一等需求。

要求：

- 源码字符串按 UTF-8 解释。
- `bgt_draw_text(x, y, "你好", size)` 默认可用。
- 随库内置中文字体。
- 默认字体覆盖常用中文字符。
- 字体加载失败时不崩溃，并提供错误信息。
- 文本渲染基于 SDL3_ttf。

推荐内置字体：

```text
assets/fonts/NotoSansSC-Regular.otf
```

可替代字体：

```text
assets/fonts/SourceHanSansSC-Regular.otf
```

字体查找顺序：

- 用户通过 `bgt_set_font` 指定的字体。
- 随库内置字体。
- 系统中文字体。
- 系统默认 Sans 字体。

系统字体 fallback：

- Windows：Microsoft YaHei、SimHei。
- macOS：PingFang SC。
- Linux：Noto Sans CJK、WenQuanYi。

编码约定：

- 示例文件保存为 UTF-8。
- MSVC 推荐启用 `/utf-8`。
- CMake 应为 MSVC 自动添加 `/utf-8` 编译选项。

## 9. SDL3 封装原则

内部使用 SDL3 负责：

- `SDL_Init`。
- `SDL_Window` 创建与销毁。
- `SDL_Renderer` 创建与销毁。
- 基本图形绘制。
- 事件轮询。
- 键盘状态维护。
- 鼠标状态维护。
- 延时与计时。
- 窗口关闭状态。

基础 API 不暴露：

- `SDL_Window`。
- `SDL_Renderer`。
- `SDL_Texture`。
- `SDL_Surface`。
- `SDL_Event`。
- SDL 指针。
- SDL 错误码。

## 10. SDL3_ttf 封装原则

内部使用 SDL3_ttf 负责：

- `TTF_Init`。
- 字体文件加载。
- 字体大小管理。
- 字体缓存。
- UTF-8 文本渲染。
- 文本尺寸测量。

基础 API 不暴露：

- `TTF_Font`。
- 字体指针。
- glyph。
- surface。
- texture。
- 字体缓存对象。

## 11. CMake 与 Submodule 设计

依赖放置位置：

```text
third_party/SDL
third_party/SDL_ttf
```

初始化方式：

```bash
git submodule update --init --recursive
```

CMake 设计要求：

- 根项目提供 `CMakeLists.txt`。
- 使用 `add_subdirectory(third_party/SDL)`。
- 使用 `add_subdirectory(third_party/SDL_ttf)`。
- 构建静态库或共享库由 CMake 选项控制。
- 示例程序通过 CMake 统一构建。
- MSVC 构建时自动启用 `/utf-8`。
- 构建后复制运行所需 DLL 和字体资源到示例输出目录。

建议 CMake 选项：

```text
BGT_BUILD_EXAMPLES
BGT_BUILD_TESTS
BGT_BUILD_SHARED
BGT_USE_SYSTEM_SDL
```

默认推荐：

- `BGT_BUILD_EXAMPLES=ON`
- `BGT_BUILD_TESTS=OFF`
- `BGT_BUILD_SHARED=OFF`
- `BGT_USE_SYSTEM_SDL=OFF`

## 12. 错误处理

基础 API 不使用异常作为教学前置要求。

错误处理策略：

- 返回 `bool` 表示操作是否成功。
- 可通过 `bgt_has_error()` 查询是否有错误。
- 可通过 `bgt_error_code()` 获取最近错误码。
- 可通过 `bgt_print_error()` 将最近错误打印到控制台。
- 可通过 `bgt_draw_error(x, y, size)` 将最近错误绘制到窗口里。
- 可通过 `bgt_clear_error()` 清除错误状态。

说明：

- 基础 API 不返回错误字符串指针，以满足“不使用指针”的教学约束。
- 首版示例尽量不要求学生主动处理错误，README 和教师文档中说明即可。

## 13. 项目结构

推荐结构：

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

## 14. 示例规划

首批示例应当从低到高排列：

```text
01_hello.cpp
02_shapes.cpp
03_keyboard.cpp
04_mouse.cpp
05_chinese_text.cpp
```

示例原则：

- 每个示例只引入少量新概念。
- 不在示例中过早封装函数。
- 不使用类、结构体或指针。
- 中文示例必须作为首批示例出现。

## 15. 后续扩展方向

可能的 `v0.2` 内容：

- 图片加载。
- 图片绘制。
- 图片缩放。
- 简单碰撞检测。
- 随机数。
- 更多示例小游戏。

可能的 `v0.3` 内容：

- 简单文件存取。
- 声音播放。
- 更完整的错误诊断。
- 自动打包示例。

可能的 `v1.0` 要求：

- API 稳定。
- 中文显示稳定。
- Windows、macOS、Linux 基本可用。
- 文档完整。
- 示例覆盖主要教学场景。

## 16. 非目标

基础版本不追求：

- 3D 图形。
- 着色器。
- 摄像机系统。
- 复杂 UI 框架。
- 物理引擎。
- ECS。
- Tilemap 编辑器。
- 网络功能。
- 多线程渲染。
- 多窗口应用。
- 游戏引擎级性能优化。

## 17. 关键取舍

### 17.1 全局函数优先

基础版本选择全局函数，是为了降低课程前置知识要求。命名冲突风险通过 `bgt_` 前缀控制。

### 17.2 隐式画布优先

学生只需要知道“窗口里有一张画布”。不要求理解 surface、renderer、texture 或 framebuffer。

### 17.3 当前颜色状态优先

图形函数默认使用 `bgt_set_color` 设置的当前颜色，减少每个绘图函数的参数数量。

### 17.4 不提供网格封装

网格绘制、单元格填充、按钮、场景切换等内容有明确教学价值，应当作为学生作业或课堂练习，而不是由库直接提供。

### 17.5 内置中文字体

中文显示不能依赖教学机房环境是否安装字体。随库内置字体会增加体积，但能显著减少配置问题。

## 18. 首版验收标准

`v0.1` 完成时应满足：

- 能用 CMake 完整构建。
- 能从 Submodule 构建 SDL3 和 SDL3_ttf。
- 示例程序能打开窗口。
- 基本图形能正确绘制。
- `bgt_draw_text` 能显示中文。
- 键盘和鼠标输入能工作。
- Windows 下示例源码使用中文字符串不乱码。
- 缺失字体或 SDL 初始化失败时有可读错误信息。
