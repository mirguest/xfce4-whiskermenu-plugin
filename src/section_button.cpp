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


#include "section_button.hpp"

//-----------------------------------------------------------------------------

GtkRadioButton* WhiskerMenu::new_section_button(const gchar* icon, const gchar* text)
{
	GtkWidget* button = gtk_radio_button_new(nullptr);
	gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(button), false);
	gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
	gtk_button_set_focus_on_click(GTK_BUTTON(button), false);

	GtkBox* box = GTK_BOX(gtk_hbox_new(false, 4));
	gtk_container_add(GTK_CONTAINER(button), GTK_WIDGET(box));

	GtkWidget* image = gtk_image_new_from_icon_name(icon, GTK_ICON_SIZE_LARGE_TOOLBAR);
	gtk_box_pack_start(box, GTK_WIDGET(image), false, false, 0);

	GtkWidget* label = gtk_label_new(text);
	gtk_box_pack_start(box, label, false, true, 0);

	return GTK_RADIO_BUTTON(button);
}

//-----------------------------------------------------------------------------