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

conf_table_t conf_table[5] = {
    {CONF_TYPE_STRING,  "time_format",  N_("Time format"),      NULL},
    {CONF_TYPE_STRING,  "date_format",  N_("Date format"),      NULL},
    {CONF_TYPE_FONT,    "font",         N_("Clock font"),       NULL},
    {CONF_TYPE_BOOL,    "custom_font",  N_("Use custom font"),  NULL},
    {CONF_TYPE_NONE,    NULL,           NULL,                   NULL}
};

/*----------------------------------------------------------------------------*/
/* Prototypes                                                                 */
/*----------------------------------------------------------------------------*/

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

    wrap_popup_at_button (clk, clk->calendar_window, clk->plugin);
}

/*----------------------------------------------------------------------------*/
/* Timer handler                                                              */
/*----------------------------------------------------------------------------*/

static gboolean clock_tick (ClockPlugin *clk)
{
    GDateTime *dt = g_date_time_new_now_local ();
    gchar *time = g_date_time_format (dt, clk->time_format);
    gchar *date = g_date_time_format (dt, clk->date_format);

    gtk_label_set_text (GTK_LABEL (clk->clock_label), time);
    gtk_widget_set_tooltip_text (clk->plugin, date);
    g_free (time);
    g_free (date);
    g_date_time_unref (dt);

    return TRUE;
}

/*----------------------------------------------------------------------------*/
/* Font                                                                       */
/*----------------------------------------------------------------------------*/

void set_font (ClockPlugin *clk)
{
    if (!clk->clock_font || !clk->override_font) gtk_widget_override_font (clk->clock_label, NULL);
    else
    {
        PangoFontDescription *pf = pango_font_description_from_string (clk->clock_font);
        gtk_widget_override_font (clk->clock_label, pf);
    }
}

/*----------------------------------------------------------------------------*/
/* wf-panel plugin functions                                                  */
/*----------------------------------------------------------------------------*/

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
    CHECK_LONGPRESS
    if (!clk->popup_shown) show_calendar (clk);
}

/* Handler for system config changed message from panel */
void clock_update_display (ClockPlugin *)
{
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
    gtk_container_add (GTK_CONTAINER (clk->plugin), clk->clock_label);

    /* Set up button */
    gtk_button_set_relief (GTK_BUTTON (clk->plugin), GTK_RELIEF_NONE);
#ifndef LXPLUG
    g_signal_connect (clk->plugin, "button-press-event", G_CALLBACK (clock_button_pressed), clk);
    g_signal_connect (clk->plugin, "clicked", G_CALLBACK (clock_button_clicked), clk);
#endif

    /* Set up variables */
    clk->calendar_window = NULL;

    set_font (clk);
    clock_tick (clk);
    gtk_widget_show_all (clk->plugin);

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

/* Handler for system config changed message from panel */
static void clock_configuration_changed (LXPanel *, GtkWidget *)
{
    // font stuff ???
}

/* Apply changes from config dialog */
static gboolean clock_apply_configuration (gpointer user_data)
{
    ClockPlugin *clk = lxpanel_plugin_get_data (GTK_WIDGET (user_data));

    lxplug_write_settings (clk->settings, conf_table);

    set_font (clk);

    return FALSE;
}

/* Display configuration dialog */
static GtkWidget *clock_configure (LXPanel *panel, GtkWidget *plugin)
{
    return lxpanel_generic_config_dlg_new (_(PLUGIN_TITLE), panel,
        clock_apply_configuration, plugin,
        conf_table);
}

FM_DEFINE_MODULE (lxpanel_gtk, clock)

/* Plugin descriptor */
LXPanelPluginInit fm_module_init_lxpanel_gtk = {
    .name = N_(PLUGIN_TITLE),
    .description = N_("Digital clock and calendar"),
    .new_instance = clock_constructor,
    .reconfigure = clock_configuration_changed,
    .button_press_event = clock_button_press_event,
    .config = clock_configure,
    .gettext_package = GETTEXT_PACKAGE
};
#endif

/* End of file */
/*----------------------------------------------------------------------------*/
