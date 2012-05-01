/*
 * vim:ts=4:sw=4:expandtab
 *
 * © 2010-2012 Michael Stapelberg
 *
 * See LICENSE for licensing information
 *
 */
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>
#include <ev.h>

#ifndef NOLIBCAIRO
#include <cairo.h>
#include <cairo/cairo-xcb.h>
#endif

#include "xcb.h"
#include "unlock_indicator.h"
#include "xinerama.h"

#define BUTTON_RADIUS 90
#define BUTTON_SPACE (BUTTON_RADIUS + 5)
#define BUTTON_CENTER (BUTTON_RADIUS + 5)
#define BUTTON_DIAMETER (2 * BUTTON_SPACE)

/*******************************************************************************
 * Variables defined in i3lock.c.
 ******************************************************************************/

/* The current position in the input buffer. Useful to determine if any
 * characters of the password have already been entered or not. */
int input_position;

/* The ev main loop. */
struct ev_loop *main_loop;

/* The lock window. */
extern xcb_window_t win;

/* The current resolution of the X11 root window. */
extern uint32_t last_resolution[2];

/* Whether the unlock indicator is enabled (defaults to true). */
extern bool unlock_indicator;

#ifndef NOLIBCAIRO
/* A Cairo surface containing the specified image (-i), if any. */
extern cairo_surface_t *img;
#endif

/* Whether the image should be tiled. */
extern bool tile;
/* The background color to use (in hex). */
extern char color[7];
/* indicator color options */
extern char insidevercolor[9];
extern char insidewrongcolor[9];
extern char insidecolor[9];
extern char ringvercolor[9];
extern char ringwrongcolor[9];
extern char ringcolor[9];
extern char linecolor[9];
extern char textcolor[9];
extern char keyhlcolor[9];
extern char bshlcolor[9];

/*******************************************************************************
 * Local variables.
 ******************************************************************************/

static struct ev_timer *clear_indicator_timeout;

/* Cache the screen’s visual, necessary for creating a Cairo context. */
static xcb_visualtype_t *vistype;

/* Maintain the current unlock/PAM state to draw the appropriate unlock
 * indicator. */
unlock_state_t unlock_state;
pam_state_t pam_state;

/*
 * Draws global image with fill color onto a pixmap with the given
 * resolution and returns it.
 *
 */
xcb_pixmap_t draw_image(uint32_t *resolution) {
    xcb_pixmap_t bg_pixmap = XCB_NONE;

#ifndef NOLIBCAIRO
    if (!vistype)
        vistype = get_root_visual_type(screen);
    bg_pixmap = create_bg_pixmap(conn, screen, resolution, color);
    /* Initialize cairo: Create one in-memory surface to render the unlock
     * indicator on, create one XCB surface to actually draw (one or more,
     * depending on the amount of screens) unlock indicators on. */
    cairo_surface_t *output = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, BUTTON_DIAMETER, BUTTON_DIAMETER);
    cairo_t *ctx = cairo_create(output);

    cairo_surface_t *xcb_output = cairo_xcb_surface_create(conn, bg_pixmap, vistype, resolution[0], resolution[1]);
    cairo_t *xcb_ctx = cairo_create(xcb_output);

    if (img) {
        if (!tile) {
            cairo_set_source_surface(xcb_ctx, img, 0, 0);
            cairo_paint(xcb_ctx);
        } else {
            /* create a pattern and fill a rectangle as big as the screen */
            cairo_pattern_t *pattern;
            pattern = cairo_pattern_create_for_surface(img);
            cairo_set_source(xcb_ctx, pattern);
            cairo_pattern_set_extend(pattern, CAIRO_EXTEND_REPEAT);
            cairo_rectangle(xcb_ctx, 0, 0, resolution[0], resolution[1]);
            cairo_fill(xcb_ctx);
            cairo_pattern_destroy(pattern);
        }
    } else {
        char strgroups[3][3] = {{color[0], color[1], '\0'},
                                {color[2], color[3], '\0'},
                                {color[4], color[5], '\0'}};
        uint32_t rgb16[3] = {(strtol(strgroups[0], NULL, 16)),
                             (strtol(strgroups[1], NULL, 16)),
                             (strtol(strgroups[2], NULL, 16))};
        cairo_set_source_rgb(xcb_ctx, rgb16[0] / 255.0, rgb16[1] / 255.0, rgb16[2] / 255.0);
        cairo_rectangle(xcb_ctx, 0, 0, resolution[0], resolution[1]);
        cairo_fill(xcb_ctx);
    }

#ifndef NOLIBCAIRO
    /* build indicator color arrays */
    char strgroupsiv[4][3] = {{insidevercolor[0], insidevercolor[1], '\0'},
                              {insidevercolor[2], insidevercolor[3], '\0'},
                              {insidevercolor[4], insidevercolor[5], '\0'},
                              {insidevercolor[6], insidevercolor[7], '\0'}};
    uint32_t insidever16[4] = {(strtol(strgroupsiv[0], NULL, 16)),
                               (strtol(strgroupsiv[1], NULL, 16)),
                               (strtol(strgroupsiv[2], NULL, 16)),
                               (strtol(strgroupsiv[3], NULL, 16))};
    char strgroupsiw[4][3] = {{insidewrongcolor[0], insidewrongcolor[1], '\0'},
                              {insidewrongcolor[2], insidewrongcolor[3], '\0'},
                              {insidewrongcolor[4], insidewrongcolor[5], '\0'},
                              {insidewrongcolor[6], insidewrongcolor[7], '\0'}};
    uint32_t insidewrong16[4] = {(strtol(strgroupsiw[0], NULL, 16)),
                                 (strtol(strgroupsiw[1], NULL, 16)),
                                 (strtol(strgroupsiw[2], NULL, 16)),
                                 (strtol(strgroupsiw[3], NULL, 16))};
    char strgroupsi[4][3] = {{insidecolor[0], insidecolor[1], '\0'},
                             {insidecolor[2], insidecolor[3], '\0'},
                             {insidecolor[4], insidecolor[5], '\0'},
                             {insidecolor[6], insidecolor[7], '\0'}};
    uint32_t inside16[4] = {(strtol(strgroupsi[0], NULL, 16)),
                            (strtol(strgroupsi[1], NULL, 16)),
                            (strtol(strgroupsi[2], NULL, 16)),
                            (strtol(strgroupsi[3], NULL, 16))};
    char strgroupsrv[4][3] = {{ringvercolor[0], ringvercolor[1], '\0'},
                              {ringvercolor[2], ringvercolor[3], '\0'},
                              {ringvercolor[4], ringvercolor[5], '\0'},
                              {ringvercolor[6], ringvercolor[7], '\0'}};
    uint32_t ringver16[4] = {(strtol(strgroupsrv[0], NULL, 16)),
                             (strtol(strgroupsrv[1], NULL, 16)),
                             (strtol(strgroupsrv[2], NULL, 16)),
                             (strtol(strgroupsrv[3], NULL, 16))};
    char strgroupsrw[4][3] = {{ringwrongcolor[0], ringwrongcolor[1], '\0'},
                              {ringwrongcolor[2], ringwrongcolor[3], '\0'},
                              {ringwrongcolor[4], ringwrongcolor[5], '\0'},
                              {ringwrongcolor[6], ringwrongcolor[7], '\0'}};
    uint32_t ringwrong16[4] = {(strtol(strgroupsrw[0], NULL, 16)),
                               (strtol(strgroupsrw[1], NULL, 16)),
                               (strtol(strgroupsrw[2], NULL, 16)),
                               (strtol(strgroupsrw[3], NULL, 16))};
    char strgroupsr[4][3] = {{ringcolor[0], ringcolor[1], '\0'},
                             {ringcolor[2], ringcolor[3], '\0'},
                             {ringcolor[4], ringcolor[5], '\0'},
                             {ringcolor[6], ringcolor[7], '\0'}};
    uint32_t ring16[4] = {(strtol(strgroupsr[0], NULL, 16)),
                          (strtol(strgroupsr[1], NULL, 16)),
                          (strtol(strgroupsr[2], NULL, 16)),
                          (strtol(strgroupsr[3], NULL, 16))};
    char strgroupsl[4][3] = {{linecolor[0], linecolor[1], '\0'},
                             {linecolor[2], linecolor[3], '\0'},
                             {linecolor[4], linecolor[5], '\0'},
                             {linecolor[6], linecolor[7], '\0'}};
    uint32_t line16[4] = {(strtol(strgroupsl[0], NULL, 16)),
                          (strtol(strgroupsl[1], NULL, 16)),
                          (strtol(strgroupsl[2], NULL, 16)),
                          (strtol(strgroupsl[3], NULL, 16))};
    char strgroupst[4][3] = {{textcolor[0], textcolor[1], '\0'},
                             {textcolor[2], textcolor[3], '\0'},
                             {textcolor[4], textcolor[5], '\0'},
                             {textcolor[6], textcolor[7], '\0'}};
    uint32_t text16[4] = {(strtol(strgroupst[0], NULL, 16)),
                          (strtol(strgroupst[1], NULL, 16)),
                          (strtol(strgroupst[2], NULL, 16)),
                          (strtol(strgroupst[3], NULL, 16))};
    char strgroupsk[4][3] = {{keyhlcolor[0], textcolor[1], '\0'},
                             {keyhlcolor[2], textcolor[3], '\0'},
                             {keyhlcolor[4], textcolor[5], '\0'},
                             {keyhlcolor[6], textcolor[7], '\0'}};
    uint32_t keyhl16[4] = {(strtol(strgroupsk[0], NULL, 16)),
                           (strtol(strgroupsk[1], NULL, 16)),
                           (strtol(strgroupsk[2], NULL, 16)),
                           (strtol(strgroupsk[3], NULL, 16))};
    char strgroupsb[4][3] = {{bshlcolor[0], textcolor[1], '\0'},
                             {bshlcolor[2], textcolor[3], '\0'},
                             {bshlcolor[4], textcolor[5], '\0'},
                             {bshlcolor[6], textcolor[7], '\0'}};
    uint32_t bshl16[4] = {(strtol(strgroupsb[0], NULL, 16)),
                          (strtol(strgroupsb[1], NULL, 16)),
                          (strtol(strgroupsb[2], NULL, 16)),
                          (strtol(strgroupsb[3], NULL, 16))};
#endif

    if (unlock_state >= STATE_KEY_PRESSED && unlock_indicator) {
        /* Draw a (centered) circle with transparent background. */
        cairo_set_line_width(ctx, 10.0);
        cairo_arc(ctx,
                  BUTTON_CENTER /* x */,
                  BUTTON_CENTER /* y */,
                  BUTTON_RADIUS /* radius */,
                  0 /* start */,
                  2 * M_PI /* end */);

        /* Use the appropriate color for the different PAM states
         * (currently verifying, wrong password, or default) */
        switch (pam_state) {
            case STATE_PAM_VERIFY:
                cairo_set_source_rgba(ctx, (double)insidever16[0]/255, (double)insidever16[1]/255, (double)insidever16[2]/255, (double)insidever16[3]/255);
                break;
            case STATE_PAM_WRONG:
                cairo_set_source_rgba(ctx, (double)insidewrong16[0]/255, (double)insidewrong16[1]/255, (double)insidewrong16[2]/255, (double)insidewrong16[3]/255);
                break;
            default:
                cairo_set_source_rgba(ctx, (double)inside16[0]/255, (double)inside16[1]/255, (double)inside16[2]/255, (double)inside16[3]/255);
                break;
        }
        cairo_fill_preserve(ctx);

        switch (pam_state) {
            case STATE_PAM_VERIFY:
                cairo_set_source_rgba(ctx, (double)ringver16[0]/255, (double)ringver16[1]/255, (double)ringver16[2]/255, (double)ringver16[3]/255);
                break;
            case STATE_PAM_WRONG:
                cairo_set_source_rgba(ctx, (double)ringwrong16[0]/255, (double)ringwrong16[1]/255, (double)ringwrong16[2]/255, (double)ringwrong16[3]/255);
                break;
            case STATE_PAM_IDLE:
                cairo_set_source_rgba(ctx, (double)ring16[0]/255, (double)ring16[1]/255, (double)ring16[2]/255, (double)ring16[3]/255);
                break;
        }
        cairo_stroke(ctx);

        /* Draw an inner seperator line. */
        cairo_set_source_rgba(ctx, (double)line16[0]/255, (double)line16[1]/255, (double)line16[2]/255, (double)line16[3]/255);
        cairo_set_line_width(ctx, 2.0);
        cairo_arc(ctx,
                  BUTTON_CENTER /* x */,
                  BUTTON_CENTER /* y */,
                  BUTTON_RADIUS - 5 /* radius */,
                  0,
                  2 * M_PI);
        cairo_stroke(ctx);

        cairo_set_line_width(ctx, 10.0);

        /* Display a (centered) text of the current PAM state. */
        char *text = NULL;
        switch (pam_state) {
            case STATE_PAM_VERIFY:
                text = "verifying…";
                break;
            case STATE_PAM_WRONG:
                text = "wrong!";
                break;
            default:
                break;
        }

        if (text) {
            cairo_text_extents_t extents;
            double x, y;

            cairo_set_source_rgba(ctx, (double)text16[0]/255, (double)text16[1]/255, (double)text16[2]/255, (double)text16[3]/255);
            cairo_set_font_size(ctx, 28.0);

            cairo_text_extents(ctx, text, &extents);
            x = BUTTON_CENTER - ((extents.width / 2) + extents.x_bearing);
            y = BUTTON_CENTER - ((extents.height / 2) + extents.y_bearing);

            cairo_move_to(ctx, x, y);
            cairo_show_text(ctx, text);
            cairo_close_path(ctx);
        }

        /* After the user pressed any valid key or the backspace key, we
         * highlight a random part of the unlock indicator to confirm this
         * keypress. */
        if (unlock_state == STATE_KEY_ACTIVE ||
            unlock_state == STATE_BACKSPACE_ACTIVE) {
            cairo_new_sub_path(ctx);
            double highlight_start = (rand() % (int)(2 * M_PI * 100)) / 100.0;
            cairo_arc(ctx,
                      BUTTON_CENTER /* x */,
                      BUTTON_CENTER /* y */,
                      BUTTON_RADIUS /* radius */,
                      highlight_start,
                      highlight_start + (M_PI / 3.0));
            if (unlock_state == STATE_KEY_ACTIVE) {
                /* For normal keys, we use a lighter green. */
                cairo_set_source_rgba(ctx, (double)keyhl16[0]/255, (double)keyhl16[1]/255, (double)keyhl16[2]/255, (double)keyhl16[3]/255);
            } else {
                /* For backspace, we use red. */
                cairo_set_source_rgba(ctx, (double)bshl16[0]/255, (double)bshl16[1]/255, (double)bshl16[2]/255, (double)bshl16[3]/255);
            }
            cairo_stroke(ctx);

            /* Draw two little separators for the highlighted part of the
             * unlock indicator. */
            cairo_set_source_rgba(ctx, (double)line16[0]/255, (double)line16[1]/255, (double)line16[2]/255, (double)line16[3]/255);
            cairo_arc(ctx,
                      BUTTON_CENTER /* x */,
                      BUTTON_CENTER /* y */,
                      BUTTON_RADIUS /* radius */,
                      highlight_start /* start */,
                      highlight_start + (M_PI / 128.0) /* end */);
            cairo_stroke(ctx);
            cairo_arc(ctx,
                      BUTTON_CENTER /* x */,
                      BUTTON_CENTER /* y */,
                      BUTTON_RADIUS /* radius */,
                      highlight_start + (M_PI / 3.0) /* start */,
                      (highlight_start + (M_PI / 3.0)) + (M_PI / 128.0) /* end */);
            cairo_stroke(ctx);
        }
    }

    if (xr_screens > 0) {
        /* Composite the unlock indicator in the middle of each screen. */
        for (int screen = 0; screen < xr_screens; screen++) {
            int x = (xr_resolutions[screen].x + ((xr_resolutions[screen].width / 2) - (BUTTON_DIAMETER / 2)));
            int y = (xr_resolutions[screen].y + ((xr_resolutions[screen].height / 2) - (BUTTON_DIAMETER / 2)));
            cairo_set_source_surface(xcb_ctx, output, x, y);
            cairo_rectangle(xcb_ctx, x, y, BUTTON_DIAMETER, BUTTON_DIAMETER);
            cairo_fill(xcb_ctx);
        }
    } else {
        /* We have no information about the screen sizes/positions, so we just
         * place the unlock indicator in the middle of the X root window and
         * hope for the best. */
        int x = (last_resolution[0] / 2);
        int y = (last_resolution[1] / 2);
        cairo_set_source_surface(xcb_ctx, output, x, y);
        cairo_rectangle(xcb_ctx, x, y, BUTTON_DIAMETER, BUTTON_DIAMETER);
        cairo_fill(xcb_ctx);
    }

    cairo_surface_destroy(xcb_output);
    cairo_surface_destroy(output);
    cairo_destroy(ctx);
    cairo_destroy(xcb_ctx);
#endif
    return bg_pixmap;
}

/*
 * Calls draw_image on a new pixmap and swaps that with the current pixmap
 *
 */
void redraw_screen(void) {
    xcb_pixmap_t bg_pixmap = draw_image(last_resolution);
    xcb_change_window_attributes(conn, win, XCB_CW_BACK_PIXMAP, (uint32_t[1]){ bg_pixmap });
    /* XXX: Possible optimization: Only update the area in the middle of the
     * screen instead of the whole screen. */
    xcb_clear_area(conn, 0, win, 0, 0, screen->width_in_pixels, screen->height_in_pixels);
    xcb_free_pixmap(conn, bg_pixmap);
    xcb_flush(conn);
}

/*
 * Hides the unlock indicator completely when there is no content in the
 * password buffer.
 *
 */
static void clear_indicator(EV_P_ ev_timer *w, int revents) {
    if (input_position == 0) {
        unlock_state = STATE_STARTED;
    } else unlock_state = STATE_KEY_PRESSED;
    redraw_screen();

    ev_timer_stop(main_loop, clear_indicator_timeout);
    free(clear_indicator_timeout);
    clear_indicator_timeout = NULL;
}

/*
 * (Re-)starts the clear_indicator timeout. Called after pressing backspace or
 * after an unsuccessful authentication attempt.
 *
 */
void start_clear_indicator_timeout(void) {
    if (clear_indicator_timeout) {
        ev_timer_stop(main_loop, clear_indicator_timeout);
        ev_timer_set(clear_indicator_timeout, 1.0, 0.);
        ev_timer_start(main_loop, clear_indicator_timeout);
    } else {
        /* When there is no memory, we just don’t have a timeout. We cannot
         * exit() here, since that would effectively unlock the screen. */
        if (!(clear_indicator_timeout = calloc(sizeof(struct ev_timer), 1)))
            return;
        ev_timer_init(clear_indicator_timeout, clear_indicator, 1.0, 0.);
        ev_timer_start(main_loop, clear_indicator_timeout);
    }
}

/*
 * Stops the clear_indicator timeout.
 *
 */
void stop_clear_indicator_timeout(void) {
    if (clear_indicator_timeout) {
        ev_timer_stop(main_loop, clear_indicator_timeout);
        free(clear_indicator_timeout);
        clear_indicator_timeout = NULL;
    }
}
