#ifndef __nova_jsfx_window_h__
#define __nova_jsfx_window_h__

#include "ardour_window.h"
#include <ytkmm/box.h>
#include <ytkmm/scale.h>
#include <ytkmm/label.h>
#include <ytkmm/button.h>
#include <ytkmm/frame.h>
#include <vector>
#include <string>
#include "nova_jsfx_parser.h"

class NovaJSFXWindow : public ArdourWindow {
public:
	NovaJSFXWindow(const std::string& title,
	               const std::string& author,
	               const std::vector<JSFXParam>& params);
	~NovaJSFXWindow();

	static void launch_for_jsfx(const std::string& jsfx_code);

private:
	std::string _title;
	std::string _author;
	std::vector<JSFXParam> _params;

	struct SliderWidget {
		int index;
		Gtk::HScale* scale;
		Gtk::Label* val_label;
	};
	std::vector<SliderWidget> _slider_widgets;

	void on_slider_changed(int idx);
};

#endif /* __nova_jsfx_window_h__ */