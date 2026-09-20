#include "nova_colab_presence.h"
#include "editor.h"
#include "ardour_ui.h"
#include "rgb_macros.h"
#include "ui_config.h"
#include "pbd/i18n.h"

#include "canvas/types.h"
#include "canvas/scroll_group.h"
#include "canvas/polygon.h"
#include <cstdio>
#include <algorithm>

using namespace Temporal;

NovaColabPresence* NovaColabPresence::_instance = nullptr;

NovaColabPresence&
NovaColabPresence::instance ()
{
	if (!_instance) {
		_instance = new NovaColabPresence ();
	}
	return *_instance;
}

NovaColabPresence::NovaColabPresence ()
	: _editor (nullptr)
	, _group (nullptr)
	, _demo_pos (0)
{
}

NovaColabPresence::~NovaColabPresence ()
{
	stop_demo ();
	clear ();
}

void
NovaColabPresence::attach_to_editor (Editor* ed)
{
	_editor = ed;
	ensure_group ();
	reposition_all ();
}

void
NovaColabPresence::ensure_group ()
{
	if (!_editor || _group) {
		return;
	}

	/* Conexión al grupo de cursores principal de Ardour */
	if (_editor->get_cursor_scroll_group ()) {
		_group = new ArdourCanvas::Container ((ArdourCanvas::Item*) _editor->get_cursor_scroll_group ());
	} else {
		_group = new ArdourCanvas::Container (_editor->get_trackview_group ());
	}
	_group->set_render_with_alpha (true);
	_group->set_ignore_events (true);
}

static uint32_t
hex_to_rgba (const std::string& hex, double alpha = 1.0)
{
	unsigned int v = 0x00F0FF; // Cian neón por defecto
	if (hex.size () >= 7 && hex[0] == '#') {
		std::sscanf (hex.c_str () + 1, "%x", &v);
	}
	double r = ((v >> 16) & 0xff) / 255.0;
	double g = ((v >> 8)  & 0xff) / 255.0;
	double b = ( v        & 0xff) / 255.0;
	return RGBA_TO_UINT(r * 255, g * 255, b * 255, alpha * 255);
}

void
NovaColabPresence::build_items (NovaRemoteUser& u)
{
	ensure_group ();
	if (!_group) {
		return;
	}

	uint32_t cian_color = hex_to_rgba (u.color_hex, 1.0);

	// 1. Línea vertical del Playhead (2.0px Cian Neón)
	u.line = new ArdourCanvas::Line (_group);
	u.line->set_outline_color (cian_color);
	u.line->set_outline_width (2.0);

	// 2. Banderín Triángulo superior (idéntico al rojo nativo)
	u.flag_head = new ArdourCanvas::Polygon (_group);
	u.flag_head->set_fill_color (cian_color);
	u.flag_head->set_outline_color (RGBA_TO_UINT(0, 0, 0, 200));
	u.flag_head->set_outline_width (1.0);

	// 3. Etiqueta con fondo Gris Oscuro DAW y borde Cian Neón
	u.label_bg = new ArdourCanvas::Rectangle (_group);
	u.label_bg->set_fill_color (RGBA_TO_UINT(18, 18, 22, 230)); // Fondo oscuro pro
	u.label_bg->set_outline_color (cian_color);                  // Borde cian neón
	u.label_bg->set_outline_width (1.0);

	// 4. Texto del Colaborador en Cian brillante
	u.label = new ArdourCanvas::Text (_group);
	u.label->set_font_description (Pango::FontDescription ("Sans Bold 8"));
	u.label->set_color (cian_color); // Texto cian neón
	u.label->set (u.name);
}

void
NovaColabPresence::destroy_items (NovaRemoteUser& u)
{
	delete u.label;
	delete u.label_bg;
	delete u.flag_head;
	delete u.line;
	u.label = nullptr;
	u.label_bg = nullptr;
	u.flag_head = nullptr;
	u.line = nullptr;
}

double
NovaColabPresence::sample_to_canvas_x (samplepos_t s) const
{
	if (!_editor) {
		return 0;
	}
	return _editor->sample_to_pixel (s);
}

void
NovaColabPresence::update_item_geometry (NovaRemoteUser& u)
{
	if (!_editor || !u.line) {
		return;
	}

	const double x = sample_to_canvas_x (u.position);
	const double h = 3000.0;

	// A) Línea vertical desde Y = 0 hacia abajo
	u.line->set_x0 (x);
	u.line->set_x1 (x);
	u.line->set_y0 (0);
	u.line->set_y1 (h);

	if (u.visible) {
		u.line->show ();
		u.line->raise_to_top ();
	} else {
		u.line->hide ();
	}

	// B) Banderín Triángulo en la regla superior (exacto como el cursor rojo)
	if (u.flag_head) {
		ArdourCanvas::Points pts;
		pts.push_back (ArdourCanvas::Duple (x - 6, 0));
		pts.push_back (ArdourCanvas::Duple (x + 6, 0));
		pts.push_back (ArdourCanvas::Duple (x, 10));
		u.flag_head->set (pts);

		if (u.visible) {
			u.flag_head->show ();
			u.flag_head->raise_to_top ();
		} else {
			u.flag_head->hide ();
		}
	}

	const double tw = u.label ? u.label->width () : 60;

	// C) Etiqueta elegante abajo del triángulo (sin tapar los números de la regla)
	if (u.label_bg) {
		u.label_bg->set (ArdourCanvas::Rect (x + 4, 12, x + 4 + tw + 10, 28));
		if (u.visible) {
			u.label_bg->show ();
			u.label_bg->raise_to_top ();
		} else {
			u.label_bg->hide ();
		}
	}

	if (u.label) {
		u.label->set_position (ArdourCanvas::Duple (x + 9, 14));
		if (u.visible) {
			u.label->show ();
			u.label->raise_to_top ();
		} else {
			u.label->hide ();
		}
	}
}

void
NovaColabPresence::upsert_user (const std::string& id,
                                const std::string& name,
                                const std::string& color_hex,
                                samplepos_t position_samples)
{
	auto it = _users.find (id);
	if (it == _users.end ()) {
		NovaRemoteUser u;
		u.id = id;
		u.name = name;
		u.color_hex = color_hex.empty () ? "#00F0FF" : color_hex;
		u.position = position_samples;
		u.visible = true;
		u.line = nullptr;
		u.flag_head = nullptr;
		u.label = nullptr;
		u.label_bg = nullptr;
		build_items (u);
		update_item_geometry (u);
		_users[id] = u;
	} else {
		it->second.name = name;
		if (!color_hex.empty ()) {
			it->second.color_hex = color_hex;
		}
		it->second.position = position_samples;
		it->second.visible = true;
		if (it->second.label) {
			it->second.label->set (name);
		}
		update_item_geometry (it->second);
	}
}

void
NovaColabPresence::set_position (const std::string& id, samplepos_t position_samples)
{
	auto it = _users.find (id);
	if (it == _users.end ()) {
		return;
	}
	it->second.position = position_samples;
	update_item_geometry (it->second);
}

void
NovaColabPresence::remove_user (const std::string& id)
{
	auto it = _users.find (id);
	if (it == _users.end ()) {
		return;
	}
	destroy_items (it->second);
	_users.erase (it);
}

void
NovaColabPresence::clear ()
{
	for (auto& kv : _users) {
		destroy_items (kv.second);
	}
	_users.clear ();
}

void
NovaColabPresence::reposition_all ()
{
	for (auto& kv : _users) {
		update_item_geometry (kv.second);
	}
}

void
NovaColabPresence::start_demo_user_b ()
{
	stop_demo ();
	_demo_pos = 0;
	upsert_user ("user_b", "User_B (Synced Playback)", "#00F0FF", _demo_pos);

	_demo_conn = Glib::signal_timeout ().connect (
		sigc::mem_fun (*this, &NovaColabPresence::demo_tick), 50);
}

void
NovaColabPresence::stop_demo ()
{
	if (_demo_conn.connected ()) {
		_demo_conn.disconnect ();
	}
}

bool
NovaColabPresence::demo_tick ()
{
	if (!_editor) {
		return true;
	}
	_demo_pos += 4800;
	if (_demo_pos > 48000LL * 60) {
		_demo_pos = 0;
	}
	set_position ("user_b", _demo_pos);
	return true;
}