#include "nova_file_explorer.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <vector>
#include <functional>

#include <glib/gstdio.h>
#include <glibmm/fileutils.h>

#include <ytkmm/stock.h>
#include <ytkmm/messagedialog.h>
#include <ytkmm/cellrendererpixbuf.h>
#include <ytkmm/cellrenderertext.h>
#include <ytkmm/dialog.h>
#include <ytkmm/entry.h>

#include "pbd/compose.h"
#include "pbd/file_utils.h"
#include "pbd/gstdio_compat.h"

#include "ardour/filesystem_paths.h"
#include "gtkmm2ext/gui_thread.h"
#include "gtkmm2ext/utils.h"
#include "pbd/i18n.h"

#ifndef GDK_F2
#define GDK_F2 0xffc3
#endif
#ifndef GDK_Delete
#define GDK_Delete 0xffff
#endif
#ifndef GDK_Return
#define GDK_Return 0xff0d
#endif

static std::string get_icon_path (const std::string& filename)
{
	std::vector<std::string> search_dirs;
	search_dirs.push_back (Glib::build_filename (ARDOUR::user_config_directory (), "icons", "scripting"));
	search_dirs.push_back (Glib::build_filename ("gtk2_ardour", "icons", "scripting"));
	search_dirs.push_back (Glib::build_filename ("icons", "scripting"));
	search_dirs.push_back (Glib::build_filename ("share", "icons", "scripting"));

	for (const auto& dir : search_dirs) {
		std::string full_path = Glib::build_filename (dir, filename);
		if (Glib::file_test (full_path, Glib::FILE_TEST_EXISTS)) {
			return full_path;
		}
	}
	return "";
}

Glib::RefPtr<Gdk::Pixbuf> NovaFileExplorer::load_scaled_pixbuf (const std::string& filename, int size)
{
	std::string path = get_icon_path (filename);

	if (path.empty ()) {
		if (filename.find ("32.png") != std::string::npos) {
			std::string alt = filename;
			size_t p = alt.find ("32.png");
			alt.replace (p, 6, ".png");
			path = get_icon_path (alt);
		} else if (filename.find ("64.png") != std::string::npos) {
			std::string alt = filename;
			size_t p = alt.find ("64.png");
			alt.replace (p, 6, ".png");
			path = get_icon_path (alt);
		} else if (filename.find (".png") != std::string::npos) {
			std::string alt = filename;
			size_t p = alt.find (".png");
			alt.replace (p, 4, "32.png");
			path = get_icon_path (alt);
		}
	}

	if (!path.empty ()) {
		try {
			Glib::RefPtr<Gdk::Pixbuf> pix = Gdk::Pixbuf::create_from_file (path);
			if (pix) {
				return pix->scale_simple (size, size, Gdk::INTERP_BILINEAR);
			}
		} catch (...) {}
	}
	return Glib::RefPtr<Gdk::Pixbuf>();
}

Gtk::Image* NovaFileExplorer::load_scaled_icon (const std::string& filename, int size)
{
	Glib::RefPtr<Gdk::Pixbuf> pix = load_scaled_pixbuf (filename, size);
	if (pix) {
		return Gtk::manage (new Gtk::Image (pix));
	}
	return Gtk::manage (new Gtk::Image ());
}

NovaFileExplorer::NovaFileExplorer()
{
	setup_ui();
	build_context_menu();
	refresh();
}

NovaFileExplorer::~NovaFileExplorer()
{
}

void NovaFileExplorer::setup_ui()
{
	_explorer_title.set_markup ("<b>EXPLORER</b>");
	_explorer_title.set_alignment (0.0, 0.5);

	Gtk::Image* img_add = load_scaled_icon ("add32.png", 14);
	if (!img_add || img_add->get_pixbuf().operator->() == nullptr) {
		img_add = load_scaled_icon ("add64.png", 14);
	}
	if (img_add) {
		_btn_explorer_add.add (*img_add);
	} else {
		_btn_explorer_add.set_label ("+");
	}
	_btn_explorer_add.set_tooltip_text (_("Create New Script in Selected Folder"));

	Gtk::Image* img_cls = load_scaled_icon ("delete32.png", 12);
	if (img_cls) {
		_btn_explorer_close.add (*img_cls);
	} else {
		_btn_explorer_close.set_label ("X");
	}

	_explorer_header.set_spacing (4);
	_explorer_header.set_border_width (4);
	_explorer_header.pack_start (_explorer_title, true, true, 0);
	_explorer_header.pack_end (_btn_explorer_close, false, false, 0);
	_explorer_header.pack_end (_btn_explorer_add, false, false, 0);

	_explorer_model = Gtk::TreeStore::create (_explorer_cols);
	_explorer_tree.set_model (_explorer_model);
	_explorer_tree.set_headers_visible (false);

	Gtk::TreeViewColumn* col_explorer = Gtk::manage (new Gtk::TreeViewColumn ());
	Gtk::CellRendererPixbuf* renderer_pixbuf = Gtk::manage (new Gtk::CellRendererPixbuf ());
	Gtk::CellRendererText* renderer_text = Gtk::manage (new Gtk::CellRendererText ());

	col_explorer->pack_start (*renderer_pixbuf, false);
	col_explorer->pack_start (*renderer_text, true);

	col_explorer->add_attribute (renderer_pixbuf->property_pixbuf (), _explorer_cols.col_icon);
	col_explorer->add_attribute (renderer_text->property_text (), _explorer_cols.col_name);

	_explorer_tree.append_column (*col_explorer);
	_explorer_tree.set_enable_search (true);
	_explorer_tree.set_search_column (_explorer_cols.col_name.index ());
	_explorer_tree.set_can_focus (true);

	_explorer_scroll.set_policy (Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
	_explorer_scroll.set_shadow_type (Gtk::SHADOW_IN);
	_explorer_scroll.add (_explorer_tree);

	set_spacing (2);
	set_size_request (240, -1);
	pack_start (_explorer_header, false, false, 0);
	pack_start (_explorer_scroll, true, true, 0);

	/* Signals */
	_btn_explorer_close.signal_clicked().connect([this]() {
		_signal_close_requested.emit();
	});

	_btn_explorer_add.signal_clicked().connect(sigc::mem_fun(*this, &NovaFileExplorer::create_new_script));

	_explorer_tree.signal_row_activated().connect(sigc::mem_fun(*this, &NovaFileExplorer::on_row_activated));
	_explorer_tree.signal_key_press_event().connect(
		sigc::mem_fun(*this, &NovaFileExplorer::on_key_press), false);
	_explorer_tree.signal_button_press_event().connect(
		sigc::mem_fun(*this, &NovaFileExplorer::on_button_press), false);
}

void NovaFileExplorer::build_context_menu()
{
	Gtk::MenuItem* item_open = Gtk::manage (new Gtk::MenuItem (_("Open File"), false));
	item_open->signal_activate().connect (sigc::mem_fun (*this, &NovaFileExplorer::open_selected_item));
	_explorer_context_menu.append (*item_open);

	Gtk::MenuItem* item_new = Gtk::manage (new Gtk::MenuItem (_("New Script..."), false));
	item_new->signal_activate().connect (sigc::mem_fun (*this, &NovaFileExplorer::create_new_script));
	_explorer_context_menu.append (*item_new);

	Gtk::MenuItem* item_rename = Gtk::manage (new Gtk::MenuItem (_("Rename (F2)"), false));
	item_rename->signal_activate().connect (sigc::mem_fun (*this, &NovaFileExplorer::rename_selected_item));
	_explorer_context_menu.append (*item_rename);

	Gtk::MenuItem* item_delete = Gtk::manage (new Gtk::MenuItem (_("Delete File"), false));
	item_delete->signal_activate().connect (sigc::mem_fun (*this, &NovaFileExplorer::delete_selected_item));
	_explorer_context_menu.append (*item_delete);

	_explorer_context_menu.show_all ();
}

std::string NovaFileExplorer::get_default_root_path() const
{
	std::string scripts = Glib::build_filename (ARDOUR::user_config_directory (), "scripts");
	if (!Glib::file_test (scripts, Glib::FILE_TEST_EXISTS)) {
		g_mkdir_with_parents (scripts.c_str (), 0755);
	}
	return scripts;
}

void NovaFileExplorer::set_root_override(const std::string& path)
{
	if (!path.empty() && Glib::file_test(path, Glib::FILE_TEST_IS_DIR)) {
		_explorer_root = path;
	}
}

void NovaFileExplorer::refresh()
{
	_explorer_model->clear ();
	if (_explorer_root.empty()) {
		_explorer_root = get_default_root_path();
	}

	static Glib::RefPtr<Gdk::Pixbuf> icon_folder = load_scaled_pixbuf ("folder32.png", 16);

	std::string base_name = Glib::path_get_basename (_explorer_root);
	if (base_name.empty ()) {
		base_name = _explorer_root;
	}

	Gtk::TreeModel::Row root = *(_explorer_model->append ());
	root[_explorer_cols.col_name]   = base_name;
	root[_explorer_cols.col_path]   = _explorer_root;
	root[_explorer_cols.col_is_dir] = true;
	root[_explorer_cols.col_icon]   = icon_folder;

	populate_dir (_explorer_root, &root);

	Gtk::TreeModel::Path p;
	p.push_back (0);
	_explorer_tree.expand_row (p, false);

	_explorer_tree.show_all();

	_explorer_title.set_markup ("<b>EXPLORER</b>");
	_explorer_title.set_tooltip_text (_explorer_root);
}

void NovaFileExplorer::populate_dir(const std::string& dir, const Gtk::TreeModel::Row* parent)
{
	static Glib::RefPtr<Gdk::Pixbuf> icon_folder = load_scaled_pixbuf ("folder32.png", 16);
	static Glib::RefPtr<Gdk::Pixbuf> icon_file   = load_scaled_pixbuf ("document2_32.png", 16);

	try {
		Glib::Dir gdir (dir);
		std::vector<std::string> dirs;
		std::vector<std::string> files;

		for (Glib::DirIterator it = gdir.begin (); it != gdir.end (); ++it) {
			std::string name = *it;
			if (name == "." || name == "..") continue;
			if (!name.empty () && name[0] == '.') continue;

			std::string full = Glib::build_filename (dir, name);
			if (Glib::file_test (full, Glib::FILE_TEST_IS_DIR)) {
				dirs.push_back (name);
			} else {
				std::string lower = name;
				std::transform (lower.begin (), lower.end (), lower.begin (), ::tolower);
				if (lower.size () >= 4) {
					std::string ext = lower.substr (lower.size () - 4);
					if (ext == ".lua" || ext == ".md" || ext == "son" ||
					    lower.find (".lua") != std::string::npos ||
					    lower.find (".json") != std::string::npos ||
					    lower.find (".txt") != std::string::npos) {
						files.push_back (name);
					}
				} else if (lower == "readme" || lower == "makefile") {
					files.push_back (name);
				}
			}
		}

		std::sort (dirs.begin (), dirs.end ());
		std::sort (files.begin (), files.end ());

		for (const auto& d : dirs) {
			std::string full = Glib::build_filename (dir, d);
			Gtk::TreeModel::Row row;
			if (parent) {
				row = *(_explorer_model->append (parent->children ()));
			} else {
				row = *(_explorer_model->append ());
			}
			row[_explorer_cols.col_name]   = d;
			row[_explorer_cols.col_path]   = full;
			row[_explorer_cols.col_is_dir] = true;
			row[_explorer_cols.col_icon]   = icon_folder;

			try {
				Glib::Dir sub (full);
				std::vector<std::string> sub_dirs, sub_files;
				for (Glib::DirIterator sit = sub.begin (); sit != sub.end (); ++sit) {
					std::string sn = *sit;
					if (sn == "." || sn == ".." || (!sn.empty () && sn[0] == '.')) continue;
					std::string sfull = Glib::build_filename (full, sn);
					if (Glib::file_test (sfull, Glib::FILE_TEST_IS_DIR)) {
						sub_dirs.push_back (sn);
					} else {
						std::string low = sn;
						std::transform (low.begin (), low.end (), low.begin (), ::tolower);
						if (low.find (".lua") != std::string::npos ||
						    low.find (".md") != std::string::npos ||
						    low.find (".json") != std::string::npos ||
						    low.find (".txt") != std::string::npos) {
							sub_files.push_back (sn);
						}
					}
				}
				std::sort (sub_dirs.begin (), sub_dirs.end ());
				std::sort (sub_files.begin (), sub_files.end ());
				for (const auto& sd : sub_dirs) {
					Gtk::TreeModel::Row r = *(_explorer_model->append (row.children ()));
					r[_explorer_cols.col_name]   = sd;
					r[_explorer_cols.col_path]   = Glib::build_filename (full, sd);
					r[_explorer_cols.col_is_dir] = true;
					r[_explorer_cols.col_icon]   = icon_folder;
				}
				for (const auto& sf : sub_files) {
					Gtk::TreeModel::Row r = *(_explorer_model->append (row.children ()));
					r[_explorer_cols.col_name]   = sf;
					r[_explorer_cols.col_path]   = Glib::build_filename (full, sf);
					r[_explorer_cols.col_is_dir] = false;
					r[_explorer_cols.col_icon]   = icon_file;
				}
			} catch (...) {}
		}

		for (const auto& f : files) {
			Gtk::TreeModel::Row row;
			if (parent) {
				row = *(_explorer_model->append (parent->children ()));
			} else {
				row = *(_explorer_model->append ());
			}
			row[_explorer_cols.col_name]   = f;
			row[_explorer_cols.col_path]   = Glib::build_filename (dir, f);
			row[_explorer_cols.col_is_dir] = false;
			row[_explorer_cols.col_icon]   = icon_file;
		}
	} catch (const Glib::FileError& e) {
		_signal_log_message.emit (std::string ("> [Explorer] ") + e.what () + "\n");
	}
}

bool NovaFileExplorer::find_and_select_path_in_tree (const Gtk::TreeNodeChildren& children, const std::string& target_path)
{
	for (Gtk::TreeModel::iterator it = children.begin(); it != children.end(); ++it) {
		Gtk::TreeModel::Row row = *it;
		Glib::ustring row_path_u = row[_explorer_cols.col_path];
		std::string row_path = row_path_u;

		if (row_path == target_path) {
			Gtk::TreeModel::Path p = _explorer_model->get_path (it);
			_explorer_tree.expand_to_path (p);
			_explorer_tree.get_selection()->select (it);
			_explorer_tree.set_cursor (p);
			_explorer_tree.scroll_to_row (p, 0.5);
			return true;
		}

		bool is_dir = row[_explorer_cols.col_is_dir];
		if (is_dir) {
			if (find_and_select_path_in_tree (row.children(), target_path)) {
				Gtk::TreeModel::Path parent_path = _explorer_model->get_path (it);
				_explorer_tree.expand_row (parent_path, false);
				return true;
			}
		}
	}
	return false;
}

void NovaFileExplorer::create_new_script()
{
	std::string target_dir = _explorer_root;

	Glib::RefPtr<Gtk::TreeSelection> sel = _explorer_tree.get_selection();
	if (sel) {
		Gtk::TreeModel::iterator it = sel->get_selected();
		if (it) {
			Gtk::TreeModel::Row row = *it;
			bool is_dir = row[_explorer_cols.col_is_dir];
			Glib::ustring p = row[_explorer_cols.col_path];
			if (is_dir) {
				target_dir = p;
			} else if (!p.empty()) {
				target_dir = Glib::path_get_dirname(p);
			}
		}
	}

	if (target_dir.empty() || !Glib::file_test(target_dir, Glib::FILE_TEST_IS_DIR)) {
		target_dir = get_default_root_path();
	}

	Gtk::Dialog dialog(_("Create New Script"), true);
	dialog.set_position(Gtk::WIN_POS_CENTER);
	dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	dialog.add_button(_("Create"), Gtk::RESPONSE_ACCEPT);
	dialog.set_default_response (Gtk::RESPONSE_ACCEPT);

	Gtk::Label* lbl = Gtk::manage(new Gtk::Label(_("Script Name:")));
	lbl->set_alignment(0.0, 0.5);

	Gtk::Entry entry;
	entry.set_text("new_script.lua");
	entry.set_activates_default (true);

	Gtk::VBox* box = dialog.get_vbox();
	box->set_spacing(6);
	box->set_border_width(12);
	box->pack_start(*lbl, false, false, 0);
	box->pack_start(entry, false, false, 0);

	dialog.show_all();
	entry.grab_focus ();
	entry.select_region (0, (int)std::string("new_script").size());

	int resp = dialog.run();
	std::string filename = entry.get_text();

	dialog.hide();
	if (Gtkmm2ext::UI::instance()) {
		Gtkmm2ext::UI::instance()->flush_pending(0.05);
	}

	if (resp == Gtk::RESPONSE_ACCEPT) {
		if (filename.empty()) filename = "new_script.lua";
		if (filename.find(".lua") == std::string::npos) {
			filename += ".lua";
		}

		std::string full_path = Glib::build_filename(target_dir, filename);

		if (Glib::file_test(full_path, Glib::FILE_TEST_EXISTS)) {
			_signal_log_message.emit("> [Explorer Error] File already exists: " + full_path + "\n");
			return;
		}

		std::string script_title = Glib::path_get_basename(filename);
		std::string template_code =
			"---- this header is required to save the script\n"
			"-- ardour { [\"type\"] = \"Snippet\", name = \"" + script_title + "\", author = \"NOVA\" }\n\n"
			"function factory ()\n"
			"    return function ()\n"
			"        local session = Session:instance()\n"
			"        -- Your script code here\n"
			"    end\n"
			"end\n";

		FILE* f = g_fopen (full_path.c_str(), "wb");
		if (f) {
			fwrite (template_code.c_str(), 1, template_code.size(), f);
			fflush (f);
			fclose (f);

			refresh();
			find_and_select_path_in_tree (_explorer_model->children(), full_path);

			_signal_log_message.emit("> Created new script: " + full_path + "\n");
			_signal_file_selected.emit(full_path, template_code);
		} else {
			_signal_log_message.emit("> [Explorer Error] Failed to create file: " + full_path + "\n");
		}
	}
}

void NovaFileExplorer::open_selected_item()
{
	Glib::RefPtr<Gtk::TreeSelection> sel = _explorer_tree.get_selection ();
	if (!sel) return;

	Gtk::TreeModel::iterator it = sel->get_selected ();
	if (!it) return;

	Gtk::TreeModel::Row row = *it;
	bool is_dir = row[_explorer_cols.col_is_dir];
	Glib::ustring path_u = row[_explorer_cols.col_path];

	if (!is_dir) {
		_signal_file_selected.emit(path_u, std::string());
	} else {
		Gtk::TreeModel::Path p = _explorer_model->get_path (it);
		if (_explorer_tree.row_expanded (p)) {
			_explorer_tree.collapse_row (p);
		} else {
			_explorer_tree.expand_row (p, false);
		}
	}
}

void NovaFileExplorer::delete_selected_item()
{
	Glib::RefPtr<Gtk::TreeSelection> sel = _explorer_tree.get_selection ();
	if (!sel) return;

	Gtk::TreeModel::iterator it = sel->get_selected ();
	if (!it) return;

	Gtk::TreeModel::Row row = *it;
	bool is_dir = row[_explorer_cols.col_is_dir];
	Glib::ustring path_u = row[_explorer_cols.col_path];
	Glib::ustring name_u = row[_explorer_cols.col_name];

	std::string target_path = path_u;
	std::string target_name = name_u;

	if (target_path.empty () || target_path == _explorer_root) {
		_signal_log_message.emit("> [Explorer] Cannot delete root folder.\n");
		return;
	}

	Gtk::MessageDialog dialog (
		string_compose (_("Are you sure you want to delete '%1'?"), target_name),
		false,
		Gtk::MESSAGE_QUESTION,
		Gtk::BUTTONS_YES_NO,
		true
	);
	dialog.set_position(Gtk::WIN_POS_CENTER);

	if (dialog.run () == Gtk::RESPONSE_YES) {
		if (is_dir) {
			if (g_rmdir (target_path.c_str ()) == 0) {
				_signal_log_message.emit("> Deleted folder: " + target_name + "\n");
			} else {
				_signal_log_message.emit("> [Explorer Error] Failed to delete folder (ensure it is empty).\n");
			}
		} else {
			if (g_remove (target_path.c_str ()) == 0) {
				_signal_log_message.emit("> Deleted file: " + target_name + "\n");
			} else {
				_signal_log_message.emit("> [Explorer Error] Failed to delete file.\n");
			}
		}
		refresh ();
	}
}

void NovaFileExplorer::rename_selected_item()
{
	Glib::RefPtr<Gtk::TreeSelection> sel = _explorer_tree.get_selection ();
	if (!sel) return;

	Gtk::TreeModel::iterator it = sel->get_selected ();
	if (!it) {
		_signal_log_message.emit("> [Explorer] Select a file or folder to rename (F2).\n");
		return;
	}

	Gtk::TreeModel::Row row = *it;
	bool is_dir = row[_explorer_cols.col_is_dir];
	Glib::ustring old_path_u = row[_explorer_cols.col_path];
	Glib::ustring old_name_u = row[_explorer_cols.col_name];

	std::string old_path = old_path_u;
	std::string old_name = old_name_u;

	if (old_path.empty () || old_path == _explorer_root) {
		_signal_log_message.emit("> [Explorer] Cannot rename root folder.\n");
		return;
	}

	Gtk::Dialog dialog (_("Rename"), true);
	dialog.set_position(Gtk::WIN_POS_CENTER);
	dialog.add_button (Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	dialog.add_button (_("Rename"), Gtk::RESPONSE_ACCEPT);
	dialog.set_default_response (Gtk::RESPONSE_ACCEPT);

	Gtk::Label* lbl = Gtk::manage (new Gtk::Label (
		is_dir ? _("Folder name:") : _("Script name:")));
	lbl->set_alignment (0.0, 0.5);

	Gtk::Entry entry;
	entry.set_activates_default (true);

	if (!is_dir) {
		size_t dot = old_name.find_last_of ('.');
		if (dot != std::string::npos && dot > 0) {
			entry.set_text (old_name.substr (0, dot));
		} else {
			entry.set_text (old_name);
		}
	} else {
		entry.set_text (old_name);
	}
	entry.select_region (0, -1);

	Gtk::VBox* box = dialog.get_vbox ();
	box->set_spacing (6);
	box->set_border_width (12);
	box->pack_start (*lbl, false, false, 0);
	box->pack_start (entry, false, false, 0);
	dialog.show_all ();
	entry.grab_focus ();

	if (dialog.run () != Gtk::RESPONSE_ACCEPT) {
		return;
	}

	std::string new_name = entry.get_text ();
	size_t a = new_name.find_first_not_of (" \t");
	size_t b = new_name.find_last_not_of (" \t");
	if (a == std::string::npos) {
		_signal_log_message.emit("> [Explorer Error] Empty name.\n");
		return;
	}
	new_name = new_name.substr (a, b - a + 1);

	if (new_name.empty ()) {
		_signal_log_message.emit("> [Explorer Error] Empty name.\n");
		return;
	}

	const std::string bad = "<>:\"/\\|?*";
	if (new_name.find_first_of (bad) != std::string::npos) {
		_signal_log_message.emit("> [Explorer Error] Invalid characters in name.\n");
		return;
	}

	if (!is_dir) {
		std::string lower = new_name;
		std::transform (lower.begin (), lower.end (), lower.begin (), ::tolower);
		if (lower.size () < 4 || lower.substr (lower.size () - 4) != ".lua") {
			new_name += ".lua";
		}
	}

	if (new_name == old_name) {
		return;
	}

	std::string parent_dir = Glib::path_get_dirname (old_path);
	std::string new_path = Glib::build_filename (parent_dir, new_name);

	if (Glib::file_test (new_path, Glib::FILE_TEST_EXISTS)) {
		_signal_log_message.emit("> [Explorer Error] File or folder already exists: " + new_path + "\n");
		return;
	}

	if (g_rename (old_path.c_str (), new_path.c_str ()) != 0) {
		_signal_log_message.emit("> [Explorer Error] Failed to rename on disk.\n");
		return;
	}

	_signal_file_renamed.emit (old_path, new_path);
	_signal_log_message.emit("> Renamed: " + old_name + " -> " + new_name + "\n");
	refresh ();
}

bool NovaFileExplorer::on_button_press (GdkEventButton* event)
{
	if (!event) return false;

	if (event->type == GDK_BUTTON_PRESS && event->button == 3) {
		Gtk::TreeModel::Path path;
		Gtk::TreeViewColumn* col = nullptr;
		int cell_x = 0, cell_y = 0;

		if (_explorer_tree.get_path_at_pos ((int)event->x, (int)event->y, path, col, cell_x, cell_y)) {
			_explorer_tree.get_selection()->select (path);
		}

		_explorer_context_menu.popup (event->button, event->time);
		return true;
	}

	return false;
}

bool NovaFileExplorer::on_key_press (GdkEventKey* event)
{
	if (!event) return false;

	uint32_t kv = event->keyval;

	if (kv == 0xFFC3 || kv == 0xffc3 || kv == GDK_F2) {
		rename_selected_item ();
		return true;
	}

	if (kv == 0xFFFF || kv == 0xffff || kv == GDK_Delete) {
		delete_selected_item ();
		return true;
	}

	return false;
}

void NovaFileExplorer::on_row_activated (const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* /*col*/)
{
	Gtk::TreeModel::iterator it = _explorer_model->get_iter (path);
	if (!it) return;

	Gtk::TreeModel::Row row = *it;
	bool is_dir = row[_explorer_cols.col_is_dir];
	Glib::ustring fpath = row[_explorer_cols.col_path];

	if (is_dir) {
		if (_explorer_tree.row_expanded (path)) {
			_explorer_tree.collapse_row (path);
		} else {
			_explorer_tree.expand_row (path, false);
		}
		return;
	}

	_signal_file_selected.emit (fpath, std::string());
}