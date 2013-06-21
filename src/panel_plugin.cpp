// Copyright (C) 2013 Graeme Gott <graeme@gottcode.org>
//
// This library is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this library.  If not, see <http://www.gnu.org/licenses/>.


#include "panel_plugin.hpp"

#include "applications_page.hpp"
#include "configuration_dialog.hpp"
#include "launcher.hpp"
#include "menu.hpp"
#include "slot.hpp"

using namespace WhiskerMenu;

//-----------------------------------------------------------------------------

extern "C" void whiskermenu_construct(XfcePanelPlugin* plugin)
{
	xfce_textdomain(GETTEXT_PACKAGE, LOCALEDIR, "UTF-8");
	new PanelPlugin(plugin);
}

static void whiskermenu_free(XfcePanelPlugin*, PanelPlugin* whiskermenu)
{
	delete whiskermenu;
	whiskermenu = nullptr;
}

//-----------------------------------------------------------------------------

PanelPlugin::PanelPlugin(XfcePanelPlugin* plugin) :
	m_plugin(plugin),
	m_button_icon_name("xfce4-whiskermenu"),
	m_menu(nullptr)
{
	// Load settings
	gchar* file = xfce_panel_plugin_lookup_rc_file(m_plugin);
	if (file)
	{
		XfceRc* settings = xfce_rc_simple_open(file, true);
		g_free(file);

		m_button_icon_name = xfce_rc_read_entry(settings, "button-icon", m_button_icon_name.c_str());
		Launcher::set_show_name(xfce_rc_read_bool_entry(settings, "launcher-show-name", true));
		Launcher::set_show_description(xfce_rc_read_bool_entry(settings, "launcher-show-description", true));
		m_menu = new Menu(settings);

		xfce_rc_close(settings);
	}
	else
	{
		m_menu = new Menu(nullptr);
	}
	g_signal_connect_slot(m_menu->get_widget(), "map", &PanelPlugin::menu_shown, this);
	g_signal_connect_slot(m_menu->get_widget(), "unmap", &PanelPlugin::menu_hidden, this);

	// Create toggle button
	m_button = xfce_create_panel_toggle_button();
	gtk_button_set_relief(GTK_BUTTON(m_button), GTK_RELIEF_NONE);
	m_button_icon = XFCE_PANEL_IMAGE(xfce_panel_image_new_from_source(m_button_icon_name.c_str()));
	xfce_panel_image_set_size(m_button_icon, -1);
	gtk_container_add(GTK_CONTAINER(m_button), GTK_WIDGET(m_button_icon));
	gtk_widget_show_all(m_button);
	g_signal_connect_slot(m_button, "button-press-event", &PanelPlugin::button_clicked, this);

	// Add plugin to panel
	gtk_container_add(GTK_CONTAINER(plugin), m_button);
	xfce_panel_plugin_add_action_widget(plugin, m_button);

	// Connect plugin signals to functions
	g_signal_connect(plugin, "free-data", G_CALLBACK(whiskermenu_free), this);
	g_signal_connect_slot(plugin, "configure-plugin", &PanelPlugin::configure, this);
	g_signal_connect_slot(plugin, "save", &PanelPlugin::save, this);
	g_signal_connect_slot(plugin, "size-changed", &PanelPlugin::size_changed, this);
	xfce_panel_plugin_menu_show_configure(plugin);
}

//-----------------------------------------------------------------------------

PanelPlugin::~PanelPlugin()
{
	delete m_menu;
	m_menu = nullptr;

	gtk_widget_destroy(m_button);
}

//-----------------------------------------------------------------------------

void PanelPlugin::reload()
{
	m_menu->hide();
	m_menu->get_applications()->reload_applications();
}

//-----------------------------------------------------------------------------

void PanelPlugin::set_button_icon_name(std::string icon)
{
	m_button_icon_name = icon;
	xfce_panel_image_set_from_source(m_button_icon, icon.c_str());
}

//-----------------------------------------------------------------------------

void PanelPlugin::set_configure_enabled(bool enabled)
{
	if (enabled)
	{
		xfce_panel_plugin_unblock_menu(m_plugin);
	}
	else
	{
		xfce_panel_plugin_block_menu(m_plugin);
	}
}

//-----------------------------------------------------------------------------

gboolean PanelPlugin::button_clicked(GtkWidget*, GdkEventButton* event)
{
	if (event->button != 1 || event->state & GDK_CONTROL_MASK)
	{
		return false;
	}

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m_button)) == true)
	{
		m_menu->hide();
	}
	else
	{
		m_menu->show(m_button, xfce_panel_plugin_get_orientation(m_plugin) == GTK_ORIENTATION_HORIZONTAL);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(m_button), true);
	}

	return true;
}

//-----------------------------------------------------------------------------

void PanelPlugin::menu_hidden()
{
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(m_button), false);
	xfce_panel_plugin_block_autohide(m_plugin, false);
}

//-----------------------------------------------------------------------------

void PanelPlugin::menu_shown()
{
	xfce_panel_plugin_block_autohide(m_plugin, true);
}

//-----------------------------------------------------------------------------

void PanelPlugin::configure()
{
	new ConfigurationDialog(this);
}

//-----------------------------------------------------------------------------

void PanelPlugin::save()
{
	gchar* file = xfce_panel_plugin_save_location(m_plugin, true);
	if (!file)
	{
		return;
	}
	XfceRc* settings = xfce_rc_simple_open(file, false);
	g_free(file);

	xfce_rc_write_entry(settings, "button-icon", m_button_icon_name.c_str());
	xfce_rc_write_bool_entry(settings, "launcher-show-name", Launcher::get_show_name());
	xfce_rc_write_bool_entry(settings, "launcher-show-description", Launcher::get_show_description());
	m_menu->save(settings);

	xfce_rc_close(settings);
}

//-----------------------------------------------------------------------------

gboolean PanelPlugin::size_changed(XfcePanelPlugin*, gint size)
{
	gtk_widget_set_size_request(GTK_WIDGET(m_plugin), size, size);
	return true;
}

//-----------------------------------------------------------------------------