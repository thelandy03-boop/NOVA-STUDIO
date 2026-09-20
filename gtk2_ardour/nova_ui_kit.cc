#include <ytk/ytk.h>
#include "nova_ui_kit.h"
#include <cairo.h>
#include <cmath>
#include <cstdio>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef M_PI_2
#define M_PI_2 1.57079632679489661923
#endif

/* Helper Hexadecimal a RGBA en Cairo */
static void set_cairo_color(cairo_t* cr, const std::string& hex, double alpha = 1.0) {
	unsigned int v = 0x222225;
	if (hex.size() >= 7 && hex[0] == '#') {
		std::sscanf(hex.c_str() + 1, "%x", &v);
	}
	double r = ((v >> 16) & 0xff) / 255.0;
	double g = ((v >> 8)  & 0xff) / 255.0;
	double b = ( v        & 0xff) / 255.0;
	cairo_set_source_rgba(cr, r, g, b, alpha);
}

/* Helper Borde Redondeado */
static void draw_rounded_rect(cairo_t* cr, double x, double y, double w, double h, double r) {
	if (r <= 0.1) {
		cairo_rectangle(cr, x, y, w, h);
		return;
	}
	cairo_new_sub_path(cr);
	cairo_arc(cr, x + w - r, y + r,     r, -M_PI_2, 0);
	cairo_arc(cr, x + w - r, y + h - r, r, 0,        M_PI_2);
	cairo_arc(cr, x + r,     y + h - r, r, M_PI_2,   M_PI);
	cairo_arc(cr, x + r,     y + r,     r, M_PI,     3 * M_PI_2);
	cairo_close_path(cr);
}

/* ------------------------------------------------------------------- */
/*  NOVA STYLE CONSTRUCTORS                                           */
/* ------------------------------------------------------------------- */
NovaStyle::NovaStyle()
	: bg_color("#222225")
	, bg_gradient_end("")
	, border_color("#3A3A40")
	, text_color("#FFFFFF")
	, hover_bg("#2E2E35")
	, border_radius(6.0)
	, border_width(1.0)
	, font_size(11)
	, font_bold(true)
	, padding_x(12)
	, padding_y(8)
	, width(-1)
	, height(-1)
	, has_shadow(false)
	, has_glow(false)
{}

NovaStyle NovaStyle::button_primary() {
	NovaStyle s;
	s.bg_color = "#1E1E24";
	s.hover_bg = "#2A2A35";
	s.border_color = "#3A3A45";
	s.text_color = "#FFFFFF";
	s.border_radius = 6.0;
	return s;
}

NovaStyle NovaStyle::button_secondary() {
	NovaStyle s;
	s.bg_color = "#141418";
	s.hover_bg = "#1F1F26";
	s.border_color = "#2A2A30";
	s.text_color = "#AAAAAA";
	s.border_radius = 6.0;
	return s;
}

NovaStyle NovaStyle::button_danger() {
	NovaStyle s;
	s.bg_color = "#3A1414";
	s.hover_bg = "#501A1A";
	s.border_color = "#FF4D4D";
	s.text_color = "#FF8080";
	s.border_radius = 6.0;
	return s;
}

NovaStyle NovaStyle::input_box() {
	NovaStyle s;
	s.bg_color = "#121215";
	s.border_color = "#2E2E36";
	s.text_color = "#FFFFFF";
	s.border_radius = 6.0;
	s.padding_x = 8;
	s.padding_y = 6;
	return s;
}

NovaStyle NovaStyle::card() {
	NovaStyle s;
	s.bg_color = "#1A1A1E";
	s.border_color = "#282830";
	s.border_radius = 8.0;
	s.padding_x = 12;
	s.padding_y = 12;
	return s;
}

NovaStyle NovaStyle::status_pill() {
	NovaStyle s;
	s.bg_color = "#102A45";
	s.bg_gradient_end = "#0A1B30";
	s.border_color = "#00F0FF";
	s.text_color = "#E0F2FE";
	s.border_radius = 18.0;
	return s;
}

/* ------------------------------------------------------------------- */
/*  WIDGET IMPLEMENTATION INTERNA                                      */
/* ------------------------------------------------------------------- */
class NovaCustomButton : public Gtk::EventBox {
public:
	NovaCustomButton(const std::string& text, const NovaStyle& style, sigc::slot<void> click_slot)
		: _text(text), _style(style), _is_hovered(false), _click_slot(click_slot)
	{
		set_app_paintable(true);
		if (_style.width > 0 || _style.height > 0) {
			set_size_request(_style.width, _style.height);
		}

		_lbl.set_markup("<span weight='" + std::string(_style.font_bold ? "bold" : "normal") + 
		                "' foreground='" + _style.text_color + "'>" + _text + "</span>");
		_lbl.set_alignment(0.5, 0.5);
		_lbl.set_padding(_style.padding_x, _style.padding_y);

		add(_lbl);

		signal_expose_event().connect(sigc::mem_fun(*this, &NovaCustomButton::on_expose));
		signal_enter_notify_event().connect(sigc::mem_fun(*this, &NovaCustomButton::on_hover_in));
		signal_leave_notify_event().connect(sigc::mem_fun(*this, &NovaCustomButton::on_hover_out));
		signal_button_press_event().connect(sigc::mem_fun(*this, &NovaCustomButton::on_click));

		add_events(Gdk::ENTER_NOTIFY_MASK | Gdk::LEAVE_NOTIFY_MASK | Gdk::BUTTON_PRESS_MASK);
	}

private:
	std::string _text;
	NovaStyle _style;
	bool _is_hovered;
	sigc::slot<void> _click_slot;
	Gtk::Label _lbl;

	bool on_expose(GdkEventExpose*) {
		Glib::RefPtr<Gdk::Window> win = get_window();
		if (!win) return false;

		cairo_t* cr = gdk_cairo_create(win->gobj());
		int w = get_allocation().get_width();
		int h = get_allocation().get_height();

		draw_rounded_rect(cr, 1, 1, w - 2, h - 2, _style.border_radius);

		// Fondo (Solid o Gradient)
		if (!_style.bg_gradient_end.empty()) {
			cairo_pattern_t* pat = cairo_pattern_create_linear(0, 0, w, 0);
			// Parse simple para gradient
			cairo_pattern_add_color_stop_rgb(pat, 0, 0.1, 0.3, 0.6);
			cairo_pattern_add_color_stop_rgb(pat, 1, 0.05, 0.15, 0.35);
			cairo_set_source(cr, pat);
			cairo_fill_preserve(cr);
			cairo_pattern_destroy(pat);
		} else {
			set_cairo_color(cr, _is_hovered ? _style.hover_bg : _style.bg_color);
			cairo_fill_preserve(cr);
		}

		// Borde
		set_cairo_color(cr, _style.border_color);
		cairo_set_line_width(cr, _style.border_width);
		cairo_stroke(cr);

		cairo_destroy(cr);
		return false;
	}

	bool on_hover_in(GdkEventCrossing*) {
		_is_hovered = true;
		queue_draw();
		return false;
	}

	bool on_hover_out(GdkEventCrossing*) {
		_is_hovered = false;
		queue_draw();
		return false;
	}

	bool on_click(GdkEventButton* ev) {
		if (ev->button == 1 && _click_slot) {
			_click_slot();
		}
		return true;
	}
};

class NovaCustomInputWrap : public Gtk::EventBox {
public:
	NovaCustomInputWrap(Gtk::Entry& entry, const NovaStyle& style)
		: _style(style)
	{
		set_app_paintable(true);
		
		entry.set_has_frame(false);
		entry.modify_base(Gtk::STATE_NORMAL, Gdk::Color(_style.bg_color));
		entry.modify_text(Gtk::STATE_NORMAL, Gdk::Color(_style.text_color));

		Gtk::HBox* pad = Gtk::manage(new Gtk::HBox());
		pad->set_border_width(_style.padding_y);
		pad->pack_start(entry, true, true, _style.padding_x);

		add(*pad);
		signal_expose_event().connect(sigc::mem_fun(*this, &NovaCustomInputWrap::on_expose));
	}

private:
	NovaStyle _style;

	bool on_expose(GdkEventExpose*) {
		Glib::RefPtr<Gdk::Window> win = get_window();
		if (!win) return false;

		cairo_t* cr = gdk_cairo_create(win->gobj());
		int w = get_allocation().get_width();
		int h = get_allocation().get_height();

		draw_rounded_rect(cr, 1, 1, w - 2, h - 2, _style.border_radius);
		set_cairo_color(cr, _style.bg_color);
		cairo_fill_preserve(cr);

		set_cairo_color(cr, _style.border_color);
		cairo_set_line_width(cr, _style.border_width);
		cairo_stroke(cr);

		cairo_destroy(cr);
		return false;
	}
};

/* ------------------------------------------------------------------- */
/*  NOVA UI KIT FACTORY METHODS                                        */
/* ------------------------------------------------------------------- */
Gtk::Widget* NovaUIKit::create_button(const std::string& text, const NovaStyle& style, sigc::slot<void> on_click) {
	return Gtk::manage(new NovaCustomButton(text, style, on_click));
}

Gtk::Widget* NovaUIKit::create_input(Gtk::Entry& entry_widget, const NovaStyle& style) {
	return Gtk::manage(new NovaCustomInputWrap(entry_widget, style));
}

Gtk::EventBox* NovaUIKit::create_card(Gtk::Widget& child_content, const NovaStyle& style) {
	Gtk::EventBox* box = Gtk::manage(new Gtk::EventBox());
	box->set_app_paintable(true);
	box->add(child_content);

	box->signal_expose_event().connect([box, style](GdkEventExpose*) -> bool {
		Glib::RefPtr<Gdk::Window> win = box->get_window();
		if (!win) return false;

		cairo_t* cr = gdk_cairo_create(win->gobj());
		int w = box->get_allocation().get_width();
		int h = box->get_allocation().get_height();

		draw_rounded_rect(cr, 1, 1, w - 2, h - 2, style.border_radius);
		set_cairo_color(cr, style.bg_color);
		cairo_fill_preserve(cr);

		set_cairo_color(cr, style.border_color);
		cairo_set_line_width(cr, style.border_width);
		cairo_stroke(cr);

		cairo_destroy(cr);
		return false;
	});

	return box;
}

Gtk::Widget* NovaUIKit::create_pill(Gtk::Label& label_widget, const NovaStyle& style) {
	Gtk::EventBox* pill = Gtk::manage(new Gtk::EventBox());
	pill->set_app_paintable(true);
	pill->set_size_request(-1, 36);
	pill->add(label_widget);

	pill->signal_expose_event().connect([pill, style](GdkEventExpose*) -> bool {
		Glib::RefPtr<Gdk::Window> win = pill->get_window();
		if (!win) return false;

		cairo_t* cr = gdk_cairo_create(win->gobj());
		int w = pill->get_allocation().get_width();
		int h = pill->get_allocation().get_height();

		draw_rounded_rect(cr, 1, 1, w - 2, h - 2, (h - 2) / 2.0);

		cairo_pattern_t* pat = cairo_pattern_create_linear(0, 0, w, 0);
		cairo_pattern_add_color_stop_rgb(pat, 0.0, 0.10, 0.30, 0.60);
		cairo_pattern_add_color_stop_rgb(pat, 1.0, 0.05, 0.18, 0.40);
		cairo_set_source(cr, pat);
		cairo_fill_preserve(cr);
		cairo_pattern_destroy(pat);

		set_cairo_color(cr, style.border_color, 0.5);
		cairo_set_line_width(cr, 1.2);
		cairo_stroke(cr);

		cairo_destroy(cr);
		return false;
	});

	return pill;
}