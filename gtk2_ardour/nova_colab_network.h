#ifndef __nova_colab_network_h__
#define __nova_colab_network_h__

#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <queue>
#include <glibmm/main.h>
#include "temporal/types.h"

using Temporal::samplepos_t;

enum NovaNetMessageType {
	NET_MSG_JOIN = 0,
	NET_MSG_WELCOME,
	NET_MSG_POSITION,
	NET_MSG_TRANSPORT,
	NET_MSG_LEAVE
};

struct NovaNetPacket {
	NovaNetMessageType type;
	std::string user_id;
	std::string user_name;
	std::string payload;
	samplepos_t position;
	bool is_playing;
};

class NovaColabNetwork
{
public:
	static NovaColabNetwork& instance ();

	/* Iniciar Servidor Host en puerto 32550 */
	bool start_server (int port = 32550);

	/* Conectar a Host mediante IP / Link */
	bool connect_to_host (const std::string& host_ip, int port = 32550);

	/* Desconectar red y detener timers */
	void disconnect ();

	/* Enviar posición actual del Playhead */
	void send_my_position (samplepos_t pos);

	/* Enviar cambio de estado de Transporte (PLAY / STOP / SEEK) */
	void send_transport_state (bool is_playing, samplepos_t pos);

	bool is_connected () const { return _connected.load (); }
	bool is_host () const { return _is_host.load (); }
	std::string my_user_id () const { return _my_user_id; }

private:
	NovaColabNetwork ();
	~NovaColabNetwork ();

	static NovaColabNetwork* _instance;

	std::atomic<bool> _connected;
	std::atomic<bool> _is_host;
	std::atomic<bool> _running;

	std::string _my_user_id;
	std::string _my_user_name;

	int _socket_fd;
	int _server_fd;
	std::vector<int> _client_sockets;

	std::thread _net_thread;
	std::mutex  _net_mutex;

	sigc::connection _sync_timer;

	void start_sync_timer ();
	void stop_sync_timer ();
	bool sync_tick ();

	void server_loop (int port);
	void client_loop (std::string ip, int port);

	void handle_incoming_data (const std::string& data, int source_fd);
	void send_raw (int socket_fd, const std::string& raw_str);
	void broadcast_raw (const std::string& raw_str, int exclude_fd = -1);

	static bool on_idle_process_packet (NovaNetPacket* pkt);
};

#endif /* __nova_colab_network_h__ */