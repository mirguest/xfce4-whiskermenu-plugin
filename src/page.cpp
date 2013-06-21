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


#include "page.hpp"

#include "favorites_page.hpp"
#include "launcher.hpp"
#include "launcher_model.hpp"
#include "launcher_view.hpp"
#include "menu.hpp"
#include "recent_page.hpp"
#include "slot.hpp"

extern "C"
{
#include <libxfce4ui/libxfce4ui.h>
}

using namespace WhiskerMenu;

//-----------------------------------------------------------------------------

Page::Page(Menu* menu) :
	m_menu(menu),
	m_selected_path(nullptr)
{
	// Create view
	m_view = new LauncherView;
	g_signal_connect_slot(m_view->get_widget(), "button-press-event", &Page::view_button_press_event, this);
	g_signal_connect_slot(m_view->get_widget(), "popup-menu", &Page::view_popup_menu_event, this);
	g_signal_connect_slot(m_view->get_widget(), "row-activated", &Page::launcher_activated, this);
	g_signal_connect_swapped(m_view->get_widget(), "start-interactive-search", G_CALLBACK(gtk_widget_grab_focus), m_menu->get_search_entry());

	// Add scrolling to view
	m_widget = gtk_scrolled_window_new(nullptr, nullptr);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(m_widget), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(m_widget), GTK_SHADOW_ETCHED_IN);
	gtk_container_add(GTK_CONTAINER(m_widget), m_view->get_widget());
	g_signal_connect_slot(m_widget, "unmap", &Page::on_unmap, this);
	g_object_ref_sink(m_widget);
}

//-----------------------------------------------------------------------------

Page::~Page()
{
	if (m_selected_path)
	{
		gtk_tree_path_free(m_selected_path);
	}

	delete m_view;
	g_object_unref(m_widget);
}

//-----------------------------------------------------------------------------

Launcher* Page::get_selected_launcher() const
{
	Launcher* launcher = nullptr;
	if (m_selected_path)
	{
		GtkTreeModel* model = m_view->get_model();
		GtkTreeIter iter;
		gtk_tree_model_get_iter(model, &iter, m_selected_path);
		gtk_tree_model_get(model, &iter, LauncherModel::COLUMN_LAUNCHER, &launcher, -1);
	}
	return launcher;
}

//-----------------------------------------------------------------------------

void Page::launcher_activated(GtkTreeView* view, GtkTreePath* path, GtkTreeViewColumn*)
{
	GtkTreeIter iter;
	GtkTreeModel* model = gtk_tree_view_get_model(view);
	gtk_tree_model_get_iter(model, &iter, path);

	// Find launcher
	Launcher* launcher = nullptr;
	gtk_tree_model_get(model, &iter, LauncherModel::COLUMN_LAUNCHER, &launcher, -1);

	// Add to recent
	m_menu->get_recent()->add(launcher);

	// Hide window
	m_menu->hide();

	// Execute app
	launcher->run(gtk_widget_get_screen(GTK_WIDGET(view)));
}

//-----------------------------------------------------------------------------

gboolean Page::view_button_press_event(GtkWidget* view, GdkEventButton* event)
{
	GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
	GtkTreeIter iter;
	if (gtk_tree_selection_get_selected(selection, nullptr, &iter)
			&& (event->type == GDK_BUTTON_PRESS)
			&& (event->button == 3))
	{
		create_context_menu(&iter, event);
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------

gboolean Page::view_popup_menu_event(GtkWidget* view)
{
	GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
	GtkTreeIter iter;
	if (gtk_tree_selection_get_selected(selection, nullptr, &iter))
	{
		create_context_menu(&iter, nullptr);
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------

void Page::on_unmap()
{
	// Clear selection and scroll to top
	GtkTreeModel* model = m_view->get_model();
	GtkTreeIter iter;
	if (gtk_tree_model_get_iter_first(model, &iter))
	{
		GtkTreePath* path = gtk_tree_model_get_path(model, &iter);
		get_view()->scroll_to_path(path);
		get_view()->unselect_all();
		gtk_tree_path_free(path);
	}
}

//-----------------------------------------------------------------------------

void Page::create_context_menu(GtkTreeIter* iter, GdkEventButton* event)
{
	gtk_tree_view_set_hover_selection(GTK_TREE_VIEW(m_view->get_widget()), false);
	m_selected_path = gtk_tree_model_get_path(m_view->get_model(), iter);

	// Create context menu
	GtkWidget* menu = gtk_menu_new();
	g_signal_connect_slot(menu, "selection-done", &Page::destroy_context_menu, this);

	// Add menu items
	GtkWidget* menuitem = nullptr;

	if (!m_menu->get_favorites()->contains(get_selected_launcher()))
	{
		menuitem = gtk_menu_item_new_with_label(_("Add to Favorites"));
		g_signal_connect_slot(menuitem, "activate", &Page::add_selected_to_favorites, this);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	}
	else
	{
		menuitem = gtk_menu_item_new_with_label(_("Remove From Favorites"));
		g_signal_connect_slot(menuitem, "activate", &Page::remove_selected_from_favorites, this);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	}

	menuitem = gtk_menu_item_new_with_label(_("Add to Desktop"));
	g_signal_connect_slot(menuitem, "activate", &Page::add_selected_to_desktop, this);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

	menuitem = gtk_menu_item_new_with_label(_("Add to Panel"));
	g_signal_connect_slot(menuitem, "activate", &Page::add_selected_to_panel, this);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

	gtk_widget_show_all(menu);

	// Show context menu
	int button = 0;
	int event_time;
	GtkMenuPositionFunc position_func = nullptr;
	if (event)
	{
		button = event->button;
		event_time = event->time;
	}
	else
	{
		position_func = (GtkMenuPositionFunc)&Page::position_context_menu;
		event_time = gtk_get_current_event_time ();
	}

	gtk_menu_attach_to_widget(GTK_MENU(menu), m_view->get_widget(), nullptr);
	gtk_menu_popup(GTK_MENU(menu), nullptr, nullptr, position_func, this, button, event_time);
}

//-----------------------------------------------------------------------------

void Page::destroy_context_menu(GtkMenuShell* menu)
{
	if (m_selected_path)
	{
		gtk_tree_path_free(m_selected_path);
		m_selected_path = nullptr;
	}
	gtk_tree_view_set_hover_selection(GTK_TREE_VIEW(m_view->get_widget()), true);

	gtk_widget_destroy(GTK_WIDGET(menu));
}

//-----------------------------------------------------------------------------

void Page::position_context_menu(GtkMenu*, gint* x, gint* y, gboolean* push_in, Page* page)
{
	// Find rectangle of selected row
	GtkTreeView* treeview = GTK_TREE_VIEW(page->m_view->get_widget());
	GdkRectangle rect;
	GtkTreeViewColumn* column = gtk_tree_view_get_column(treeview, 0);
	gtk_tree_view_get_cell_area(treeview, page->m_selected_path, column, &rect);

	int root_x;
	int root_y;
	gdk_window_get_root_coords(gtk_tree_view_get_bin_window(treeview), rect.x, rect.y, &root_x, &root_y);

	// Position context menu centered on row
	*push_in = false;
	*x = root_x + (rect.width >> 2);
	*y = root_y + (rect.height >> 1);
}

//-----------------------------------------------------------------------------

void Page::add_selected_to_desktop()
{
	// Fetch desktop folder
	const gchar* desktop_path = g_get_user_special_dir(G_USER_DIRECTORY_DESKTOP);
	GFile* desktop_folder = g_file_new_for_path(desktop_path);

	// Fetch launcher source
	Launcher* launcher = get_selected_launcher();
	GFile* source_file = garcon_menu_item_get_file(launcher->get_item());

	// Fetch launcher destination
	char* basename = g_file_get_basename(source_file);
	GFile* destination_file = g_file_get_child(desktop_folder, basename);
	g_free(basename);

	// Copy launcher to desktop folder
	GError* error = nullptr;
	if (!g_file_copy(source_file, destination_file, G_FILE_COPY_NONE, nullptr, nullptr, nullptr, &error))
	{
		xfce_dialog_show_error(nullptr, error, _("Unable to add launcher to desktop."));
		g_error_free(error);
	}

	g_object_unref(destination_file);
	g_object_unref(source_file);
	g_object_unref(desktop_folder);
}

//-----------------------------------------------------------------------------

void Page::add_selected_to_panel()
{
	// Connect to Xfce panel through D-Bus
	GError* error = nullptr;
	GDBusProxy* proxy = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SESSION,
			G_DBUS_PROXY_FLAGS_NONE,
			nullptr,
			"org.xfce.Panel",
			"/org/xfce/Panel",
			"org.xfce.Panel",
			nullptr,
			&error);
	if (proxy)
	{
		// Fetch launcher desktop ID
		Launcher* launcher = get_selected_launcher();
		const gchar* parameters[] = { garcon_menu_item_get_desktop_id(launcher->get_item()), nullptr };

		// Tell panel to add item
		if (!g_dbus_proxy_call_sync(proxy,
				"AddNewItem",
				g_variant_new("(s^as)", "launcher", parameters),
				G_DBUS_CALL_FLAGS_NONE,
				-1,
				nullptr,
				&error))
		{
			xfce_dialog_show_error(nullptr, error, _("Unable to add launcher to panel."));
			g_error_free(error);
		}

		// Disconnect from D-Bus
		g_object_unref(proxy);
	}
	else
	{
		xfce_dialog_show_error(nullptr, error, _("Unable to add launcher to panel."));
		g_error_free(error);
	}
}

//-----------------------------------------------------------------------------

void Page::add_selected_to_favorites()
{
	Launcher* launcher = get_selected_launcher();
	m_menu->get_favorites()->add(launcher);
}

//-----------------------------------------------------------------------------

void Page::remove_selected_from_favorites()
{
	Launcher* launcher = get_selected_launcher();
	m_menu->get_favorites()->remove(launcher);
}

//-----------------------------------------------------------------------------