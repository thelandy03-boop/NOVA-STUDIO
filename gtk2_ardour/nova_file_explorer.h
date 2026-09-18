#pragma once
#ifndef __gtk2_ardour_nova_file_explorer_h__
#define __gtk2_ardour_nova_file_explorer_h__

#include <ytkmm/box.h>
#include <ytkmm/scrolledwindow.h>
#include <ytkmm/label.h>
#include <ytkmm/button.h>
#include <ytkmm/treeview.h>
#include <ytkmm/treestore.h>
#include <ytkmm/treemodelcolumn.h>
#include <ytkmm/menu.h>
#include <ytkmm/image.h>
#include <ydkmm/pixbuf.h>

#include <string>
#include <vector>
#include "pbd/signals.h"

class NovaFileExplorer : public Gtk::VBox
{
public:
	NovaFileExplorer();
	~NovaFileExplorer();

	void refresh();
	void set_root_override(const std::string& path);

	/* Signals de interacción con el exterior (path, content) */
	sigc::signal<void, std::string, std::string>& signal_file_selected()  { return _signal_file_selected; }
	sigc::signal<void>&                            signal_close_requested() { return _signal_close_requested; }
	sigc::signal<void, std::string>&               signal_log_message()     { return _signal_log_message; }
	sigc::signal<void, std::string, std::string>&  signal_file_renamed()    { return _signal_file_renamed; }

private:
	Gtk::HBox           _explorer_header;
	Gtk::Label          _explorer_title;
	Gtk::Button         _btn_explorer_close;
	Gtk::Button         _btn_explorer_add;
	Gtk::ScrolledWindow _explorer_scroll;
	Gtk::TreeView       _explorer_tree;

	Gtk::Menu           _explorer_context_menu;

	struct ExplorerColumns : public Gtk::TreeModel::ColumnRecord {
		ExplorerColumns() {
			add (col_name);
			add (col_path);
			add (col_is_dir);
			add (col_icon);
		}
		Gtk::TreeModelColumn<Glib::ustring> col_name;
		Gtk::TreeModelColumn<Glib::ustring> col_path;
		Gtk::TreeModelColumn<bool>          col_is_dir;
		Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf> > col_icon;
	};
	ExplorerColumns _explorer_cols;
	Glib::RefPtr<Gtk::TreeStore> _explorer_model;

	std::string _explorer_root;

	sigc::signal<void, std::string, std::string> _signal_file_selected;
	sigc::signal<void>                           _signal_close_requested;
	sigc::signal<void, std::string>              _signal_log_message;
	sigc::signal<void, std::string, std::string> _signal_file_renamed;

	void setup_ui();
	void build_context_menu();
	std::string get_default_root_path() const;

	void populate_dir(const std::string& dir, const Gtk::TreeModel::Row* parent);
	bool find_and_select_path_in_tree(const Gtk::TreeNodeChildren& children, const std::string& target_path);
	void create_new_script();
	void rename_selected_item();
	void delete_selected_item();
	void open_selected_item();

	bool on_key_press(GdkEventKey* event);
	bool on_button_press(GdkEventButton* event);
	void on_row_activated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* col);

	static Glib::RefPtr<Gdk::Pixbuf> load_scaled_pixbuf(const std::string& filename, int size);
	static Gtk::Image* load_scaled_icon(const std::string& filename, int size);
};

#endif /* __gtk2_ardour_nova_file_explorer_h__ */