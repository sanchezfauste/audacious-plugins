/**
 * Copyright (C) 2017-2018 Marc Sanchez Fauste
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <math.h>
#include <string.h>

#include <gtk/gtk.h>

#include <libaudcore/hook.h>
#include <libaudcore/i18n.h>
#include <libaudcore/interface.h>
#include <libaudcore/plugin.h>
#include <libaudcore/preferences.h>
#include <libaudcore/runtime.h>
#include <libaudgui/libaudgui.h>
#include <libaudgui/libaudgui-gtk.h>
#include "arduino-serial-lib.h"

#define CHANNELS 2
#define DB_RANGE -96
#define DRAW_DB_RANGE 96
const char* const serialport = "/dev/ttyACM0";
const int baudrate = 115200;

typedef struct ArduinoArduinoVUMeterInfo {
    float db_l;
    float db_r;
    float peak_l;
    float peak_r;
} ArduinoArduinoVUMeterInfo;

class ArduinoVUMeter : public VisPlugin
{
public:
    static const char about[];
    static const PreferencesWidget widgets[];
    static const PluginPreferences prefs;
    static const char * const prefs_defaults[];
    int fd = -1;

    static constexpr PluginInfo info = {
        N_("Arduino VU Meter"),
        PACKAGE,
        about,
        & prefs,
        PluginGLibOnly
    };

    constexpr ArduinoVUMeter () : VisPlugin (info, Visualizer::MultiPCM) {}

    bool init ();

    void * get_gtk_widget ();

    void clear ();
    void render_multi_pcm (const float * pcm, int channels);

    void send_data_to_arduino();
    bool init_arduino();
};

EXPORT ArduinoVUMeter aud_plugin_instance;

const char ArduinoVUMeter::about[] =
 N_("Arduino VU Meter Plugin for Audacious\n"
    "Copyright 2017-2018 Marc Sánchez Fauste");

const PreferencesWidget ArduinoVUMeter::widgets[] = {
    WidgetLabel (N_("<b>Arduino VU Meter Settings</b>")),
    WidgetSpin (
        N_("Peak hold time:"),
        WidgetFloat ("arduino_ArduinoVUMeter", "peak_hold_time"),
        {0.1, 30, 0.1, N_("seconds")}
    ),
    WidgetSpin (
        N_("Fall-off time:"),
        WidgetFloat ("arduino_ArduinoVUMeter", "falloff"),
        {0.1, 96, 0.1, N_("dB/second")}
    )
};

const PluginPreferences ArduinoVUMeter::prefs = {{widgets}};

const char * const ArduinoVUMeter::prefs_defaults[] = {
    "peak_hold_time", "1.6",
    "falloff", "13.3",
    nullptr
};

static GtkWidget * spect_widget = nullptr;
static int width, height;
static int bands = 4;
static int nchannels = 2;
static float bars[CHANNELS + 1];
static float peak[CHANNELS + 1];
static gint64 last_peak_times[CHANNELS + 1]; // Time elapsed since peak was set
static gint64 last_render_time = 0;
static gint64 last_arduino_time = 0;

static float fclamp(float x, float low, float high)
{
    return fminf (fmaxf (x, low), high);
}

void ArduinoVUMeter::render_multi_pcm (const float * pcm, int channels)
{
    gint64 current_time = g_get_monotonic_time();
    gint64 elapsed_render_time = current_time - last_render_time;
    gint64 elapsed_arduino_time = current_time - last_arduino_time;
    last_render_time = current_time;
    nchannels = channels;
    bands = channels <= CHANNELS ? channels + 2 : CHANNELS + 2;
    float falloff = aud_get_double ("arduino_ArduinoVUMeter", "falloff") / 1000000.0;
    gint64 peak_hold_time = aud_get_double ("arduino_ArduinoVUMeter", "peak_hold_time") * 1000000;

    float peaks[channels];
    for (int channel = 0; channel < channels; channel++)
    {
        peaks[channel] = pcm[channel];
    }
    for (int i = 0; i < 512 * channels;)
    {
        for (int channel = 0; channel < channels; channel++)
        {
            peaks[channel] = fmaxf(peaks[channel], fabsf(pcm[i++]));
        }
    }

    for (int i = 0; i < channels; i ++)
    {
        float n = peaks[i];

        float db = 20 * log10f (n);
        db = fclamp (db, DB_RANGE, 0);

        bars[i] = fclamp(bars[i] - elapsed_render_time * falloff, DB_RANGE, 0);

        if (db > bars[i])
        {
            bars[i] = db;
        }
        gint64 elapsed_peak_time = current_time - last_peak_times[i];
        if (db > peak[i] || elapsed_peak_time > peak_hold_time) {
            peak[i] = db;
            last_peak_times[i] = g_get_monotonic_time();
        }
    }

    if (spect_widget)
        gtk_widget_queue_draw (spect_widget);
    if (elapsed_arduino_time > 20000) {
        send_data_to_arduino();
        last_arduino_time = current_time;
    }
}

bool ArduinoVUMeter::init ()
{
    aud_config_set_defaults ("arduino_ArduinoVUMeter", prefs_defaults);
    if (fd == -1) {
        init_arduino();
    }
    return true;
}

void ArduinoVUMeter::clear ()
{
    for (int i = 0; i < CHANNELS; i += 1) {
        bars[i] = DB_RANGE;
        peak[i] = DB_RANGE;
    }
    memset (last_peak_times, 0, sizeof last_peak_times);

    if (spect_widget)
        gtk_widget_queue_draw (spect_widget);
}

static void draw_vu_legend_db(cairo_t * cr, int db, const char *text)
{
    cairo_text_extents_t extents;
    cairo_text_extents (cr, text, &extents);
    cairo_move_to(cr, (width / bands) - extents.width - 4, height - ((DRAW_DB_RANGE - db) * height / DRAW_DB_RANGE) + (extents.height/2));
    cairo_show_text (cr, text);
    cairo_move_to(cr, (width / bands) * (nchannels + 1) + 4, height - ((DRAW_DB_RANGE - db) * height / DRAW_DB_RANGE) + (extents.height/2));
    cairo_show_text (cr, text);
}

static void draw_vu_legend(cairo_t * cr)
{
    cairo_set_source_rgb (cr, 1, 1, 1);
    float font_size_width = (width / bands) / 2;
    float font_size_height = 2 * height / DRAW_DB_RANGE;
    cairo_set_font_size (cr, fmin(font_size_width, font_size_height));
    draw_vu_legend_db(cr, 1, "-1");
    draw_vu_legend_db(cr, 3, "-3");
    draw_vu_legend_db(cr, 5, "-5");
    draw_vu_legend_db(cr, 7, "-7");
    draw_vu_legend_db(cr, 9, "-9");
    draw_vu_legend_db(cr, 12, "-12");
    draw_vu_legend_db(cr, 15, "-15");
    draw_vu_legend_db(cr, 17, "-17");
    draw_vu_legend_db(cr, 20, "-20");
    draw_vu_legend_db(cr, 25, "-25");
    draw_vu_legend_db(cr, 30, "-30");
    draw_vu_legend_db(cr, 35, "-35");
    draw_vu_legend_db(cr, 40, "-40");
    draw_vu_legend_db(cr, 45, "-45");
    draw_vu_legend_db(cr, 50, "-50");
    draw_vu_legend_db(cr, 55, "-55");
    draw_vu_legend_db(cr, 60, "-60");
    draw_vu_legend_db(cr, 70, "-70");
    draw_vu_legend_db(cr, 80, "-80");
    draw_vu_legend_db(cr, 90, "-90");
    draw_vu_legend_db(cr, 96, "-96");
}

static void draw_background (GtkWidget * area, cairo_t * cr)
{
    GtkAllocation alloc;
    gtk_widget_get_allocation (area, & alloc);

    cairo_rectangle(cr, 0, 0, alloc.width, alloc.height);
    cairo_fill (cr);

    draw_vu_legend(cr);
}

static void draw_visualizer2 (cairo_t *cr)
{
    for (int i = 0; i < nchannels; i++)
    {
        int x = ((width / bands) * (i+1)) + 2;
        float r=0, g=1, b=0;

        for (double l = 0; l < bars[i]; l += 0.2) {
            if (l > DRAW_DB_RANGE - 3) {
                cairo_set_source_rgb (cr, 1, 0, 0);
            } else if (l >= DRAW_DB_RANGE - 9) {
                cairo_set_source_rgb (cr, 1, 1, 0);
            } else {
                cairo_set_source_rgb (cr, r, g, b);
            }
            cairo_rectangle (cr, x + 1, height - (l * height / DRAW_DB_RANGE),
             (width / bands) - 1, (0.1 * height / DRAW_DB_RANGE));
            cairo_fill (cr);
        }
        
        
        if (peak[i] > DRAW_DB_RANGE - 3) {
            cairo_set_source_rgb (cr, 1, 0, 0);
        } else if (peak[i] >= DRAW_DB_RANGE - 9) {
            cairo_set_source_rgb (cr, 1, 1, 0);
        } else {
            cairo_set_source_rgb (cr, r, g, b);
        }
        cairo_rectangle (cr, x + 1, height - (peak[i] * height / DRAW_DB_RANGE),
         (width / bands) - 1, (0.1 * height / DRAW_DB_RANGE));
        cairo_fill (cr);
        
        
        
        
    }
}

static void draw_visualizer (cairo_t *cr)
{
    for (int i = 0; i < nchannels; i++)
    {
        int x = ((width / bands) * (i+1)) + 2;
        float r=0, g=1, b=0;

        cairo_set_source_rgba (cr, 1, 0, 0, 0.2);
        cairo_rectangle (cr, x + 1, 0, (width / bands) - 1,
         (3 * height / DRAW_DB_RANGE) + 1);
        cairo_fill (cr);
        float level = bars[i] + DRAW_DB_RANGE;
        if (level > DRAW_DB_RANGE - 3) {
            float size = fclamp (level - (DRAW_DB_RANGE - 3), 0, 3) + 2;
            float barsdei = level;
            cairo_set_source_rgb (cr, 1, 0, 0);
            cairo_rectangle (cr, x + 1, height - (barsdei * height / DRAW_DB_RANGE),
             (width / bands) - 1, (size * height / DRAW_DB_RANGE));
            cairo_fill (cr);
        }
        cairo_set_source_rgba (cr, 1, 1, 0, 0.2);
        cairo_rectangle (cr, x + 1, height - ((DRAW_DB_RANGE - 3) * height / DRAW_DB_RANGE),
         (width / bands) - 1, (6 * height / DRAW_DB_RANGE));
        cairo_fill (cr);
        if (level >= DRAW_DB_RANGE - 9) {
            float size = fclamp (level - (DRAW_DB_RANGE - 9), 0, 6) + 2;
            float barsdei = fclamp (level, 0, DRAW_DB_RANGE - 3);
            cairo_set_source_rgb (cr, 1, 1, 0);
            cairo_rectangle (cr, x + 1, height - (barsdei * height / DRAW_DB_RANGE),
             (width / bands) - 1, (size * height / DRAW_DB_RANGE));
            cairo_fill (cr);
        }
        cairo_set_source_rgba (cr, r, g, b, 0.2);
        cairo_rectangle (cr, x + 1, height - ((DRAW_DB_RANGE - 9) * height / DRAW_DB_RANGE),
         (width / bands) - 1, ((DRAW_DB_RANGE - 9) * height / DRAW_DB_RANGE));
        cairo_fill (cr);
        float size = fclamp (level, 0, DRAW_DB_RANGE - 9);
        cairo_set_source_rgb (cr, r, g, b);
        cairo_rectangle (cr, x + 1, height - (size * height / DRAW_DB_RANGE),
         (width / bands) - 1, (size * height / DRAW_DB_RANGE));
        cairo_fill (cr);
        float peak_level = peak[i] + DRAW_DB_RANGE;
        if (peak_level > DRAW_DB_RANGE - 3) {
            cairo_set_source_rgb (cr, 1, 0, 0);
        } else if (peak_level >= DRAW_DB_RANGE - 9) {
            cairo_set_source_rgb (cr, 1, 1, 0);
        }
        cairo_rectangle (cr, x + 1, height - (peak_level * height / DRAW_DB_RANGE),
         (width / bands) - 1, (0.1 * height / DRAW_DB_RANGE));
        cairo_fill (cr);
    }
}

static gboolean configure_event (GtkWidget * widget, GdkEventConfigure * event)
{
    width = event->width;
    height = event->height;
    gtk_widget_queue_draw(widget);

    return true;
}

void ArduinoVUMeter::send_data_to_arduino() {
    ArduinoArduinoVUMeterInfo info;
    info.db_l = bars[0];
    info.db_r = bars[1];
    info.peak_l = peak[0];
    info.peak_r = peak[1];
    char buffer[255];
    sprintf(buffer, "%f$%f$%f$%f$;", info.db_l, info.db_r, info.peak_l, info.peak_r);
    if (fd != -1) {
        serialport_write(fd, buffer);
    }
}

bool ArduinoVUMeter::init_arduino() {
    fd = serialport_init(serialport, baudrate);
    if( fd == -1 ) {
        printf("couldn't open port %s\n", serialport);
        return false;
    }
    printf("opened port %s\n", serialport);
    serialport_flush(fd);
    return true;
}

static gboolean draw_event (GtkWidget * widget)
{
    cairo_t * cr = gdk_cairo_create (gtk_widget_get_window (widget));

    draw_background (widget, cr);
    draw_visualizer (cr);

    cairo_destroy (cr);
    return true;
}

void * ArduinoVUMeter::get_gtk_widget ()
{
    GtkWidget *area = gtk_drawing_area_new();
    spect_widget = area;

    g_signal_connect(area, "expose-event", (GCallback) draw_event, nullptr);
    g_signal_connect(area, "configure-event", (GCallback) configure_event, nullptr);
    g_signal_connect(area, "destroy", (GCallback) gtk_widget_destroyed, & spect_widget);

    GtkWidget * frame = gtk_frame_new (nullptr);
    gtk_frame_set_shadow_type ((GtkFrame *) frame, GTK_SHADOW_IN);
    gtk_container_add ((GtkContainer *) frame, area);
    return frame;
}
