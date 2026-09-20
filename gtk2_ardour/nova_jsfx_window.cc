#include <ytk/ytk.h>
#include "nova_jsfx_window.h"
#include "pbd/i18n.h"
#include <cstdio>

NovaJSFXWindow::NovaJSFXWindow(const std::string& title,
                               const std::string& author,
                               const std::vector<JSFXParam>& params)
	: ArdourWindow(title.empty() ? _("JSFX Live Plugin") : title)
	, _title(title)
	, _author(author)
	, _params(params)
{
	set_default_size(480, 120 + params.size() * 55);
	set_position(Gtk::WIN_POS_CENTER);
	set_keep_above(true);
	set_resizable(true);

	Gtk::VBox* main_box = Gtk::manage(new Gtk::VBox(false, 0));
	main_box->set_border_width(12);
	main_box->set_spacing(10);
	add(*main_box);

	// Header: Title & Author
	Gtk::VBox* header_box = Gtk::manage(new Gtk::VBox(false, 2));
	Gtk::Label* lbl_title = Gtk::manage(new Gtk::Label());
	lbl_title->set_markup("<span size='large' weight='bold' foreground='#00F0FF'>" + (title.empty() ? "JSFX Plugin" : title) + "</span>");
	lbl_title->set_alignment(0.0, 0.5);

	Gtk::Label* lbl_author = Gtk::manage(new Gtk::Label());
	lbl_author->set_markup("<span size='small' foreground='#888888'>By " + (author.empty() ? "JSFX Port" : author) + "  |  NOVA-STUDIO Live GUI</span>");
	lbl_author->set_alignment(0.0, 0.5);

	header_box->pack_start(*lbl_title, false, false, 0);
	header_box->pack_start(*lbl_author, false, false, 0);
	main_box->pack_start(*header_box, false, false, 0);

	// Container for Sliders
	Gtk::VBox* sliders_box = Gtk::manage(new Gtk::VBox(false, 8));

	for (size_t i = 0; i < params.size(); ++i) {
		const auto& p = params[i];

		Gtk::Frame* frame = Gtk::manage(new Gtk::Frame());
		frame->set_shadow_type(Gtk::SHADOW_ETCHED_IN);

		Gtk::VBox* row = Gtk::manage(new Gtk::VBox(false, 2));
		row->set_border_width(6);

		Gtk::HBox* label_row = Gtk::manage(new Gtk::HBox(false, 4));
		Gtk::Label* param_lbl = Gtk::manage(new Gtk::Label());
		param_lbl->set_markup("<span weight='bold' foreground='#D4D4D4'>" + p.label + "</span>");
		param_lbl->set_alignment(0.0, 0.5);

		char val_buf[32];
		snprintf(val_buf, sizeof(val_buf), "%.2f", p.def_val);
		Gtk::Label* val_lbl = Gtk::manage(new Gtk::Label());
		val_lbl->set_markup("<span weight='bold' foreground='#51CF66'>" + std::string(val_buf) + "</span>");
		val_lbl->set_alignment(1.0, 0.5);

		label_row->pack_start(*param_lbl, true, true, 0);
		label_row->pack_end(*val_lbl, false, false, 0);

		Gtk::HScale* scale = Gtk::manage(new Gtk::HScale(p.min_val, p.max_val, 0.01));
		scale->set_value(p.def_val);
		scale->set_draw_value(false);

		row->pack_start(*label_row, false, false, 0);
		row->pack_start(*scale, false, false, 2);

		frame->add(*row);
		sliders_box->pack_start(*frame, false, false, 0);

		SliderWidget sw;
		sw.index = p.index;
		sw.scale = scale;
		sw.val_label = val_lbl;
		_slider_widgets.push_back(sw);

		scale->signal_value_changed().connect(
			sigc::bind(sigc::mem_fun(*this, &NovaJSFXWindow::on_slider_changed), i));
	}

	main_box->pack_start(*sliders_box, true, true, 0);
	show_all_children();
}

NovaJSFXWindow::~NovaJSFXWindow() {}

void
NovaJSFXWindow::on_slider_changed(int idx)
{
	if (idx < 0 || idx >= (int)_slider_widgets.size()) return;
	double val = _slider_widgets[idx].scale->get_value();

	char buf[32];
	snprintf(buf, sizeof(buf), "%.2f", val);
	_slider_widgets[idx].val_label->set_markup("<span weight='bold' foreground='#51CF66'>" + std::string(buf) + "</span>");
}

void
NovaJSFXWindow::launch_for_jsfx(const std::string& jsfx_code)
{
	std::string title = NovaJSFXParser::extract_title(jsfx_code);
	std::string author = NovaJSFXParser::extract_author(jsfx_code);
	std::vector<JSFXParam> params = NovaJSFXParser::extract_params(jsfx_code);

	NovaJSFXWindow* win = new NovaJSFXWindow(title, author, params);
	win->show();
	win->present();
}