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

#ifndef WIDGETS_CLOCK_HPP
#define WIDGETS_CLOCK_HPP

#include <widget.hpp>
#include <gtkmm/button.h>

extern "C" {
#include "lxutils.h"
#include "clock.h"
}

class WidgetClock : public PanelWidget
{
    std::unique_ptr <Gtk::Button> plugin;

    WfOption <std::string> time_format {"clock/time_format"};
    WfOption <std::string> date_format {"clock/date_format"};
    WfOption <std::string> clock_font {"clock/font"};
    WfOption <bool> font_override {"clock/custom_font"};
    WfOption <bool> analogue {"clock/analogue"};
    WfOption <std::string> face_col {"clock/face_col"};
    WfOption <std::string> hands_col {"clock/hands_col"};

    /* plugin */
    ClockPlugin *clk;

  public:

    void init (Gtk::HBox *container) override;
    virtual ~WidgetClock ();
    bool set_icon (void);
    void read_settings (void);
    void settings_changed_cb (void);
};

#endif /* end of include guard: WIDGETS_CLOCK_HPP */

/* End of file */
/*----------------------------------------------------------------------------*/
