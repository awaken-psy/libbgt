// =====================================================================
// libbgt 图片 API 演示 —— 严格 RST 变换模型（v0.2）
//
// 每个板块介绍一小块图片 API，按【空格】进入下一板块，随时可以按
// 【Esc】退出。图片变换遵循图形学经典 RST 模型（RST = 平移-旋转-缩放）：
//
//   M = T * R * S
//
// 变换按 S → R → T 的顺序合成，全部围绕图片的局部原点（左上角）。
// 翻转没有独立函数——负缩放就是翻转。
//
//   板块 1  加载与基本信息
//   板块 2  绘制基础
//   板块 3  S — 缩放
//   板块 4  S — 翻转（负缩放）
//   板块 5  R — 旋转
//   板块 6  T — 平移
//   板块 7  RST 组合与清除
//   板块 8  错误路径
//
// 写作约定（与 08_api_tour.cpp 一致）：
//   - 不出现类、结构体、指针、std::string；
//   - 常量用 const int 声明（本文件刻意不使用 constexpr）。
// =====================================================================

#include "bgt.h"

#include <cmath>
#include <cstdio>

// NOLINTBEGIN(readability-magic-numbers, readability-identifier-length,
// readability-function-cognitive-complexity)

int main()
{
    // ---------- 常量 ----------
    const int window_width = 1000;   // 窗口宽度
    const int window_height = 700;   // 窗口高度
    const int target_fps = 60;      // 帧率上限

    if (!bgt_open_window(window_width, window_height,
                         "libbgt 图片 API 演示")) {
        bgt_print_error();
        return 1;
    }
    bgt_set_fps_limit(target_fps);

    // ---------- 加载图片（在主循环外，只做一次）----------
    // 测试图 09_image.png 是 128x128 的四象限图：
    //   左上=不透明红   右上=半透明蓝   左下=完全透明   右下=绿色圆
    //   中心有一个白色小圆点
    // 不对称的象限让我们能一眼看出翻转和旋转的方向。
    const int img = bgt_load_image("09_image.png");

    // ---------- 各板块需要的"跨帧状态"（就是普通变量）----------

    int part = 1;                    // 当前板块号：1 到 8

    // 板块 5（旋转）使用：按 ←/→ 调角度
    double demo_angle = 0.0;

    // 板块 6（平移）使用：弹跳动画的进度
    double bounce_t = 0.0;

    // 板块 8（错误）使用
    bool error_printed = false;

    // ---------- 主循环 ----------
    while (bgt_window_is_open()) {

        // 随时退出
        if (bgt_key_just_pressed(BGT_KEY_ESCAPE)) {
            bgt_close_window();
            break;
        }

        // 加载失败时：所有板块都显示错误提示
        if (img == BGT_IMAGE_NONE) {
            bgt_set_background(BGT_WHITE);
            bgt_set_color(BGT_RED);
            bgt_draw_text(40, 40, "图片加载失败：", 24);
            bgt_draw_error(40, 80, 20);
            bgt_set_color(BGT_DARK_GRAY);
            bgt_draw_text(40, 120,
                          "请从 build 目录运行本程序（09_image.png 在 exe 旁"
                          "边）。", 18);
            bgt_update_window();
            continue;
        }

        if (part == 1) {
            // =====================================================
            // 板块 1：加载与基本信息
            // =====================================================
            bgt_set_background(bgt_rgb(250, 250, 250));
            bgt_set_window_title("libbgt 图片演示 - 板块 1：加载与基本信息");

            bgt_set_color(BGT_BLACK);
            bgt_draw_text(40, 36, "板块 1：加载与基本信息", 32);
            bgt_draw_text(40, 94,
                          "bgt_load_image(文件名) 加载图片，返回一个大于 0 的"
                          "编号。", 20);
            bgt_draw_text(40, 128,
                          "加载失败时返回 BGT_IMAGE_NONE(0)，可用 "
                          "bgt_print_error() 查看原因。", 20);
            bgt_draw_text(40, 162,
                          "请在主循环外加载：同一文件多次加载得到多个独立"
                          "编号。", 20);
            bgt_draw_text(40, 196,
                          "关闭窗口后所有编号失效，需要重新加载。", 20);

            // 显示图片的原始尺寸
            char size_text[96];
            std::snprintf(size_text, sizeof(size_text),
                          "bgt_image_width(%d) = %d    "
                          "bgt_image_height(%d) = %d",
                          img, bgt_image_width(img), img,
                          bgt_image_height(img));
            bgt_set_color(BGT_BLUE);
            bgt_draw_text(40, 260, size_text, 20);

            // 原样绘制（板块 2 详细讲）
            bgt_draw_image(img, 80, 320);
            bgt_set_color(BGT_BLACK);
            bgt_draw_text(240, 320,
                          "这就是刚加载的图（bgt_draw_image 原样绘制）。", 18);
            bgt_draw_text(240, 350,
                          "左上红色象限、右上半透明蓝、右下绿色圆、"
                          "左下完全透明。", 18);

        } else if (part == 2) {
            // =====================================================
            // 板块 2：绘制基础
            // =====================================================
            bgt_set_background(bgt_rgb(250, 250, 250));
            bgt_set_window_title("libbgt 图片演示 - 板块 2：绘制基础");

            bgt_set_color(BGT_BLACK);
            bgt_draw_text(40, 36, "板块 2：绘制基础", 32);
            bgt_draw_text(40, 94,
                          "bgt_draw_image(编号, x, y)：在 (x, y) 绘制图片。",
                          20);
            bgt_draw_text(40, 128,
                          "(x, y) 是图片的局部原点——即图片的左上角。",
                          20);
            bgt_draw_text(40, 162,
                          "同一张图片可以画在多个位置，编号不用重复加载。",
                          20);

            // 画三个副本，展示 (x, y) 控制左上角位置
            bgt_draw_image(img, 80, 220);
            bgt_draw_image(img, 300, 220);
            bgt_draw_image(img, 520, 220);

            // 用十字标记中间那张图的左上角 = 局部原点
            bgt_set_color(BGT_RED);
            bgt_draw_line(290, 220, 310, 220);
            bgt_draw_line(300, 210, 300, 230);
            bgt_set_color(BGT_BLACK);
            bgt_draw_text(240, 380,
                          "红色十字标记了中间那张图的局部原点 (300, 220)：",
                          18);
            bgt_draw_text(240, 410,
                          "改变 (x, y) 就是把左上角搬到新位置。", 18);

            bgt_draw_text(40, 480,
                          "小知识：图片的透明部分会正确地与背景混合，"
                          "绘制不受 bgt_set_color 影响。", 18);

        } else if (part == 3) {
            // =====================================================
            // 板块 3：S — 缩放
            // =====================================================
            bgt_set_background(bgt_rgb(250, 250, 250));
            bgt_set_window_title("libbgt 图片演示 - 板块 3：S — 缩放");

            bgt_set_color(BGT_BLACK);
            bgt_draw_text(40, 36, "板块 3：S — 缩放", 32);
            bgt_draw_text(40, 94,
                          "bgt_scale_image(编号, sx, sy) 设置缩放倍率：",
                          20);
            bgt_draw_text(40, 128,
                          "  1.0 = 原始大小    2.0 = 放大一倍    "
                          "0.5 = 缩小一半", 20);
            bgt_draw_text(40, 162,
                          "缩放围绕局部原点（左上角）——左上角不动，"
                          "图片向右下伸缩。", 20);

            // 每帧设置缩放并绘制（设置一次持续生效，但这里为了对比
            // 三种倍率，用同一个编号轮流改也行——缩放是图片的属性，
            // 改了就影响后续所有 bgt_draw_image）
            //
            // 想同时画三种倍率？那就加载三次得到三个编号！
            // 这就是"同一文件多次加载得到独立编号"的用处。
            const int img_small = bgt_load_image("09_image.png");
            const int img_big = bgt_load_image("09_image.png");
            bgt_scale_image(img_small, 0.5, 0.5);
            bgt_scale_image(img_big, 2.0, 2.0);

            // 三张图并排：缩小、原始、放大
            bgt_draw_image(img, 80, 220);            // img: 原始 (scale=1.0)
            bgt_draw_image(img_small, 280, 220);     // 0.5 倍
            bgt_draw_image(img_big, 500, 220);       // 2.0 倍 → 256x256

            bgt_set_color(BGT_BLACK);
            bgt_draw_text(80, 530, "scale(1.0, 1.0) = 原始", 18);
            bgt_draw_text(240, 530, "scale(0.5, 0.5) = 缩小一半", 18);
            bgt_draw_text(500, 530, "scale(2.0, 2.0) = 放大一倍", 18);

            bgt_draw_text(40, 580,
                          "注意：三张图的左上角都在同一水平线上，"
                          "但大小不同——因为缩放围绕左上角。", 18);

            // 清理多加载的两个编号（关窗时也会自动释放，这里只是为了
            // 让后面的板块不受影响——实际上后面的板块每次都重新设置
            // 变换状态，所以不清也没关系。但养成好习惯：不用的图片让
            // 程序结束时自动回收即可。）

        } else if (part == 4) {
            // =====================================================
            // 板块 4：S — 翻转（负缩放）
            // =====================================================
            bgt_set_background(bgt_rgb(250, 250, 250));
            bgt_set_window_title("libbgt 图片演示 - 板块 4：S — 翻转");

            bgt_set_color(BGT_BLACK);
            bgt_draw_text(40, 36, "板块 4：S — 翻转（负缩放）", 32);
            bgt_draw_text(40, 94,
                          "RST 里翻转没有独立函数——负缩放就是翻转：", 20);
            bgt_draw_text(40, 128,
                          "  sx = -1.0  →  左右翻转    sy = -1.0  →  "
                          "上下翻转", 20);

            bgt_set_color(BGT_RED);
            bgt_draw_text(40, 162,
                          "注意：翻转后图片翻到原点的另一侧！", 20);
            bgt_set_color(BGT_BLACK);

            // 原始图
            bgt_draw_image(img, 100, 220);
            bgt_draw_text(100, 370, "原始 scale(1, 1)", 18);

            // 左右翻转：需要第二个编号，否则改的是同一张图
            const int img_flip_h = bgt_load_image("09_image.png");
            bgt_scale_image(img_flip_h, -1.0, 1.0);
            bgt_draw_image(img_flip_h, 350, 220);
            // 图片翻到 (350, 220) 的左侧！实际占据 (222, 220) 到 (350, 348)
            bgt_set_color(BGT_RED);
            bgt_draw_line(340, 220, 360, 220);
            bgt_draw_line(350, 210, 350, 230);
            bgt_set_color(BGT_BLACK);
            bgt_draw_text(300, 370, "scale(-1, 1) 左右翻转", 18);
            bgt_draw_text(300, 400, "（图片在十字的左侧）", 16);

            // 上下翻转：再来一个编号
            const int img_flip_v = bgt_load_image("09_image.png");
            bgt_scale_image(img_flip_v, 1.0, -1.0);
            bgt_draw_image(img_flip_v, 600, 480);
            // 图片翻到 (600, 480) 的上方！实际占据 (600, 352) 到 (728, 480)
            bgt_set_color(BGT_RED);
            bgt_draw_line(590, 480, 610, 480);
            bgt_draw_line(600, 470, 600, 490);
            bgt_set_color(BGT_BLACK);
            bgt_draw_text(560, 500, "scale(1, -1) 上下翻转", 18);
            bgt_draw_text(560, 530, "（图片在十字的上方）", 16);

            bgt_draw_text(40, 580,
                          "红色十字标记局部原点的位置——翻转不改变原点坐标，",
                          18);
            bgt_draw_text(40, 610,
                          "只是图片的'身体'翻到了另一侧。这是严格 RST 的行为。",
                          18);

        } else if (part == 5) {
            // =====================================================
            // 板块 5：R — 旋转
            // =====================================================
            bgt_set_background(bgt_rgb(250, 250, 250));
            bgt_set_window_title("libbgt 图片演示 - 板块 5：R — 旋转");

            bgt_set_color(BGT_BLACK);
            bgt_draw_text(40, 36, "板块 5：R — 旋转", 32);
            bgt_draw_text(40, 94,
                          "bgt_rotate_image(编号, angle)：设置旋转角度"
                          "（度，正值顺时针）。", 20);
            bgt_draw_text(40, 128,
                          "旋转围绕图片的局部原点（左上角）——图片绕它转动。",
                          20);
            bgt_draw_text(40, 162,
                          "按 ←/→ 调整角度，观察图片绕左上角的红色十字旋转。",
                          20);

            // 交互：←/→ 改角度
            if (bgt_key_is_down(BGT_KEY_LEFT)) {
                demo_angle = demo_angle - 90.0 * bgt_delta_time();
            }
            if (bgt_key_is_down(BGT_KEY_RIGHT)) {
                demo_angle = demo_angle + 90.0 * bgt_delta_time();
            }

            // 旋转中心的参考图（不旋转，作对比）
            bgt_draw_image(img, 600, 220);

            // 旋转的图
            bgt_rotate_image(img, demo_angle);
            bgt_draw_image(img, 200, 220);
            bgt_rotate_image(img, 0.0);   // 旋转回来，不影响后面的板块

            // 标记旋转中心（局部原点）
            bgt_set_color(BGT_RED);
            bgt_draw_line(190, 220, 210, 220);
            bgt_draw_line(200, 210, 200, 230);

            char angle_text[64];
            std::snprintf(angle_text, sizeof(angle_text),
                          "当前角度：%.0f 度", demo_angle);
            bgt_set_color(BGT_BLACK);
            bgt_draw_text(200, 420, angle_text, 20);
            bgt_draw_text(600, 370, "参考：未旋转", 18);

            bgt_draw_text(40, 480,
                          "看红色十字：旋转时左上角始终不动，图片绕它摆动。",
                          18);
            bgt_draw_text(40, 510,
                          "这就是严格 RST——旋转围绕局部原点(0, 0)，",
                          18);
            bgt_draw_text(40, 540,
                          "不是围绕图片中心。想围绕中心转是后续课程的话题。",
                          18);

        } else if (part == 6) {
            // =====================================================
            // 板块 6：T — 平移
            // =====================================================
            bgt_set_background(bgt_rgb(250, 250, 250));
            bgt_set_window_title("libbgt 图片演示 - 板块 6：T — 平移");

            bgt_set_color(BGT_BLACK);
            bgt_draw_text(40, 36, "板块 6：T — 平移", 32);
            bgt_draw_text(40, 94,
                          "bgt_translate_image(编号, dx, dy) 设置偏移量。",
                          20);
            bgt_draw_text(40, 128,
                          "实际显示位置 = bgt_draw_image 的 (x, y) + "
                          "这里的 (dx, dy)。", 20);
            bgt_draw_text(40, 162,
                          "适合做弹跳、后坐力等'逻辑位置不变、视觉位置微调'"
                          "的动画。", 20);

            // 弹跳动画：逻辑位置 (500, 320) 不变，视觉位置上下浮动
            bounce_t = bounce_t + bgt_delta_time() * 3.0;
            const double bounce = std::sin(bounce_t) * 20.0;
            const int bounce_offset = static_cast<int>(bounce);

            // 有偏移的图
            bgt_translate_image(img, 0, bounce_offset);
            bgt_draw_image(img, 500, 320);

            // 无偏移的参考图（用另一个编号保持无偏移）
            const int img_ref = bgt_load_image("09_image.png");
            bgt_draw_image(img_ref, 200, 320);

            // 逻辑位置标记（十字 = 逻辑位置，图片在它上下浮动）
            bgt_set_color(BGT_RED);
            bgt_draw_line(490, 320, 510, 320);
            bgt_draw_line(500, 310, 500, 330);
            bgt_set_color(BGT_BLACK);

            bgt_draw_text(200, 470, "无偏移（参考）", 18);
            bgt_draw_text(500, 470,
                          "有偏移：红色十字是逻辑位置，图片在浮动", 18);

            char offset_text[64];
            std::snprintf(offset_text, sizeof(offset_text),
                          "当前偏移 translate(0, %d)", bounce_offset);
            bgt_draw_text(40, 530, offset_text, 20);

            bgt_draw_text(40, 580,
                          "偏移是图片的属性，设置一次持续生效直到改变。",
                          18);

            // 恢复偏移，不影响后面的板块
            bgt_translate_image(img, 0, 0);

        } else if (part == 7) {
            // =====================================================
            // 板块 7：RST 组合与清除
            // =====================================================
            bgt_set_background(bgt_rgb(250, 250, 250));
            bgt_set_window_title("libbgt 图片演示 - 板块 7：RST 组合与清除");

            bgt_set_color(BGT_BLACK);
            bgt_draw_text(40, 36, "板块 7：RST 组合与清除", 32);
            bgt_draw_text(40, 94,
                          "三个变换可以同时生效，合成顺序固定：", 20);
            bgt_draw_text(40, 128,
                          "  先缩放（S），再旋转（R），最后平移（T）",
                          20);
            bgt_draw_text(40, 162,
                          "  M = T * R * S    （先 S 后 R 后 T）", 20);

            // 左：原始图
            bgt_draw_image(img, 100, 240);

            // 右：缩放 + 旋转 + 平移的组合
            const int img_combo = bgt_load_image("09_image.png");
            bgt_scale_image(img_combo, 1.5, 1.5);       // S：放大 1.5 倍
            bgt_rotate_image(img_combo, 30.0);           // R：转 30 度
            bgt_translate_image(img_combo, 50, 20);      // T：向右下偏移
            bgt_draw_image(img_combo, 400, 240);

            bgt_draw_text(100, 390, "原始", 18);
            bgt_draw_text(400, 460, "scale(1.5, 1.5) + rotate(30) + "
                                    "translate(50, 20)", 16);
            bgt_draw_text(400, 490, "（先缩放，再绕左上角转，最后偏移）",
                          16);

            // 清除后恢复原样
            bgt_clear_image_transform(img_combo);
            bgt_draw_image(img_combo, 750, 240);
            bgt_draw_text(750, 390, "clear 后", 18);

            bgt_draw_text(40, 540,
                          "bgt_clear_image_transform 一次调用把 T、R、S "
                          "全部恢复默认：", 18);
            bgt_draw_text(40, 570,
                          "  T(0, 0)、R(0 度)、S(1.0, 1.0) = 恒等变换",
                          18);
            bgt_draw_text(40, 600,
                          "图片回到刚加载时的样子。", 18);

        } else {
            // =====================================================
            // 板块 8：错误路径
            // =====================================================
            bgt_set_background(bgt_rgb(250, 250, 250));
            bgt_set_window_title("libbgt 图片演示 - 板块 8：错误路径");

            bgt_set_color(BGT_BLACK);
            bgt_draw_text(40, 36, "板块 8：错误路径", 32);
            bgt_draw_text(40, 94,
                          "图片操作失败不会崩溃——会记录错误，"
                          "你可以看到它。", 20);

            // 故意加载一个不存在的文件
            const int bad = bgt_load_image("this_file_does_not_exist.png");

            if (bad == BGT_IMAGE_NONE) {
                bgt_set_color(BGT_RED);
                bgt_draw_text(40, 150,
                              "加载不存在的文件 → 返回 BGT_IMAGE_NONE(0)：",
                              20);
                bgt_draw_error(40, 190, 18);
                if (!error_printed) {
                    error_printed = true;
                    bgt_print_error();
                }
            }

            // 故意用无效编号绘制
            bgt_set_color(BGT_BLACK);
            bgt_draw_text(40, 250,
                          "故意用编号 0（无效）绘制 → 记录错误，不崩溃：",
                          20);
            bgt_clear_error();
            bgt_draw_image(BGT_IMAGE_NONE, 100, 300);
            if (bgt_has_error()) {
                bgt_set_color(BGT_RED);
                bgt_draw_error(40, 290, 18);
            }

            bgt_set_color(BGT_BLACK);
            bgt_draw_text(40, 360,
                          "用 bgt_clear_error() 清除后，bgt_has_error() "
                          "变回 false。", 20);
            bgt_draw_text(40, 400,
                          "常见 bug：加载失败没检查，画面一片空白——",
                          20);
            bgt_draw_text(40, 430,
                          "用 bgt_draw_error() 就能立刻看到原因。",
                          20);

            bgt_draw_text(40, 640, "按【空格】关闭窗口（Esc 也可以）。", 20);
        }

        // 底部统一提示（最后一个板块自己提示了关闭方式）
        if (part < 8) {
            bgt_set_color(BGT_DARK_GRAY);
            bgt_draw_text(40, 648,
                          "按【空格】进入下一板块，按【Esc】退出程序", 18);
        }

        // 每个板块都支持按空格进入下一板块；最后一个板块按空格关闭
        if (bgt_key_just_pressed(BGT_KEY_SPACE)) {
            if (part == 8) {
                break;
            }
            part = part + 1;
        }

        bgt_update_window();
    }

    bgt_close_window();
    return 0;
}

// NOLINTEND(readability-magic-numbers, readability-identifier-length,
// readability-function-cognitive-complexity)
