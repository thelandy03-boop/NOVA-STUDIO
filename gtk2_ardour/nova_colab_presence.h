#ifndef __nova_colab_presence_h__
#define __nova_colab_presence_h__

#include <string>
#include <map>
#include <cstdint>
#include <glibmm/main.h>

#include "temporal/types.h"
#include "temporal/timeline.h"
#include "canvas/container.h"
#include "canvas/line.h"
#include "canvas/text.h"
#include "canvas/rectangle.h"
#include "canvas/polygon.h"

using Temporal::samplepos_t;

class Editor;

struct NovaRemoteUser {
	std::string id;
	std::string name;
	std::string color_hex;
	samplepos_t position;
	bool visible;

	ArdourCanvas::Line*      line;
	ArdourCanvas::Polygon*   flag_head; /* Triángulo del Banderín superior */
	ArdourCanvas::Rectangle* label_bg;
	ArdourCanvas::Text*      label;
};

class NovaColabPresence
{
public:
	static NovaColabPresence& instance ();

	void attach_to_editor (Editor* ed);

	void upsert_user (const std::string& id,
	                  const std::string& name,
	                  const std::string& color_hex,
	                  samplepos_t position_samples);

	void set_position (const std::string& id, samplepos_t position_samples);
	void remove_user (const std::string& id);
	void clear ();

	void start_demo_user_b ();
	void stop_demo ();
	void reposition_all ();

private:
	NovaColabPresence ();
	~NovaColabPresence ();

	Editor* _editor;
	ArdourCanvas::Container* _group;

	std::map<std::string, NovaRemoteUser> _users;

	sigc::connection _demo_conn;
	samplepos_t _demo_pos;

	void ensure_group ();
	void build_items (NovaRemoteUser& u);
	void update_item_geometry (NovaRemoteUser& u);
	void destroy_items (NovaRemoteUser& u);

	double sample_to_canvas_x (samplepos_t s) const;
	bool demo_tick ();

	static NovaColabPresence* _instance;
};

#endif /* __nova_colab_presence_h__ */