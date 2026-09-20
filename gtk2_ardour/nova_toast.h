#ifndef __nova_toast_h__
#define __nova_toast_h__

#include <ytkmm/window.h>
#include <ytkmm/eventbox.h>
#include <ytkmm/frame.h>
#include <ytkmm/label.h>
#include <ytkmm/box.h>
#include <glibmm/main.h>
#include <string>
#include <vector>
#include <queue>

enum NovaToastType {
	TOAST_INFO = 0,
	TOAST_SUCCESS,
	TOAST_WARN,
	TOAST_ERROR
};

class NovaToast : public Gtk::Window
{
public:
	using Gtk::Window::show;

	/** API pública — úsala desde cualquier parte del DAW */
	static void show (const std::string& message,
	                  NovaToastType type = TOAST_INFO,
	                  int duration_ms = 3200);

	static void show_success (const std::string& message, int duration_ms = 3200);
	static void show_info    (const std::string& message, int duration_ms = 3200);
	static void show_warn    (const std::string& message, int duration_ms = 3200);
	static void show_error   (const std::string& message, int duration_ms = 3200);

private:
	NovaToast (const std::string& message, NovaToastType type, int duration_ms);
	~NovaToast ();

	bool on_click (GdkEventButton* ev);
	bool on_timeout_close ();
	void dismiss ();
	static bool destroy_toast (NovaToast* toast);

	void place_inside_ardour (int stack_index);
	void start_life_cycle ();

	static void pump_queue ();
	static int  visible_count ();

	Gtk::EventBox _box;
	Gtk::Label    _lbl;
	NovaToastType _type;
	int           _duration_ms;
	sigc::connection _timeout;
	int           _stack_index;

	struct Pending {
		std::string msg;
		NovaToastType type;
		int duration;
	};
	static std::queue<Pending> _queue;
	static std::vector<NovaToast*> _active;
	static const int MAX_VISIBLE = 4;
	static const int MARGIN_X = 20;
	static const int MARGIN_Y = 40; /* margen sobre el pie de ventana */
	static const int GAP      = 8;
	static const int TOAST_W  = 380; /* ancho suficiente para textos largos */
	static const int TOAST_H  = 42;
};

#endif /* __nova_toast_h__ */