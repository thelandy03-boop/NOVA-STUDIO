#ifndef __nova_ui_kit_h__
#define __nova_ui_kit_h__

#include <ytkmm/widget.h>
#include <ytkmm/box.h>
#include <ytkmm/label.h>
#include <ytkmm/entry.h>
#include <ytkmm/eventbox.h>
#include <sigc++/sigc++.h>
#include <string>

/* ------------------------------------------------------------------- */
/*  CSS-LIKE STYLING STRUCT (Propiedades de Estilo Tipo Web)          */
/* ------------------------------------------------------------------- */
struct NovaStyle {
	std::string bg_color;         /* e.g. "#1E1E24" */
	std::string bg_gradient_end;  /* Opcional: para gradientes */
	std::string border_color;     /* e.g. "#00F0FF" */
	std::string text_color;       /* e.g. "#FFFFFF" */
	std::string hover_bg;         /* e.g. "#2A2A35" */
	
	double border_radius;         /* e.g. 8.0 (Corners) */
	double border_width;          /* e.g. 1.0 */
	
	int font_size;                /* e.g. 12 */
	bool font_bold;
	
	int padding_x;
	int padding_y;
	int width;
	int height;

	bool has_shadow;
	bool has_glow;

	NovaStyle();

	/* Preset de CSS rápidos */
	static NovaStyle button_primary();
	static NovaStyle button_secondary();
	static NovaStyle button_danger();
	static NovaStyle input_box();
	static NovaStyle card();
	static NovaStyle status_pill();
};

/* ------------------------------------------------------------------- */
/*  NOVA UI KIT FACTORY (Generador de Componentes)                    */
/* ------------------------------------------------------------------- */
class NovaUIKit {
public:
	/* Crear Botón con estilo CSS + evento OnClick */
	static Gtk::Widget* create_button (
		const std::string& text,
		const NovaStyle& style,
		sigc::slot<void> on_click
	);

	/* Crear Campo de Texto (Input) con corners y fondo CSS */
	static Gtk::Widget* create_input (
		Gtk::Entry& entry_widget,
		const NovaStyle& style
	);

	/* Crear Contenedor/Tarjeta (Div con fondo, borde y corners) */
	static Gtk::EventBox* create_card (
		Gtk::Widget& child_content,
		const NovaStyle& style
	);

	/* Crear Cápsula/Pill de estado */
	static Gtk::Widget* create_pill (
		Gtk::Label& label_widget,
		const NovaStyle& style
	);
};

#endif /* __nova_ui_kit_h__ */