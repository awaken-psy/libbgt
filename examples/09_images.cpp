// v0.2 图片 API 验收示例：原尺寸 / 缩放 / 旋转 / 翻转 / 透明混合。
#include "bgt.h"

int main()
{
    bgt_open_window(800, 600, "图片绘制演示");
    bgt_set_background(BGT_WHITE);

    const int image = bgt_load_image("09_image.png");

    while (bgt_window_is_open()) {
        if (image == BGT_IMAGE_NONE) {
            bgt_set_color(BGT_RED);
            bgt_draw_text(40, 40, "图片加载失败：", 24);
            bgt_draw_error(40, 80, 20);
            bgt_update_window();
            continue;
        }

        bgt_set_color(BGT_BLACK);

        bgt_draw_image(image, 30, 30);
        bgt_draw_text(30, 180, "原尺寸", 20);

        bgt_draw_image(image, 200, 30, 256, 256);
        bgt_draw_text(200, 310, "放大 2 倍", 20);

        const double angle = bgt_total_time() * 90.0;
        bgt_draw_image_rotated(image, 520, 60, 128, 128, angle);
        bgt_draw_text(520, 220, "顺时针旋转", 20);

        bgt_draw_image_flipped(image, 520, 280, 96, 96, BGT_FLIP_HORIZONTAL);
        bgt_draw_image_flipped(image, 640, 280, 96, 96, BGT_FLIP_VERTICAL);
        bgt_draw_text(520, 400, "水平 / 垂直翻转", 20);

        bgt_draw_text(30, 540, "左下象限是完全透明的：背景应为白色", 18);

        bgt_update_window();
    }

    bgt_close_window();
    return 0;
}
