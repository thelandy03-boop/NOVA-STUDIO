#include <ytk/ytk.h>
#include "nova_toast.h"
#include "ardour_ui.h"
#include "pbd/i18n.h"
#include <algorithm>

std::queue<NovaToast::Pending> NovaToast::_queue;
std::vector<NovaToast*>        NovaToast::_active;

void
NovaToast::show (const std::string& message, NovaToastType type, int duration_ms)
{
	Pending p;
	p.msg = message;
	p.type = type;
	p.duration = duration_ms;
	_queue.push (p);
	pump_queue ();
}

void NovaToast::show_success (const std::string& m, int d) { show (m, TOAST_SUCCESS, d); }
void NovaToast::show_info    (const std::string& m, int d) { show (m, TOAST_INFO,    d); }
void NovaToast::show_warn    (const std::string& m, int d) { show (m, TOAST_WARN,    d); }
void NovaToast::show_error   (const std::string& m, int d) { show (m, TOAST_ERROR,   d); }

int
NovaToast::visible_count ()
{
	return static_cast<int> (_active.size ());
}

void
NovaToast::pump_queue ()
{
	while (!_queue.empty () && visible_count () < MAX_VISIBLE) {
		Pending p = _queue.front ();
		_queue.pop ();
		NovaToast* t = new NovaToast (p.msg, p.type, p.duration);
		_active.push_back (t);
		t->_stack_index = visible_count () - 1;
		t->place_inside_ardour (t->_stack_index);
		t->Gtk::Window::show ();
		t->start_life_cycle ();
	}
}

NovaToast::NovaToast (const std::string& message, NovaToastType type, int duration_ms)
	: _type (type)
	, _duration_ms (duration_ms)
	, _stack_index (0)
{
	set_decorated (false);
	set_skip_taskbar_hint (true);
	set_skip_pager_hint (true);
	set_accept_focus (false);
	set_keep_above (true);
	set_type_hint (Gdk::WINDOW_TYPE_HINT_UTILITY);
	set_default_size (TOAST_W, TOAST_H);
	set_resizable (false);
	set_border_width (0);

	Gtk::Frame* frame = Gtk::manage (new Gtk::Frame ());
	frame->set_shadow_type (Gtk::SHADOW_ETCHED_OUT);

	_box.modify_bg (Gtk::STATE_NORMAL, Gdk::Color ("#222225"));
	_box.set_border_width (0);
	_box.add_events (Gdk::BUTTON_PRESS_MASK);
	_box.signal_button_press_event ().connect (
		sigc::mem_fun (*this, &NovaToast::on_click));

	std::string icon = "●";
	if (_type == TOAST_SUCCESS) icon = "✓";
	else if (_type == TOAST_WARN) icon = "⚠";
	else if (_type == TOAST_ERROR) icon = "✕";

	_lbl.set_markup (
		"<span foreground='#FFFFFF' weight='bold'>" + icon + "  " +
		Glib::Markup::escape_text (message) + "</span>");
	_lbl.set_alignment (0.0, 0.5);
	_lbl.set_padding (12, 0);
	_lbl.set_ellipsize (Pango::ELLIPSIZE_NONE);

	_box.add (_lbl);
	frame->add (_box);
	add (*frame);
	show_all_children ();
}

NovaToast::~NovaToast ()
{
	if (_timeout.connected ()) {
		_timeout.disconnect ();
	}
	_active.erase (std::remove (_active.begin (), _active.end (), this), _active.end ());
	for (size_t i = 0; i < _active.size (); ++i) {
		_active[i]->_stack_index = static_cast<int> (i);
		_active[i]->place_inside_ardour (static_cast<int> (i));
	}
	pump_queue ();
}

void
NovaToast::start_life_cycle ()
{
	_timeout = Glib::signal_timeout ().connect (
		sigc::mem_fun (*this, &NovaToast::on_timeout_close),
		_duration_ms);
}

void
NovaToast::dismiss ()
{
	if (_timeout.connected ()) {
		_timeout.disconnect ();
	}
	hide ();
	Glib::signal_idle().connect(sigc::bind(sigc::ptr_fun(&NovaToast::destroy_toast), this));
}

bool
NovaToast::destroy_toast (NovaToast* toast)
{
	delete toast;
	return false;
}

bool
NovaToast::on_timeout_close ()
{
	dismiss ();
	return false;
}

bool
NovaToast::on_click (GdkEventButton* ev)
{
	if (ev->button == 1) {
		dismiss ();
	}
	return true;
}

void
NovaToast::place_inside_ardour (int stack_index)
{
	Gtk::Window* main_win = nullptr;
	if (ARDOUR_UI::instance()) {
		main_win = &ARDOUR_UI::instance()->main_window();
	}

	int wx = 0, wy = 0;
	int ww = 0, wh = 0;

	if (main_win && main_win->get_window()) {
		set_transient_for (*main_win);
		main_win->get_window()->get_origin (wx, wy);
		ww = main_win->get_allocation().get_width();
		wh = main_win->get_allocation().get_height();
	} else {
		Glib::RefPtr<Gdk::Screen> screen = Gdk::Screen::get_default ();
		ww = screen->get_width ();
		wh = screen->get_height ();
	}

	// Márgenes pegados al borde derecho inferior
	const int margin_right = 10;
	const int margin_bottom = 28;

	int x = wx + ww - TOAST_W - margin_right;
	int y = wy + wh - TOAST_H - margin_bottom - stack_index * (TOAST_H + GAP);

	if (y < wy + margin_bottom) {
		y = wy + margin_bottom;
	}

	move (x, y);
}