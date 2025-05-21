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

#include <glibmm.h>
#include "gtk-utils.hpp"
#include "clock.hpp"

extern "C" {
    WayfireWidget *create () { return new WayfireClock; }
    void destroy (WayfireWidget *w) { delete w; }

    const conf_table_t *config_params (void) { return conf_table; };
    const char *display_name (void) { return N_(PLUGIN_TITLE); };
    const char *package_name (void) { return GETTEXT_PACKAGE; };
}

void WayfireClock::settings_changed_cb (void)
{
    if (clk->time_format) g_free (clk->time_format);
    if (clk->date_format) g_free (clk->date_format);
    if (clk->clock_font) g_free (clk->clock_font);
    clk->time_format = g_strdup (((std::string) time_format).c_str());
    clk->date_format = g_strdup (((std::string) date_format).c_str());
    clk->clock_font = g_strdup (((std::string) clock_font).c_str());
    clk->override_font = font_override;
    clock_update_display (clk);
}

void WayfireClock::init (Gtk::HBox *container)
{
    /* Create the button */
    plugin = std::make_unique <Gtk::Button> ();
    plugin->set_name (PLUGIN_NAME);
    container->pack_start (*plugin, false, false);

    /* Setup structure */
    clk = g_new0 (ClockPlugin, 1);
    clk->plugin = (GtkWidget *)((*plugin).gobj());

    /* Add long press for right click */
    gesture = add_longpress_default (*plugin);

    /* Initialise the plugin */
    clock_init (clk);

    /* Setup callbacks */
    time_format.set_callback (sigc::mem_fun (*this, &WayfireClock::settings_changed_cb));
    date_format.set_callback (sigc::mem_fun (*this, &WayfireClock::settings_changed_cb));
    clock_font.set_callback (sigc::mem_fun (*this, &WayfireClock::settings_changed_cb));
    font_override.set_callback (sigc::mem_fun (*this, &WayfireClock::settings_changed_cb));

    settings_changed_cb ();
}

WayfireClock::~WayfireClock()
{
    clock_destructor (clk);
}

/* End of file */
/*----------------------------------------------------------------------------*/
