/*============================================================================
Copyright (c) 2025 Raspberry Pi
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================*/

#include <math.h>
#include <locale.h>
#include <glib/gi18n.h>

#ifdef LXPLUG
#include "plugin.h"
#else
#include "lxutils.h"
#endif

#include "clock.h"

/*----------------------------------------------------------------------------*/
/* Typedefs and macros                                                        */
/*----------------------------------------------------------------------------*/

#define DEFAULT_TIME_FORMAT     "%R"
#define DEFAULT_DATE_FORMAT     "%A %x"

/*----------------------------------------------------------------------------*/
/* Global data                                                                */
/*----------------------------------------------------------------------------*/

conf_table_t conf_table[8] = {
    {CONF_TYPE_STRING,  "time_format",  N_("Time format"),      NULL},
    {CONF_TYPE_STRING,  "date_format",  N_("Date format"),      NULL},
    {CONF_TYPE_FONT,    "font",         N_("Clock font"),       NULL},
    {CONF_TYPE_BOOL,    "custom_font",  N_("Use custom font"),  NULL},
    {CONF_TYPE_BOOL,    "analogue",     N_("Analogue clock"),   NULL},
    {CONF_TYPE_COLOUR,  "face_col",     N_("Colour of face"),   NULL},
    {CONF_TYPE_COLOUR,  "hands_col",    N_("Colour of hands"),  NULL},
    {CONF_TYPE_NONE,    NULL,           NULL,                   NULL}
};

/*----------------------------------------------------------------------------*/
/* Prototypes                                                                 */
/*----------------------------------------------------------------------------*/

static void show_calendar (ClockPlugin *clk);
static gboolean handle_popup_keypress (GtkWidget *, GdkEventKey *event, gpointer user_data);
static void cal_destroyed (GtkWidget *, gpointer user_data);
static void draw_face (ClockPlugin *clk, int hr, int min);
static gboolean clock_tick (ClockPlugin *clk);
#ifndef LXPLUG
static gboolean clock_button_pressed (GtkWidget *, GdkEventButton *, ClockPlugin *clk);
#endif
static void clock_button_clicked (GtkWidget *, ClockPlugin *clk);

/*----------------------------------------------------------------------------*/
/* Function definitions                                                       */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/* Calendar pop-up                                                            */
/*----------------------------------------------------------------------------*/

static void show_calendar (ClockPlugin *clk)
{
    clk->calendar_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

    gtk_window_set_decorated (GTK_WINDOW (clk->calendar_window), FALSE);
    gtk_window_set_resizable (GTK_WINDOW (clk->calendar_window), FALSE);
    gtk_container_set_border_width (GTK_CONTAINER (clk->calendar_window), 1);
    gtk_window_set_skip_taskbar_hint (GTK_WINDOW (clk->calendar_window), TRUE);
    gtk_window_set_skip_pager_hint (GTK_WINDOW (clk->calendar_window), TRUE);
    gtk_widget_set_name (clk->calendar_window, "panelpopup");

    /* Create a standard calendar widget as a child of the vertical box. */
    GtkWidget *calendar = gtk_calendar_new ();
    gtk_container_add (GTK_CONTAINER (clk->calendar_window), calendar);
    g_signal_connect (calendar, "key-press-event", G_CALLBACK (handle_popup_keypress), clk);

    g_signal_connect (clk->calendar_window, "destroy", G_CALLBACK (cal_destroyed), clk);

    wrap_popup_at_button (clk, clk->calendar_window, clk->plugin);
}

static gboolean handle_popup_keypress (GtkWidget *, GdkEventKey *event, gpointer user_data)
{
    ClockPlugin *clk = (ClockPlugin *) user_data;

    if (event->keyval == GDK_KEY_Escape)
    {
#ifdef LXPLUG
        if (clk->calendar_window) gtk_widget_destroy (clk->calendar_window);
#else
        close_popup ();
#endif
        return TRUE;
    }
    return FALSE;
}

static void cal_destroyed (GtkWidget *, gpointer user_data)
{
    ClockPlugin *clk = (ClockPlugin *) user_data;
    clk->calendar_window = NULL;
}

/*----------------------------------------------------------------------------*/
/* Analogue clock                                                             */
/*----------------------------------------------------------------------------*/

static void draw_face (ClockPlugin *clk, int hr, int min)
{
    int ic, hm, scale;;
    double mid, wid, r, th;
    double twopi = 2.0 * M_PI;

    // calculate dimensions based on icon size
    scale = gtk_widget_get_scale_factor (clk->clock_ana);
    ic = wrap_icon_size (clk) - 2;
    mid = ic / 2;
    wid = mid / 16;

    // create the drawing surface
    cairo_surface_t *surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, ic * scale, ic * scale);
    cairo_surface_set_device_scale (surface, scale, scale);
    cairo_t *cr = cairo_create (surface);

    // draw circle on surface
    cairo_set_source_rgb (cr, clk->face_col.red, clk->face_col.green, clk->face_col.blue);
    cairo_arc (cr, mid, mid, mid, 0, twopi);
    cairo_fill (cr);

    // draw border
    cairo_set_source_rgb (cr, clk->hands_col.red, clk->hands_col.green, clk->hands_col.blue);
    cairo_set_line_width (cr, wid);
    cairo_arc (cr, mid, mid, mid - (wid / 2.0), 0, twopi);
    cairo_stroke (cr);

    // draw markings
    cairo_set_line_width (cr, wid / 4.0);
    for (hm = 0; hm < 12; hm++)
    {
        th = hm * twopi / 12.0;
        r = mid * ((hm % 3 == 0) ? 0.6 : 0.75);
        cairo_move_to (cr, mid + (cos (th) * mid), mid + (sin (th) * mid));
        cairo_line_to (cr, mid + (cos (th) * r), mid + (sin (th) * r));
        cairo_stroke (cr);
    }

    // draw hands
    cairo_set_line_width (cr, wid);
    th = (min - 15) * twopi / 60.0;
    r = mid * 0.85;
    cairo_move_to (cr, mid, mid);
    cairo_line_to (cr, mid + (cos (th) * r), mid + (sin (th) * r));
    cairo_stroke (cr);

    th = hr * 1.0 + min / 60.0;
    th = (th - 3.0) * twopi / 12.0;
    r = mid * 0.5;
    cairo_move_to (cr, mid, mid);
    cairo_line_to (cr, mid + (cos (th) * r), mid + (sin (th) * r));
    cairo_stroke (cr);

    // draw spindle
    cairo_arc (cr, mid, mid, wid, 0, twopi);
    cairo_fill (cr);

    // copy the surface to the image
    g_object_ref_sink (clk->clock_ana);
    gtk_image_set_from_surface (GTK_IMAGE (clk->clock_ana), surface);

    cairo_destroy (cr);
}

/*----------------------------------------------------------------------------*/
/* Timer handler                                                              */
/*----------------------------------------------------------------------------*/

static gboolean clock_tick (ClockPlugin *clk)
{
    static int last_min = -1;
    GDateTime *dt = g_date_time_new_now_local ();
    gchar *time = g_date_time_format (dt, clk->time_format);
    gchar *date = g_date_time_format (dt, clk->date_format);

    if (clk->override_font)
    {
        char *markup = g_strdup_printf ("<span font = \"%s\">%s</span>", clk->clock_font, time);
        gtk_label_set_markup (GTK_LABEL (clk->clock_label), markup);
        g_free (markup);
    }
    else gtk_label_set_text (GTK_LABEL (clk->clock_label), time);
    gtk_widget_set_tooltip_text (clk->plugin, date);

    // only do all the cairo drawing if the picture needs to change
    if (last_min != g_date_time_get_minute (dt))
    {
        draw_face (clk, g_date_time_get_hour (dt), g_date_time_get_minute (dt));
        last_min = g_date_time_get_minute (dt);
    }

    g_free (time);
    g_free (date);
    g_date_time_unref (dt);

    gtk_widget_set_visible (clk->clock_label, !clk->analogue);
    gtk_widget_set_visible (clk->clock_ana, clk->analogue);

    return TRUE;
}

/*----------------------------------------------------------------------------*/
/* wf-panel plugin functions                                                  */
/*----------------------------------------------------------------------------*/

/* Handler for system config changed message from panel */
void clock_update_display (ClockPlugin *clk)
{
    GDateTime *dt = g_date_time_new_now_local ();
    draw_face (clk, g_date_time_get_hour (dt), g_date_time_get_minute (dt));
    g_date_time_unref (dt);

    gtk_widget_set_visible (clk->clock_label, !clk->analogue);
    gtk_widget_set_visible (clk->clock_ana, clk->analogue);
}

/* Handler for button click */
#ifndef LXPLUG
static gboolean clock_button_pressed (GtkWidget *, GdkEventButton *, ClockPlugin *clk)
{
    if (clk->calendar_window && GTK_IS_WIDGET (clk->calendar_window) && gtk_widget_get_visible (clk->calendar_window)) clk->popup_shown = TRUE;
    else clk->popup_shown = FALSE;
    return FALSE;
}
#endif

static void clock_button_clicked (GtkWidget *, ClockPlugin *clk)
{
#ifdef LXPLUG
    if (clk->calendar_window) gtk_widget_destroy (clk->calendar_window);
#else
    CHECK_LONGPRESS
    if (clk->popup_shown) close_popup ();
#endif
    else show_calendar (clk);
}

void clock_init (ClockPlugin *clk)
{
    setlocale (LC_ALL, "");
    bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");

    /* Create label as a child of top level */
    clk->clock_label = gtk_label_new (NULL);
    gtk_widget_set_margin_start (clk->clock_label, 4);
    gtk_widget_set_margin_end (clk->clock_label, 4);
    gtk_label_set_xalign (GTK_LABEL (clk->clock_label), 0.5);
    clk->clock_ana = gtk_image_new ();

    GtkWidget *box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_container_add (GTK_CONTAINER (clk->plugin), box);
    gtk_container_add (GTK_CONTAINER (box), clk->clock_label);
    gtk_container_add (GTK_CONTAINER (box), clk->clock_ana);

    /* Set up button */
    gtk_button_set_relief (GTK_BUTTON (clk->plugin), GTK_RELIEF_NONE);
#ifndef LXPLUG
    g_signal_connect (clk->plugin, "button-press-event", G_CALLBACK (clock_button_pressed), clk);
#endif
    g_signal_connect (clk->plugin, "clicked", G_CALLBACK (clock_button_clicked), clk);

    /* Set up variables */
    clk->calendar_window = NULL;

    gtk_widget_show_all (clk->plugin);
    clock_tick (clk);
    clock_update_display (clk);

    /* Start timed event to update clock */
    clk->timer = g_timeout_add_seconds (1, (GSourceFunc) clock_tick, clk);
}

void clock_destructor (gpointer user_data)
{
    ClockPlugin *clk = (ClockPlugin *) user_data;

    if (clk->timer) g_source_remove (clk->timer);

    /* Deallocate memory */
    if (clk->time_format) g_free (clk->time_format);
    if (clk->date_format) g_free (clk->date_format);
    if (clk->clock_font) g_free (clk->clock_font);
    g_free (clk);
}

/*----------------------------------------------------------------------------*/
/* LXPanel plugin functions                                                   */
/*----------------------------------------------------------------------------*/
#ifdef LXPLUG

/* Constructor */
static GtkWidget *clock_constructor (LXPanel *panel, config_setting_t *settings)
{
    /* Allocate and initialize plugin context */
    ClockPlugin *clk = g_new0 (ClockPlugin, 1);

    /* Allocate top level widget and set into plugin widget pointer. */
    clk->panel = panel;
    clk->settings = settings;
    clk->plugin = gtk_button_new ();
    lxpanel_plugin_set_data (clk->plugin, clk, clock_destructor);

    /* Set config defaults */
    clk->time_format = g_strdup (DEFAULT_TIME_FORMAT);
    clk->date_format = g_strdup (DEFAULT_DATE_FORMAT);
    clk->clock_font = g_strdup ("");
    clk->override_font = FALSE;

    /* Read config */
    conf_table[0].value = (void *) &clk->time_format;
    conf_table[1].value = (void *) &clk->date_format;
    conf_table[2].value = (void *) &clk->clock_font;
    conf_table[3].value = (void *) &clk->override_font;
    lxplug_read_settings (clk->settings, conf_table);

    clock_init (clk);

    return clk->plugin;
}

/* Handler for button press */
static gboolean clock_button_press_event (GtkWidget *plugin, GdkEventButton *event, LXPanel *)
{
    ClockPlugin *clk = lxpanel_plugin_get_data (plugin);

    if (event->button == 1)
    {
        clock_button_clicked (plugin, clk);
        return TRUE;
    }
    else return FALSE;
}

/* Apply changes from config dialog */
static gboolean clock_apply_configuration (gpointer user_data)
{
    ClockPlugin *clk = lxpanel_plugin_get_data (GTK_WIDGET (user_data));

    lxplug_write_settings (clk->settings, conf_table);

    return FALSE;
}

/* Display configuration dialog */
static GtkWidget *clock_configure (LXPanel *panel, GtkWidget *plugin)
{
    return lxpanel_generic_config_dlg_new (_(PLUGIN_TITLE), panel,
        clock_apply_configuration, plugin,
        conf_table);
}

int module_lxpanel_gtk_version = 1;
char module_name[] = PLUGIN_NAME;

/* Plugin descriptor */
LXPanelPluginInit fm_module_init_lxpanel_gtk = {
    .name = PLUGIN_TITLE,
    .description = N_("Digital clock and calendar"),
    .new_instance = clock_constructor,
    .button_press_event = clock_button_press_event,
    .config = clock_configure,
    .gettext_package = GETTEXT_PACKAGE
};
#endif

/* End of file */
/*----------------------------------------------------------------------------*/
