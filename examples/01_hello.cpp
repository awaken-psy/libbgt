#include "bgt.h"

int main()
{
    constexpr int kWindowWidth = 800;
    constexpr int kWindowHeight = 600;
    constexpr int kTargetFps = 60;
    constexpr int kCircleX = 400;
    constexpr int kCircleY = 300;
    constexpr int kCircleRadius = 60;
    constexpr int kTextX = 260;
    constexpr int kTextY = 210;
    constexpr int kTextSize = 32;

    if (!bgt_open_window(kWindowWidth, kWindowHeight, "libbgt hello")) {
        bgt_print_error();
        return 1;
    }

    bgt_set_fps_limit(kTargetFps);

    while (bgt_window_is_open()) {
        bgt_clear_screen(BGT_WHITE);

        bgt_set_color(BGT_BLUE);
        bgt_fill_circle(kCircleX, kCircleY, kCircleRadius);

        bgt_set_color(BGT_BLACK);
        bgt_draw_text(kTextX, kTextY, "你好，libbgt！", kTextSize);

        bgt_update_window();
    }

    bgt_close_window();
    return 0;
}
