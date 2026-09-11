#include "app_tic_tac_toe.h"
#include "nino_colors.h"
#include "utils/sd_utils.h"
#include "storage/storage.h"
#include <Arduino.h>
#include <string.h>

#define X_IMG "/images/nino_xo.bin"

#define CELL      60
#define CGAP       4
#define BORD       2
#define MARK      10               // visual inset of a mark from the cell edge
#define XOFF      (MARK - BORD)    // mark inset in content coords (centers the X)
#define XO        (CELL - 2 * MARK) // mark box size
#define BOARD_SZ  (3 * CELL + 2 * CGAP)

struct WinLine { int8_t a, b, c; };
static const WinLine WINS[8] = {
    {0,1,2}, {3,4,5}, {6,7,8},
    {0,3,6}, {1,4,7}, {2,5,8},
    {0,4,8}, {2,4,6}
};

static int8_t board[9];
static bool vs_computer;
static bool x_turn;
static bool game_over;
static bool locked;
static int score_x, score_o;

static lv_obj_t *g_content;
static lv_obj_t *cells[9];
static lv_obj_t *xl1[9], *xl2[9], *oc[9], *xi[9];
static bool use_x_img;
static lv_obj_t *status_lab;
static lv_obj_t *score_lab;
static lv_obj_t *overlay;
static lv_timer_t *ai_timer;

static void cancel_ai(void)
{
    if (ai_timer) { lv_timer_del(ai_timer); ai_timer = nullptr; }
}

static int check_winner(void)
{
    for (int i = 0; i < 8; i++) {
        int a = WINS[i].a, b = WINS[i].b, c = WINS[i].c;
        if (board[a] && board[a] == board[b] && board[a] == board[c])
            return board[a];
    }
    return 0;
}

static bool board_full(void)
{
    for (int i = 0; i < 9; i++) if (!board[i]) return false;
    return true;
}

static int find_move(int p)
{
    for (int i = 0; i < 8; i++) {
        int a = WINS[i].a, b = WINS[i].b, c = WINS[i].c;
        if (board[a] == p && board[b] == p && !board[c]) return c;
        if (board[a] == p && board[c] == p && !board[b]) return b;
        if (board[b] == p && board[c] == p && !board[a]) return a;
    }
    return -1;
}

static int ai_decide(void)
{
    int m;
    if ((m = find_move(2)) >= 0) return m;
    if ((m = find_move(1)) >= 0) return m;
    if (!board[4]) return 4;
    int av[9], n = 0;
    for (int i = 0; i < 9; i++) if (!board[i]) av[n++] = i;
    return av[random(n)];
}

static void update_score(void)
{
    char buf[16];
    snprintf(buf, sizeof(buf), "X:%d  O:%d", score_x, score_o);
    lv_label_set_text(score_lab, buf);
}

static void update_status(const char *s) { lv_label_set_text(status_lab, s); }

static void show_mark(int cell, int p)
{
    if (p == 1) {
        if (use_x_img) {
            lv_obj_clear_flag(xi[cell], LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_clear_flag(xl1[cell], LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(xl2[cell], LV_OBJ_FLAG_HIDDEN);
        }
    } else {
        lv_obj_clear_flag(oc[cell], LV_OBJ_FLAG_HIDDEN);
    }
}

static void clear_visuals(void)
{
    for (int i = 0; i < 9; i++) {
        lv_obj_add_flag(xl1[i], LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(xl2[i], LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(oc[i],  LV_OBJ_FLAG_HIDDEN);
        if (xi[i]) lv_obj_add_flag(xi[i], LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_bg_color(cells[i], lv_color_hex(0xFFFFFF), 0);
    }
}

static void check_end(void)
{
    int w = check_winner();
    if (w) {
        game_over = true;
        for (int i = 0; i < 8; i++) {
            int a = WINS[i].a, b = WINS[i].b, c = WINS[i].c;
            if (board[a] == w && board[b] == w && board[c] == w) {
                lv_obj_set_style_bg_color(cells[a], NINO_COLOR_SUCCESS, 0);
                lv_obj_set_style_bg_color(cells[b], NINO_COLOR_SUCCESS, 0);
                lv_obj_set_style_bg_color(cells[c], NINO_COLOR_SUCCESS, 0);
                break;
            }
        }
        if (w == 1) { score_x++; update_status("X wins!"); }
        else        { score_o++; update_status("O wins!"); }
        update_score();
    } else if (board_full()) {
        game_over = true;
        update_status("It's a draw!");
    }
}

static void new_game(void)
{
    cancel_ai();
    memset(board, 0, sizeof(board));
    game_over = false;
    x_turn    = true;
    locked    = false;
    clear_visuals();
    update_status(vs_computer ? "Your turn" : "X's turn");
}

static void do_ai_move(void)
{
    int m = ai_decide();
    board[m] = 2;
    show_mark(m, 2);
    check_end();
    if (!game_over) {
        x_turn = true;
        update_status("Your turn");
    }
    locked = false;
}

static void on_ai_timer(lv_timer_t *)
{
    ai_timer = nullptr;
    do_ai_move();
}

static void on_cell_tap(lv_event_t *e)
{
    if (locked || game_over) return;
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    if (board[idx]) return;

    int player = x_turn ? 1 : 2;
    board[idx] = player;
    show_mark(idx, player);
    check_end();
    if (game_over) return;

    if (vs_computer) {
        x_turn = false;
        locked = true;
        update_status("Computer's turn");
        ai_timer = lv_timer_create(on_ai_timer, 400, nullptr);
        lv_timer_set_repeat_count(ai_timer, 1);
    } else {
        x_turn = !x_turn;
        update_status(x_turn ? "X's turn" : "O's turn");
    }
}

static void on_new_game(lv_event_t *e) { (void)e; new_game(); }

static void show_overlay(void);

static void on_mode(lv_event_t *e)
{
    (void)e;
    cancel_ai();
    show_overlay();
}

static void on_mode_pick(lv_event_t *e)
{
    vs_computer = ((int)(intptr_t)lv_event_get_user_data(e) == 0);
    lv_obj_del(overlay);
    overlay = nullptr;
    score_x = score_o = 0;
    update_score();
    new_game();
}

static void show_overlay(void)
{
    overlay = lv_obj_create(g_content);
    lv_obj_remove_style_all(overlay);
    lv_obj_set_size(overlay, 480, 276);
    lv_obj_align(overlay, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(overlay, lv_color_hex(0xFFF8F0), 0);
    lv_obj_set_style_bg_opa(overlay, LV_OPA_COVER, 0);
    lv_obj_clear_flag(overlay, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *tl = lv_label_create(overlay);
    lv_label_set_text(tl, "Choose mode");
    lv_obj_set_style_text_font(tl, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(tl, lv_color_hex(0x333333), 0);
    lv_obj_align(tl, LV_ALIGN_TOP_MID, 0, 40);

    static const char *labels[]  = { "Computer", "Two Players" };
    static const lv_color_t bg[] = { lv_color_hex(0x00B4D8), lv_color_hex(0xA8E6CF) };

    int bw = 200, bh = 64, gap = 20;
    int ox = (480 - 2 * bw - gap) / 2;

    for (int i = 0; i < 2; i++) {
        lv_obj_t *b = lv_btn_create(overlay);
        lv_obj_set_size(b, bw, bh);
        lv_obj_set_pos(b, ox + i * (bw + gap), 110);
        lv_obj_set_style_radius(b, 20, 0);
        lv_obj_set_style_shadow_width(b, 0, 0);
        lv_obj_set_style_bg_color(b, bg[i], 0);

        lv_obj_t *l = lv_label_create(b);
        lv_label_set_text(l, labels[i]);
        lv_obj_set_style_text_font(l, &lv_font_montserrat_20, 0);
        lv_obj_set_style_text_color(l, lv_color_hex(0x333333), 0);
        lv_obj_center(l);

        lv_obj_add_event_cb(b, on_mode_pick, LV_EVENT_CLICKED,
                            (void *)(intptr_t)i);
    }
}

void app_tic_tac_toe_create(lv_obj_t *content)
{
    g_content = content;
    score_x = score_o = 0;
    ai_timer = nullptr;
    overlay  = nullptr;
    use_x_img = nino_sd_is_mounted() && nino_storage_exists(X_IMG);

    lv_obj_set_style_bg_color(content, lv_color_hex(0xFFF8F0), 0);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_add_event_cb(content, [](lv_event_t *) { cancel_ai(); },
                        LV_EVENT_DELETE, nullptr);

    // ---- Score (top-left) ----
    score_lab = lv_label_create(content);
    lv_label_set_text(score_lab, "X:0  O:0");
    lv_obj_set_style_text_font(score_lab, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(score_lab, lv_color_hex(0x888888), 0);
    lv_obj_align(score_lab, LV_ALIGN_TOP_LEFT, 10, 4);

    // ---- Status (top-center) ----
    status_lab = lv_label_create(content);
    lv_label_set_text(status_lab, "");
    lv_obj_set_style_text_font(status_lab, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(status_lab, lv_color_hex(0x333333), 0);
    lv_obj_align(status_lab, LV_ALIGN_TOP_MID, 0, 2);

    // ---- 3×3 board ----
    int bx = (464 - BOARD_SZ) / 2;
    int by = 28;

    for (int i = 0; i < 9; i++) {
        int r = i / 3, c = i % 3;
        int cx = bx + c * (CELL + CGAP);
        int cy = by + r * (CELL + CGAP);

cells[i] = lv_btn_create(content);
        lv_obj_set_size(cells[i], CELL, CELL);
        lv_obj_set_pos(cells[i], cx, cy);
        lv_obj_set_style_pad_all(cells[i], 0, 0);
        lv_obj_set_style_radius(cells[i], 10, 0);
        lv_obj_set_style_bg_color(cells[i], lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_bg_opa(cells[i], LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(cells[i], BORD, 0);
        lv_obj_set_style_border_color(cells[i], lv_color_hex(0xDDDDDD), 0);
        lv_obj_set_style_shadow_width(cells[i], 0, 0);
        lv_obj_add_event_cb(cells[i], on_cell_tap, LV_EVENT_CLICKED,
                            (void *)(intptr_t)i);

        // X line 1 (\)
        xl1[i] = lv_line_create(cells[i]);
        static lv_point_precise_t x1p[] = {{XOFF, XOFF}, {XOFF + XO, XOFF + XO}};
        lv_line_set_points(xl1[i], x1p, 2);
        lv_obj_set_style_line_width(xl1[i], 4, 0);
        lv_obj_set_style_line_color(xl1[i], NINO_COLOR_DANGER, 0);
        lv_obj_add_flag(xl1[i], LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(xl1[i], LV_OBJ_FLAG_CLICKABLE);

        // X line 2 (/)
        xl2[i] = lv_line_create(cells[i]);
        static lv_point_precise_t x2p[] = {{XOFF, XOFF + XO}, {XOFF + XO, XOFF}};
        lv_line_set_points(xl2[i], x2p, 2);
        lv_obj_set_style_line_width(xl2[i], 4, 0);
        lv_obj_set_style_line_color(xl2[i], NINO_COLOR_DANGER, 0);
        lv_obj_add_flag(xl2[i], LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(xl2[i], LV_OBJ_FLAG_CLICKABLE);

        // O circle
        oc[i] = lv_obj_create(cells[i]);
        lv_obj_remove_style_all(oc[i]);
        lv_obj_set_size(oc[i], XO, XO);
        lv_obj_align(oc[i], LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_radius(oc[i], LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_border_width(oc[i], 4, 0);
        lv_obj_set_style_border_color(oc[i], NINO_COLOR_BLUE, 0);
        lv_obj_set_style_bg_opa(oc[i], LV_OPA_TRANSP, 0);
        lv_obj_add_flag(oc[i], LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(oc[i], LV_OBJ_FLAG_CLICKABLE);

        // X image (from SD) replaces lines when available
        xi[i] = nullptr;
        if (use_x_img) {
            xi[i] = lv_image_create(cells[i]);
            lv_image_set_src(xi[i], "S:" X_IMG);
            lv_obj_set_size(xi[i], XO, XO);
            lv_obj_align(xi[i], LV_ALIGN_CENTER, 0, 0);
            lv_obj_add_flag(xi[i], LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(xi[i], LV_OBJ_FLAG_CLICKABLE);
            lv_obj_add_flag(xl1[i], LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(xl2[i], LV_OBJ_FLAG_HIDDEN);
        }
    }

    // ---- Bottom buttons ----
    int bw = 120, bh = 32;
    int btn_y = by + BOARD_SZ + 8;

    lv_obj_t *ng = lv_btn_create(content);
    lv_obj_set_size(ng, bw, bh);
    lv_obj_set_pos(ng, (464 / 2) - bw - 6, btn_y);
    lv_obj_set_style_radius(ng, 16, 0);
    lv_obj_set_style_bg_color(ng, NINO_COLOR_PRIMARY, 0);
    lv_obj_set_style_shadow_width(ng, 0, 0);
    lv_obj_add_event_cb(ng, on_new_game, LV_EVENT_CLICKED, nullptr);
    lv_obj_t *nl = lv_label_create(ng);
    lv_label_set_text(nl, "New Game");
    lv_obj_set_style_text_font(nl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(nl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(nl);

    lv_obj_t *mb = lv_btn_create(content);
    lv_obj_set_size(mb, bw, bh);
    lv_obj_set_pos(mb, (464 / 2) + 6, btn_y);
    lv_obj_set_style_radius(mb, 16, 0);
    lv_obj_set_style_bg_color(mb, NINO_COLOR_PRIMARY, 0);
    lv_obj_set_style_shadow_width(mb, 0, 0);
    lv_obj_add_event_cb(mb, on_mode, LV_EVENT_CLICKED, nullptr);
    lv_obj_t *ml = lv_label_create(mb);
    lv_label_set_text(ml, "Mode");
    lv_obj_set_style_text_font(ml, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(ml, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(ml);

    // ---- Start with mode picker ----
    show_overlay();
}