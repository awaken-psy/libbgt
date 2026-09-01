#include "bgt.h"

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

// The implementation mirrors the beginner-facing API, where short coordinate
// names, integer colors, and C-style string parameters are deliberate.
// NOLINTBEGIN(readability-magic-numbers, modernize-avoid-c-arrays,
// bugprone-easily-swappable-parameters, readability-identifier-length)

namespace {

constexpr int kMaxPublicKey = 512;
constexpr int kMouseButtonCount = 4;
constexpr int kDefaultFontSize = 24;
constexpr double kFpsWindowSeconds = 1.0;
constexpr double kTwoPi = 6.28318530717958647692;

struct FontEntry {
    std::string path;
    int size = kDefaultFontSize;
    TTF_Font *font = nullptr;
};

struct Point {
    int x = 0;
    int y = 0;
};

struct State {
    SDL_Window *window = nullptr;
    SDL_Renderer *renderer = nullptr;
    bool sdl_ready = false;
    bool ttf_ready = false;
    bool open = false;
    bool closing = false;
    int width = 0;
    int height = 0;
    unsigned color = BGT_WHITE;
    unsigned background = BGT_BLACK;
    int line_width = 1;
    int font_size = kDefaultFontSize;
    int fps_limit = 0;
    std::string font_path;
    std::vector<FontEntry> fonts;
    std::array<bool, kMaxPublicKey> keys{};
    std::array<bool, kMaxPublicKey> previous_keys{};
    std::array<bool, kMouseButtonCount> mouse_buttons{};
    std::array<bool, kMouseButtonCount> previous_mouse_buttons{};
    int mouse_x = 0;
    int mouse_y = 0;
    int mouse_wheel = 0;
    std::uint64_t start_ticks = 0;
    std::uint64_t previous_ticks = 0;
    std::uint64_t fps_ticks = 0;
    int fps_frames = 0;
    std::uint64_t next_frame_ns = 0;
    double delta_time = 0.0;
    double total_time = 0.0;
    double fps = 0.0;
    int error_code = BGT_ERROR_NONE;
    std::string error_message;

    ~State()
    {
        close();
    }

    void clear_error()
    {
        error_code = BGT_ERROR_NONE;
        error_message.clear();
        SDL_ClearError();
    }

    void set_error(int code, const std::string &message)
    {
        error_code = code;
        error_message = message;
        const char *sdl_error = SDL_GetError();
        if (sdl_error != nullptr && sdl_error[0] != '\0') {
            error_message += ": ";
            error_message += sdl_error;
        }
    }

    void close_fonts()
    {
        for (FontEntry &entry : fonts) {
            if (entry.font != nullptr) {
                TTF_CloseFont(entry.font);
                entry.font = nullptr;
            }
        }
        fonts.clear();
    }

    void close()
    {
        close_fonts();
        if (renderer != nullptr) {
            SDL_DestroyRenderer(renderer);
            renderer = nullptr;
        }
        if (window != nullptr) {
            SDL_DestroyWindow(window);
            window = nullptr;
        }
        if (ttf_ready) {
            TTF_Quit();
            ttf_ready = false;
        }
        if (sdl_ready) {
            SDL_Quit();
            sdl_ready = false;
        }
        open = false;
        closing = false;
        width = 0;
        height = 0;
        keys.fill(false);
        previous_keys.fill(false);
        mouse_buttons.fill(false);
        previous_mouse_buttons.fill(false);
        mouse_x = 0;
        mouse_y = 0;
        mouse_wheel = 0;
    }
};

State &state()
{
    static State instance;
    return instance;
}

int clamp_byte(int value)
{
    return std::clamp(value, 0, 255);
}

std::uint8_t color_a(unsigned color)
{
    return static_cast<std::uint8_t>((static_cast<std::uint32_t>(color) >> 24U) &
                                     0xFFU);
}

std::uint8_t color_r(unsigned color)
{
    return static_cast<std::uint8_t>(
        (static_cast<std::uint32_t>(color) >> 16U) & 0xFFU);
}

std::uint8_t color_g(unsigned color)
{
    return static_cast<std::uint8_t>((static_cast<std::uint32_t>(color) >> 8U) &
                                     0xFFU);
}

std::uint8_t color_b(unsigned color)
{
    return static_cast<std::uint8_t>(static_cast<std::uint32_t>(color) & 0xFFU);
}

SDL_Color to_sdl_color(unsigned color)
{
    return SDL_Color{color_r(color), color_g(color), color_b(color),
                     color_a(color)};
}

void apply_render_color(State &s, unsigned color)
{
    if (s.renderer == nullptr) {
        return;
    }
    SDL_SetRenderDrawColor(s.renderer, color_r(color), color_g(color),
                           color_b(color), color_a(color));
}

bool ensure_open(State &s)
{
    if (!s.open || s.renderer == nullptr) {
        s.set_error(BGT_ERROR_NOT_OPEN, "libbgt window is not open");
        return false;
    }
    return true;
}

std::string join_path(const std::string &left, const std::string &right)
{
    if (left.empty()) {
        return right;
    }
    const char tail = left[left.size() - 1];
    if (tail == '/' || tail == '\\') {
        return left + right;
    }
    return left + "/" + right;
}

bool file_exists(const std::string &path)
{
    SDL_IOStream *stream = SDL_IOFromFile(path.c_str(), "rb");
    if (stream == nullptr) {
        return false;
    }
    SDL_CloseIO(stream);
    return true;
}

void add_candidate(std::vector<std::string> &candidates,
                   const std::string &path)
{
    if (path.empty()) {
        return;
    }
    if (std::find(candidates.begin(), candidates.end(), path) ==
        candidates.end()) {
        candidates.push_back(path);
    }
}

std::vector<std::string> default_font_candidates()
{
    std::vector<std::string> candidates;
#ifdef _WIN32
    const char *win_dir = SDL_getenv("WINDIR");
    const std::string font_dir =
        win_dir == nullptr ? "C:/Windows/Fonts" : join_path(win_dir, "Fonts");
    add_candidate(candidates, join_path(font_dir, "msyh.ttc"));
    add_candidate(candidates, join_path(font_dir, "simhei.ttf"));
#elif defined(__APPLE__)
    add_candidate(candidates, "/System/Library/Fonts/PingFang.ttc");
    add_candidate(candidates, "/System/Library/Fonts/STHeiti Light.ttc");
#else
    add_candidate(candidates,
                  "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc");
    add_candidate(candidates, "/usr/share/fonts/truetype/wqy/wqy-microhei.ttc");
    add_candidate(candidates,
                  "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf");
#endif

    return candidates;
}

std::string resolve_default_font()
{
    for (const std::string &candidate : default_font_candidates()) {
        if (file_exists(candidate)) {
            return candidate;
        }
    }
    return "";
}

TTF_Font *get_font(State &s, int size)
{
    if (size <= 0) {
        size = s.font_size;
    }
    if (size <= 0) {
        size = kDefaultFontSize;
    }

    if (s.font_path.empty()) {
        s.font_path = resolve_default_font();
        if (s.font_path.empty()) {
            s.set_error(BGT_ERROR_FONT,
                        "no usable system font found; install a CJK font or "
                        "use bgt_set_font()");
            return nullptr;
        }
    }

    for (FontEntry &entry : s.fonts) {
        if (entry.path == s.font_path && entry.size == size) {
            return entry.font;
        }
    }

    TTF_Font *font =
        TTF_OpenFont(s.font_path.c_str(), static_cast<float>(size));
    if (font == nullptr) {
        s.set_error(BGT_ERROR_FONT, "failed to open font " + s.font_path);
        return nullptr;
    }

    s.fonts.push_back(FontEntry{s.font_path, size, font});
    return font;
}

int digit_key_from_scancode(SDL_Scancode scancode)
{
    switch (scancode) {
        case SDL_SCANCODE_0:
        case SDL_SCANCODE_KP_0:
            return BGT_KEY_0;
        case SDL_SCANCODE_1:
        case SDL_SCANCODE_KP_1:
            return BGT_KEY_1;
        case SDL_SCANCODE_2:
        case SDL_SCANCODE_KP_2:
            return BGT_KEY_2;
        case SDL_SCANCODE_3:
        case SDL_SCANCODE_KP_3:
            return BGT_KEY_3;
        case SDL_SCANCODE_4:
        case SDL_SCANCODE_KP_4:
            return BGT_KEY_4;
        case SDL_SCANCODE_5:
        case SDL_SCANCODE_KP_5:
            return BGT_KEY_5;
        case SDL_SCANCODE_6:
        case SDL_SCANCODE_KP_6:
            return BGT_KEY_6;
        case SDL_SCANCODE_7:
        case SDL_SCANCODE_KP_7:
            return BGT_KEY_7;
        case SDL_SCANCODE_8:
        case SDL_SCANCODE_KP_8:
            return BGT_KEY_8;
        case SDL_SCANCODE_9:
        case SDL_SCANCODE_KP_9:
            return BGT_KEY_9;
        default:
            return 0;
    }
}

int numlock_off_keypad_key(SDL_Scancode scancode)
{
    switch (scancode) {
        case SDL_SCANCODE_KP_2:
            return BGT_KEY_DOWN;
        case SDL_SCANCODE_KP_4:
            return BGT_KEY_LEFT;
        case SDL_SCANCODE_KP_6:
            return BGT_KEY_RIGHT;
        case SDL_SCANCODE_KP_8:
            return BGT_KEY_UP;
        default:
            return 0;
    }
}

bool is_keypad_digit_scancode(SDL_Scancode scancode)
{
    switch (scancode) {
        case SDL_SCANCODE_KP_0:
        case SDL_SCANCODE_KP_1:
        case SDL_SCANCODE_KP_2:
        case SDL_SCANCODE_KP_3:
        case SDL_SCANCODE_KP_4:
        case SDL_SCANCODE_KP_5:
        case SDL_SCANCODE_KP_6:
        case SDL_SCANCODE_KP_7:
        case SDL_SCANCODE_KP_8:
        case SDL_SCANCODE_KP_9:
            return true;
        default:
            return false;
    }
}

int scancode_to_key(SDL_Scancode scancode, SDL_Keymod modifiers)
{
    switch (scancode) {
        case SDL_SCANCODE_LEFT:
            return BGT_KEY_LEFT;
        case SDL_SCANCODE_RIGHT:
            return BGT_KEY_RIGHT;
        case SDL_SCANCODE_UP:
            return BGT_KEY_UP;
        case SDL_SCANCODE_DOWN:
            return BGT_KEY_DOWN;
        case SDL_SCANCODE_SPACE:
            return BGT_KEY_SPACE;
        case SDL_SCANCODE_RETURN:
        case SDL_SCANCODE_KP_ENTER:
            return BGT_KEY_ENTER;
        case SDL_SCANCODE_ESCAPE:
            return BGT_KEY_ESCAPE;
        case SDL_SCANCODE_TAB:
        case SDL_SCANCODE_KP_TAB:
            return BGT_KEY_TAB;
        case SDL_SCANCODE_BACKSPACE:
        case SDL_SCANCODE_KP_BACKSPACE:
            return BGT_KEY_BACKSPACE;
        case SDL_SCANCODE_LSHIFT:
        case SDL_SCANCODE_RSHIFT:
            return BGT_KEY_SHIFT;
        case SDL_SCANCODE_LCTRL:
        case SDL_SCANCODE_RCTRL:
            return BGT_KEY_CTRL;
        case SDL_SCANCODE_LALT:
        case SDL_SCANCODE_RALT:
            return BGT_KEY_ALT;
        default:
            break;
    }

    if (scancode >= SDL_SCANCODE_A && scancode <= SDL_SCANCODE_Z) {
        return BGT_KEY_A + (scancode - SDL_SCANCODE_A);
    }
    if (scancode >= SDL_SCANCODE_F1 && scancode <= SDL_SCANCODE_F12) {
        return BGT_KEY_F1 + (scancode - SDL_SCANCODE_F1);
    }

    if ((modifiers & SDL_KMOD_NUM) == 0) {
        const int keypad_key = numlock_off_keypad_key(scancode);
        if (keypad_key != 0) {
            return keypad_key;
        }
    }

    const int digit_key = digit_key_from_scancode(scancode);
    if (digit_key != 0 && ((modifiers & SDL_KMOD_NUM) != 0 ||
                           !is_keypad_digit_scancode(scancode))) {
        return digit_key;
    }

    return 0;
}

void sync_keyboard(State &s)
{
    s.keys.fill(false);

    int key_count = 0;
    const bool *keyboard = SDL_GetKeyboardState(&key_count);
    if (keyboard == nullptr) {
        return;
    }

    const SDL_Keymod modifiers = SDL_GetModState();
    for (int scancode_value = 0; scancode_value < key_count; ++scancode_value) {
        if (!keyboard[scancode_value]) {
            continue;
        }
        const int key = scancode_to_key(
            static_cast<SDL_Scancode>(scancode_value), modifiers);
        if (key > 0 && key < kMaxPublicKey) {
            s.keys[static_cast<std::size_t>(key)] = true;
        }
    }
}

int mouse_index(int button)
{
    if (button < BGT_MOUSE_LEFT || button > BGT_MOUSE_MIDDLE) {
        return 0;
    }
    return button;
}

void sync_mouse(State &s)
{
    float window_x = 0.0F;
    float window_y = 0.0F;
    const SDL_MouseButtonFlags flags = SDL_GetMouseState(&window_x, &window_y);

    float render_x = window_x;
    float render_y = window_y;
    if (s.renderer != nullptr) {
        SDL_RenderCoordinatesFromWindow(s.renderer, window_x, window_y,
                                        &render_x, &render_y);
    }

    s.mouse_x = static_cast<int>(std::lround(render_x));
    s.mouse_y = static_cast<int>(std::lround(render_y));
    s.mouse_buttons[BGT_MOUSE_LEFT] = (flags & SDL_BUTTON_LMASK) != 0U;
    s.mouse_buttons[BGT_MOUSE_RIGHT] = (flags & SDL_BUTTON_RMASK) != 0U;
    s.mouse_buttons[BGT_MOUSE_MIDDLE] = (flags & SDL_BUTTON_MMASK) != 0U;
}

void update_time(State &s)
{
    const std::uint64_t now = SDL_GetTicks();
    if (s.previous_ticks == 0) {
        s.previous_ticks = now;
    }
    s.delta_time = static_cast<double>(now - s.previous_ticks) / 1000.0;
    s.total_time = static_cast<double>(now - s.start_ticks) / 1000.0;
    s.previous_ticks = now;
}

void update_fps(State &s)
{
    ++s.fps_frames;
    const std::uint64_t now = SDL_GetTicks();
    const double elapsed = static_cast<double>(now - s.fps_ticks) / 1000.0;
    if (elapsed >= kFpsWindowSeconds) {
        s.fps = static_cast<double>(s.fps_frames) / elapsed;
        s.fps_frames = 0;
        s.fps_ticks = now;
    }
}

void apply_fps_limit(State &s)
{
    if (s.fps_limit <= 0) {
        s.next_frame_ns = 0;
        return;
    }
    const std::uint64_t period_ns = static_cast<std::uint64_t>(SDL_NS_PER_SECOND)
        / static_cast<std::uint64_t>(s.fps_limit);
    const std::uint64_t now = SDL_GetTicksNS();

    if (s.next_frame_ns == 0) {
        s.next_frame_ns = now + period_ns;
        return;
    }
    if (now < s.next_frame_ns) {
        SDL_DelayPrecise(s.next_frame_ns - now);
    }
    else if (now - s.next_frame_ns >= period_ns) {
        // 落后超过一整个帧周期（如卡顿、拖动窗口）时放弃补帧，
        // 从当前时刻重新对齐节拍，避免连续高速追赶。
        s.next_frame_ns = now + period_ns;
        return;
    }
    s.next_frame_ns += period_ns;
}

void draw_hline(State &s, int x1, int x2, int y)
{
    if (x1 > x2) {
        std::swap(x1, x2);
    }
    SDL_RenderLine(s.renderer, static_cast<float>(x1), static_cast<float>(y),
                   static_cast<float>(x2), static_cast<float>(y));
}

double fractional_part(double value)
{
    return value - std::floor(value);
}

double reverse_fractional_part(double value)
{
    return 1.0 - fractional_part(value);
}

void draw_coverage_point(State &s, int x, int y, double coverage)
{
    coverage = std::clamp(coverage, 0.0, 1.0);
    const int alpha =
        static_cast<int>(std::lround(color_a(s.color) * coverage));
    if (alpha <= 0) {
        return;
    }

    SDL_SetRenderDrawColor(s.renderer, color_r(s.color), color_g(s.color),
                           color_b(s.color), static_cast<std::uint8_t>(alpha));
    SDL_RenderPoint(s.renderer, static_cast<float>(x), static_cast<float>(y));
}

void draw_aa_line(State &s, int x1, int y1, int x2, int y2)
{
    if (x1 == x2 && y1 == y2) {
        draw_coverage_point(s, x1, y1, 1.0);
        return;
    }

    const bool steep = std::abs(y2 - y1) > std::abs(x2 - x1);
    if (steep) {
        std::swap(x1, y1);
        std::swap(x2, y2);
    }
    if (x1 > x2) {
        std::swap(x1, x2);
        std::swap(y1, y2);
    }

    const double dx = static_cast<double>(x2 - x1);
    const double dy = static_cast<double>(y2 - y1);
    const double gradient = dx == 0.0 ? 1.0 : dy / dx;

    const int first_x = static_cast<int>(std::lround(x1));
    const double first_y = y1 + (gradient * (first_x - x1));
    const double first_gap = reverse_fractional_part(x1 + 0.5);
    const int first_pixel_y = static_cast<int>(std::floor(first_y));

    if (steep) {
        draw_coverage_point(s, first_pixel_y, first_x,
                            reverse_fractional_part(first_y) * first_gap);
        draw_coverage_point(s, first_pixel_y + 1, first_x,
                            fractional_part(first_y) * first_gap);
    }
    else {
        draw_coverage_point(s, first_x, first_pixel_y,
                            reverse_fractional_part(first_y) * first_gap);
        draw_coverage_point(s, first_x, first_pixel_y + 1,
                            fractional_part(first_y) * first_gap);
    }

    double inter_y = first_y + gradient;

    const int last_x = static_cast<int>(std::lround(x2));
    const double last_y = y2 + (gradient * (last_x - x2));
    const double last_gap = fractional_part(x2 + 0.5);
    const int last_pixel_y = static_cast<int>(std::floor(last_y));

    for (int x = first_x + 1; x < last_x; ++x) {
        const int y = static_cast<int>(std::floor(inter_y));
        if (steep) {
            draw_coverage_point(s, y, x, reverse_fractional_part(inter_y));
            draw_coverage_point(s, y + 1, x, fractional_part(inter_y));
        }
        else {
            draw_coverage_point(s, x, y, reverse_fractional_part(inter_y));
            draw_coverage_point(s, x, y + 1, fractional_part(inter_y));
        }
        inter_y += gradient;
    }

    if (steep) {
        draw_coverage_point(s, last_pixel_y, last_x,
                            reverse_fractional_part(last_y) * last_gap);
        draw_coverage_point(s, last_pixel_y + 1, last_x,
                            fractional_part(last_y) * last_gap);
    }
    else {
        draw_coverage_point(s, last_x, last_pixel_y,
                            reverse_fractional_part(last_y) * last_gap);
        draw_coverage_point(s, last_x, last_pixel_y + 1,
                            fractional_part(last_y) * last_gap);
    }
}

void draw_text_impl(State &s, int x, int y, const char text[], int size)
{
    if (!ensure_open(s) || text == nullptr || text[0] == '\0') {
        return;
    }

    TTF_Font *font = get_font(s, size);
    if (font == nullptr) {
        return;
    }

    SDL_Surface *surface =
        TTF_RenderText_Blended(font, text, 0, to_sdl_color(s.color));
    if (surface == nullptr) {
        s.set_error(BGT_ERROR_TTF, "failed to render text");
        return;
    }

    SDL_Texture *texture = SDL_CreateTextureFromSurface(s.renderer, surface);
    SDL_DestroySurface(surface);
    if (texture == nullptr) {
        s.set_error(BGT_ERROR_RENDERER, "failed to create text texture");
        return;
    }

    float width = 0.0F;
    float height = 0.0F;
    if (!SDL_GetTextureSize(texture, &width, &height)) {
        SDL_DestroyTexture(texture);
        s.set_error(BGT_ERROR_RENDERER, "failed to query text texture size");
        return;
    }

    SDL_FRect dst{static_cast<float>(x), static_cast<float>(y), width, height};
    SDL_RenderTexture(s.renderer, texture, nullptr, &dst);
    SDL_DestroyTexture(texture);
}

int text_size_impl(State &s, const char text[], int size, bool width)
{
    if (!ensure_open(s) || text == nullptr || text[0] == '\0') {
        return 0;
    }
    TTF_Font *font = get_font(s, size);
    if (font == nullptr) {
        return 0;
    }
    int measured_width = 0;
    int measured_height = 0;
    if (!TTF_GetStringSize(font, text, 0, &measured_width, &measured_height)) {
        s.set_error(BGT_ERROR_TTF, "failed to measure text");
        return 0;
    }
    return width ? measured_width : measured_height;
}

bool same_sign_or_zero(int value, int reference)
{
    return value == 0 || (value > 0) == (reference > 0);
}

int edge_value(const Point &a, const Point &b, const Point &p)
{
    return ((p.x - a.x) * (b.y - a.y)) - ((p.y - a.y) * (b.x - a.x));
}

bool open_window_impl(int width, int height, const char title[],
                      SDL_WindowFlags flags)
{
    State &s = state();
    if (width <= 0 || height <= 0) {
        s.set_error(BGT_ERROR_WINDOW, "window size must be positive");
        return false;
    }

    s.close();
    s.clear_error();

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        s.set_error(BGT_ERROR_SDL, "failed to initialize SDL3");
        return false;
    }
    s.sdl_ready = true;

    if (!TTF_Init()) {
        s.set_error(BGT_ERROR_TTF, "failed to initialize SDL3_ttf");
        s.close();
        return false;
    }
    s.ttf_ready = true;

    s.window = SDL_CreateWindow(title == nullptr ? "libbgt" : title, width,
                                height, flags);
    if (s.window == nullptr) {
        s.set_error(BGT_ERROR_WINDOW, "failed to create window");
        s.close();
        return false;
    }

    s.renderer = SDL_CreateRenderer(s.window, nullptr);
    if (s.renderer == nullptr) {
        s.set_error(BGT_ERROR_RENDERER, "failed to create renderer");
        s.close();
        return false;
    }

    SDL_SetRenderDrawBlendMode(s.renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderLogicalPresentation(s.renderer, width, height,
                                     SDL_LOGICAL_PRESENTATION_STRETCH);

    s.open = true;
    s.closing = false;
    s.width = width;
    s.height = height;
    s.color = BGT_WHITE;
    s.background = BGT_BLACK;
    s.line_width = 1;
    s.font_size = kDefaultFontSize;
    s.fps_limit = 0;
    s.start_ticks = SDL_GetTicks();
    s.previous_ticks = s.start_ticks;
    s.fps_ticks = s.start_ticks;
    s.fps_frames = 0;
    s.next_frame_ns = 0;
    s.delta_time = 0.0;
    s.total_time = 0.0;
    s.fps = 0.0;
    s.keys.fill(false);
    s.previous_keys.fill(false);
    s.mouse_buttons.fill(false);
    s.previous_mouse_buttons.fill(false);
    apply_render_color(s, s.background);
    SDL_RenderClear(s.renderer);
    SDL_RenderPresent(s.renderer);
    apply_render_color(s, s.color);
    return true;
}

} // namespace

bool bgt_open_window(int width, int height, const char title[])
{
    return open_window_impl(width, height, title, 0);
}

bool bgt_open_window_resizable(int width, int height, const char title[])
{
    return open_window_impl(width, height, title, SDL_WINDOW_RESIZABLE);
}

bool bgt_window_is_open()
{
    const State &s = state();
    return s.open && !s.closing;
}

void bgt_update_window()
{
    State &s = state();
    if (!ensure_open(s)) {
        return;
    }

    s.previous_keys = s.keys;
    s.previous_mouse_buttons = s.mouse_buttons;
    s.mouse_wheel = 0;

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT ||
            event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
            s.closing = true;
        }
        else if (event.type == SDL_EVENT_MOUSE_WHEEL) {
            s.mouse_wheel += static_cast<int>(std::lround(event.wheel.y));
        }
    }

    sync_keyboard(s);
    sync_mouse(s);
    update_time(s);

    SDL_RenderPresent(s.renderer);

    // 本帧已经显示：立刻把后备缓冲清成背景色，下一帧的绘制从空白画布开始。
    // 因此用户不需要（也没有）手动"清屏"：没画到的区域永远是背景色。
    apply_render_color(s, s.background);
    SDL_RenderClear(s.renderer);
    apply_render_color(s, s.color);

    update_fps(s);
    apply_fps_limit(s);
}

void bgt_close_window()
{
    state().close();
}

int bgt_window_width()
{
    return state().width;
}

int bgt_window_height()
{
    return state().height;
}

void bgt_set_window_title(const char title[])
{
    State &s = state();
    if (s.window != nullptr) {
        SDL_SetWindowTitle(s.window, title == nullptr ? "" : title);
    }
}

unsigned bgt_rgb(int r, int g, int b)
{
    return bgt_rgba(r, g, b, 255);
}

unsigned bgt_rgba(int r, int g, int b, int a)
{
    const std::uint32_t color =
        (static_cast<std::uint32_t>(clamp_byte(a)) << 24U) |
        (static_cast<std::uint32_t>(clamp_byte(r)) << 16U) |
        (static_cast<std::uint32_t>(clamp_byte(g)) << 8U) |
        static_cast<std::uint32_t>(clamp_byte(b));
    return static_cast<unsigned>(color);
}

void bgt_set_color(unsigned color)
{
    State &s = state();
    s.color = color;
    apply_render_color(s, color);
}

unsigned bgt_get_color()
{
    return state().color;
}

void bgt_set_background(unsigned color)
{
    // 只记录状态：真正的清空发生在 bgt_update_window() 结束时，
    // 下一帧起生效，中途调用不会擦掉本帧已经画好的内容。
    state().background = color;
}

void bgt_draw_point(int x, int y)
{
    State &s = state();
    if (!ensure_open(s)) {
        return;
    }
    apply_render_color(s, s.color);
    SDL_RenderPoint(s.renderer, static_cast<float>(x), static_cast<float>(y));
}

void bgt_draw_line(int x1, int y1, int x2, int y2)
{
    State &s = state();
    if (!ensure_open(s)) {
        return;
    }
    apply_render_color(s, s.color);
    const int half = std::max(0, s.line_width / 2);
    for (int offset = -half; offset <= half; ++offset) {
        draw_aa_line(s, x1 + offset, y1, x2 + offset, y2);
        if (offset != 0) {
            draw_aa_line(s, x1, y1 + offset, x2, y2 + offset);
        }
    }
    apply_render_color(s, s.color);
}

void bgt_draw_rect(int x, int y, int width, int height)
{
    State &s = state();
    if (!ensure_open(s) || width <= 0 || height <= 0) {
        return;
    }
    const int line_width = std::max(1, s.line_width);
    bgt_fill_rect(x, y, width, line_width);
    bgt_fill_rect(x, y + height - line_width, width, line_width);
    bgt_fill_rect(x, y, line_width, height);
    bgt_fill_rect(x + width - line_width, y, line_width, height);
}

void bgt_fill_rect(int x, int y, int width, int height)
{
    State &s = state();
    if (!ensure_open(s) || width <= 0 || height <= 0) {
        return;
    }
    apply_render_color(s, s.color);
    SDL_FRect rect{static_cast<float>(x), static_cast<float>(y),
                   static_cast<float>(width), static_cast<float>(height)};
    SDL_RenderFillRect(s.renderer, &rect);
}

void bgt_draw_circle(int x, int y, int radius)
{
    State &s = state();
    if (!ensure_open(s) || radius <= 0) {
        return;
    }
    apply_render_color(s, s.color);
    int dx = radius;
    int dy = 0;
    int error = 1 - dx;
    while (dx >= dy) {
        SDL_RenderPoint(s.renderer, static_cast<float>(x + dx),
                        static_cast<float>(y + dy));
        SDL_RenderPoint(s.renderer, static_cast<float>(x + dy),
                        static_cast<float>(y + dx));
        SDL_RenderPoint(s.renderer, static_cast<float>(x - dy),
                        static_cast<float>(y + dx));
        SDL_RenderPoint(s.renderer, static_cast<float>(x - dx),
                        static_cast<float>(y + dy));
        SDL_RenderPoint(s.renderer, static_cast<float>(x - dx),
                        static_cast<float>(y - dy));
        SDL_RenderPoint(s.renderer, static_cast<float>(x - dy),
                        static_cast<float>(y - dx));
        SDL_RenderPoint(s.renderer, static_cast<float>(x + dy),
                        static_cast<float>(y - dx));
        SDL_RenderPoint(s.renderer, static_cast<float>(x + dx),
                        static_cast<float>(y - dy));
        ++dy;
        if (error < 0) {
            error += (2 * dy) + 1;
        }
        else {
            --dx;
            error += 2 * (dy - dx + 1);
        }
    }
}

void bgt_fill_circle(int x, int y, int radius)
{
    State &s = state();
    if (!ensure_open(s) || radius <= 0) {
        return;
    }
    apply_render_color(s, s.color);
    for (int dy = -radius; dy <= radius; ++dy) {
        const double normalized_y =
            static_cast<double>(dy) / static_cast<double>(radius);
        const int span = static_cast<int>(std::floor(
            radius * std::sqrt(1.0 - (normalized_y * normalized_y))));
        draw_hline(s, x - span, x + span, y + dy);
    }
}

void bgt_draw_ellipse(int x, int y, int radius_x, int radius_y)
{
    State &s = state();
    if (!ensure_open(s) || radius_x <= 0 || radius_y <= 0) {
        return;
    }
    apply_render_color(s, s.color);
    const int steps = std::max(24, (radius_x + radius_y) * 3);
    int previous_x = x + radius_x;
    int previous_y = y;
    for (int step = 1; step <= steps; ++step) {
        const double angle =
            (static_cast<double>(step) * kTwoPi) / static_cast<double>(steps);
        const int next_x =
            x + static_cast<int>(std::lround(std::cos(angle) * radius_x));
        const int next_y =
            y + static_cast<int>(std::lround(std::sin(angle) * radius_y));
        draw_aa_line(s, previous_x, previous_y, next_x, next_y);
        previous_x = next_x;
        previous_y = next_y;
    }
    apply_render_color(s, s.color);
}

void bgt_fill_ellipse(int x, int y, int radius_x, int radius_y)
{
    State &s = state();
    if (!ensure_open(s) || radius_x <= 0 || radius_y <= 0) {
        return;
    }
    apply_render_color(s, s.color);
    for (int dy = -radius_y; dy <= radius_y; ++dy) {
        const double normalized_y =
            static_cast<double>(dy) / static_cast<double>(radius_y);
        const int span = static_cast<int>(std::lround(
            radius_x * std::sqrt(1.0 - (normalized_y * normalized_y))));
        draw_hline(s, x - span, x + span, y + dy);
    }
}

void bgt_draw_triangle(int x1, int y1, int x2, int y2, int x3, int y3)
{
    bgt_draw_line(x1, y1, x2, y2);
    bgt_draw_line(x2, y2, x3, y3);
    bgt_draw_line(x3, y3, x1, y1);
}

void bgt_fill_triangle(int x1, int y1, int x2, int y2, int x3, int y3)
{
    State &s = state();
    if (!ensure_open(s)) {
        return;
    }
    apply_render_color(s, s.color);
    const Point a{x1, y1};
    const Point b{x2, y2};
    const Point c{x3, y3};
    if (edge_value(a, b, c) == 0) {
        bgt_draw_triangle(x1, y1, x2, y2, x3, y3);
        return;
    }
    const int min_x = std::min({x1, x2, x3});
    const int max_x = std::max({x1, x2, x3});
    const int min_y = std::min({y1, y2, y3});
    const int max_y = std::max({y1, y2, y3});
    const Point reference{(x1 + x2 + x3) / 3, (y1 + y2 + y3) / 3};
    const int edge_ab_reference = edge_value(a, b, reference);
    const int edge_bc_reference = edge_value(b, c, reference);
    const int edge_ca_reference = edge_value(c, a, reference);

    for (int y = min_y; y <= max_y; ++y) {
        for (int x = min_x; x <= max_x; ++x) {
            const Point p{x, y};
            if (same_sign_or_zero(edge_value(a, b, p), edge_ab_reference) &&
                same_sign_or_zero(edge_value(b, c, p), edge_bc_reference) &&
                same_sign_or_zero(edge_value(c, a, p), edge_ca_reference)) {
                SDL_RenderPoint(s.renderer, static_cast<float>(x),
                                static_cast<float>(y));
            }
        }
    }
}

void bgt_set_line_width(int width)
{
    state().line_width = std::max(1, width);
}

int bgt_get_line_width()
{
    return state().line_width;
}

bool bgt_set_font(const char filename[], int size)
{
    State &s = state();
    if (filename == nullptr || filename[0] == '\0') {
        s.set_error(BGT_ERROR_FONT, "font filename is empty");
        return false;
    }
    if (size <= 0) {
        s.set_error(BGT_ERROR_FONT, "font size must be positive");
        return false;
    }
    s.close_fonts();
    s.font_path = filename;
    s.font_size = size;
    return get_font(s, size) != nullptr;
}

void bgt_set_font_size(int size)
{
    if (size > 0) {
        state().font_size = size;
    }
}

int bgt_get_font_size()
{
    return state().font_size;
}

void bgt_draw_text(int x, int y, const char text[])
{
    State &s = state();
    draw_text_impl(s, x, y, text, s.font_size);
}

void bgt_draw_text(int x, int y, const char text[], int size)
{
    draw_text_impl(state(), x, y, text, size);
}

int bgt_text_width(const char text[])
{
    State &s = state();
    return text_size_impl(s, text, s.font_size, true);
}

int bgt_text_width(const char text[], int size)
{
    return text_size_impl(state(), text, size, true);
}

int bgt_text_height(const char text[])
{
    State &s = state();
    return text_size_impl(s, text, s.font_size, false);
}

int bgt_text_height(const char text[], int size)
{
    return text_size_impl(state(), text, size, false);
}

bool bgt_key_is_down(int key)
{
    const State &s = state();
    if (key < 0 || key >= kMaxPublicKey) {
        return false;
    }
    return s.keys[static_cast<std::size_t>(key)];
}

bool bgt_key_just_pressed(int key)
{
    const State &s = state();
    if (key < 0 || key >= kMaxPublicKey) {
        return false;
    }
    const auto index = static_cast<std::size_t>(key);
    return s.keys[index] && !s.previous_keys[index];
}

bool bgt_key_just_released(int key)
{
    const State &s = state();
    if (key < 0 || key >= kMaxPublicKey) {
        return false;
    }
    const auto index = static_cast<std::size_t>(key);
    return !s.keys[index] && s.previous_keys[index];
}

int bgt_mouse_x()
{
    return state().mouse_x;
}

int bgt_mouse_y()
{
    return state().mouse_y;
}

bool bgt_mouse_is_down(int button)
{
    const int index = mouse_index(button);
    return index != 0 && state().mouse_buttons[static_cast<std::size_t>(index)];
}

bool bgt_mouse_just_pressed(int button)
{
    const int index = mouse_index(button);
    if (index == 0) {
        return false;
    }
    const State &s = state();
    const auto button_index = static_cast<std::size_t>(index);
    return s.mouse_buttons[button_index] &&
           !s.previous_mouse_buttons[button_index];
}

bool bgt_mouse_just_released(int button)
{
    const int index = mouse_index(button);
    if (index == 0) {
        return false;
    }
    const State &s = state();
    const auto button_index = static_cast<std::size_t>(index);
    return !s.mouse_buttons[button_index] &&
           s.previous_mouse_buttons[button_index];
}

int bgt_mouse_wheel()
{
    return state().mouse_wheel;
}

void bgt_show_mouse()
{
    SDL_ShowCursor();
}

void bgt_hide_mouse()
{
    SDL_HideCursor();
}

void bgt_delay(int milliseconds)
{
    if (milliseconds > 0) {
        SDL_Delay(static_cast<Uint32>(milliseconds));
    }
}

void bgt_set_fps_limit(int fps)
{
    state().fps_limit = std::max(0, fps);
}

double bgt_delta_time()
{
    return state().delta_time;
}

double bgt_total_time()
{
    return state().total_time;
}

double bgt_fps()
{
    return state().fps;
}

bool bgt_has_error()
{
    return state().error_code != BGT_ERROR_NONE;
}

int bgt_error_code()
{
    return state().error_code;
}

void bgt_print_error()
{
    const State &s = state();
    if (s.error_code == BGT_ERROR_NONE) {
        return;
    }
    std::fprintf(stderr, "libbgt error %d: %s\n", s.error_code,
                 s.error_message.c_str());
}

void bgt_draw_error(int x, int y, int size)
{
    State &s = state();
    if (s.error_code == BGT_ERROR_NONE || s.error_message.empty()) {
        return;
    }
    draw_text_impl(s, x, y, s.error_message.c_str(), size);
}

void bgt_clear_error()
{
    state().clear_error();
}

// NOLINTEND(readability-magic-numbers, modernize-avoid-c-arrays,
// bugprone-easily-swappable-parameters, readability-identifier-length)
