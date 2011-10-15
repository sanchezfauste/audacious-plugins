/*  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2005-2009  Audacious development team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; under version 3 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses>.
 *
 *  The Audacious team does not consider modular code linking to
 *  Audacious or using our public API to be a derived work.
 */

#include <audacious/configdb.h>
#include <audacious/plugin.h>

#include "gtkui_cfg.h"

gtkui_cfg_t config;

gtkui_cfg_t gtkui_default_config = {
    .player_x = MAINWIN_DEFAULT_POS_X,
    .player_y = MAINWIN_DEFAULT_POS_Y,
    .player_width = MAINWIN_DEFAULT_WIDTH,
    .player_height = MAINWIN_DEFAULT_HEIGHT,
    .save_window_position = TRUE,
    .player_visible = TRUE,
    .infoarea_visible = TRUE,
    .menu_visible = TRUE,
    .statusbar_visible = TRUE,
    .show_song_titles = TRUE,
    .playlist_columns = NULL,
    .playlist_headers = TRUE,
    .custom_playlist_colors = FALSE,
    .playlist_fg = NULL,
    .playlist_bg = NULL,
    .playlist_font = NULL,
    .autoscroll = TRUE,
};

typedef struct gtkui_cfg_boolent_t
{
    gchar const *be_vname;
    gboolean *be_vloc;
    gboolean be_wrt;
} gtkui_cfg_boolent;

static gtkui_cfg_boolent gtkui_boolents[] = {
    {"save_window_position", &config.save_window_position, TRUE},
    {"player_visible", &config.player_visible, TRUE},
    {"infoarea_visible", &config.infoarea_visible, TRUE},
    {"menu_visible", &config.menu_visible, TRUE},
    {"statusbar_visible", &config.statusbar_visible, TRUE},
    {"show_song_titles", &config.show_song_titles, TRUE},
    {"playlist_headers", &config.playlist_headers, TRUE},
    {"custom_playlist_colors", &config.custom_playlist_colors, TRUE},
    {"always_on_top", &config.always_on_top, TRUE},
    {"autoscroll", &config.autoscroll, TRUE},
};

static gint ncfgbent = G_N_ELEMENTS(gtkui_boolents);

typedef struct gtkui_cfg_nument_t
{
    gchar const *ie_vname;
    gint *ie_vloc;
    gboolean ie_wrt;
} gtkui_cfg_nument;

static gtkui_cfg_nument gtkui_numents[] = {
    {"player_x", &config.player_x, TRUE},
    {"player_y", &config.player_y, TRUE},
    {"player_width", &config.player_width, TRUE},
    {"player_height", &config.player_height, TRUE},
};

static gint ncfgient = G_N_ELEMENTS(gtkui_numents);

typedef struct gtkui_cfg_strent_t
{
    gchar const *se_vname;
    gchar **se_vloc;
    gboolean se_wrt;
} gtkui_cfg_strent;

static gtkui_cfg_strent gtkui_strents[] = {
    {"playlist_columns", &config.playlist_columns, TRUE},
    {"playlist_bg", &config.playlist_bg, TRUE},
    {"playlist_fg", &config.playlist_fg, TRUE},
    {"playlist_font", &config.playlist_font, TRUE},
};

static gint ncfgsent = G_N_ELEMENTS(gtkui_strents);

void gtkui_cfg_free()
{
    gint i;

    for (i = 0; i < ncfgsent; ++i)
    {
        if (*(gtkui_strents[i].se_vloc) != NULL)
        {
            g_free(*(gtkui_strents[i].se_vloc));
            *(gtkui_strents[i].se_vloc) = NULL;
        }
    }
}

void gtkui_cfg_load()
{
    memcpy(&config, &gtkui_default_config, sizeof(gtkui_cfg_t));

    mcs_handle_t *cfgfile = aud_cfg_db_open();
    if (! cfgfile)
        return;

    gint i;

    for (i = 0; i < ncfgbent; ++i)
        aud_cfg_db_get_bool(cfgfile, "gtkui", gtkui_boolents[i].be_vname, gtkui_boolents[i].be_vloc);

    for (i = 0; i < ncfgient; ++i)
        aud_cfg_db_get_int(cfgfile, "gtkui", gtkui_numents[i].ie_vname, gtkui_numents[i].ie_vloc);

    for (i = 0; i < ncfgsent; ++i)
        aud_cfg_db_get_string(cfgfile, "gtkui", gtkui_strents[i].se_vname, gtkui_strents[i].se_vloc);

    aud_cfg_db_close(cfgfile);
}


void gtkui_cfg_save()
{
    mcs_handle_t *cfgfile = aud_cfg_db_open();
    if (! cfgfile)
        return;

    gint i;

    for (i = 0; i < ncfgsent; ++i)
    {
        if (gtkui_strents[i].se_wrt)
            aud_cfg_db_set_string(cfgfile, "gtkui", gtkui_strents[i].se_vname, *gtkui_strents[i].se_vloc);
    }

    for (i = 0; i < ncfgbent; ++i)
    {
        if (gtkui_boolents[i].be_wrt)
            aud_cfg_db_set_bool(cfgfile, "gtkui", gtkui_boolents[i].be_vname, *gtkui_boolents[i].be_vloc);
    }

    for (i = 0; i < ncfgient; ++i)
    {
        if (gtkui_numents[i].ie_wrt)
            aud_cfg_db_set_int(cfgfile, "gtkui", gtkui_numents[i].ie_vname, *gtkui_numents[i].ie_vloc);
    }

    aud_cfg_db_close(cfgfile);
}