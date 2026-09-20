#include "nova_colab_network.h"
#include "nova_colab_presence.h"
#include "nova_toast.h"
#include "nova_colab_dialog.h"
#include "ardour_ui.h"
#include "ardour/session.h"
#include "pbd/i18n.h"

#include <glibmm/main.h>
#include <sstream>
#include <iostream>

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #pragma comment(lib, "ws2_32.lib")
  typedef int socklen_t;
#else
  #include <sys/socket.h>
  #include <arpa/inet.h>
  #include <netinet/in.h>
  #include <unistd.h>
  #define closesocket close
#endif

NovaColabNetwork* NovaColabNetwork::_instance = nullptr;

NovaColabNetwork&
NovaColabNetwork::instance ()
{
	if (!_instance) {
		_instance = new NovaColabNetwork ();
	}
	return *_instance;
}

NovaColabNetwork::NovaColabNetwork ()
	: _connected (false)
	, _is_host (false)
	, _running (false)
	, _my_user_id ("user_" + std::to_string(rand() % 9000 + 1000))
	, _my_user_name ("Collaborator")
	, _socket_fd (-1)
	, _server_fd (-1)
{
#ifdef _WIN32
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
}

NovaColabNetwork::~NovaColabNetwork ()
{
	disconnect ();
#ifdef _WIN32
	WSACleanup();
#endif
}

void
NovaColabNetwork::start_sync_timer ()
{
	stop_sync_timer ();
	_sync_timer = Glib::signal_timeout().connect(
		sigc::mem_fun(*this, &NovaColabNetwork::sync_tick), 50);
}

void
NovaColabNetwork::stop_sync_timer ()
{
	if (_sync_timer.connected()) {
		_sync_timer.disconnect();
	}
}

bool
NovaColabNetwork::sync_tick ()
{
	if (!_connected) return false;

	ARDOUR::Session* s = ARDOUR_UI::instance() ? ARDOUR_UI::instance()->the_session() : nullptr;
	if (s) {
		samplepos_t pos = s->transport_sample();
		send_my_position(pos);
	}
	return true;
}

bool
NovaColabNetwork::start_server (int port)
{
	disconnect ();

	_is_host = true;
	_running = true;
	_my_user_id = "host_master";
	_my_user_name = "Host (User-A)";

	_server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (_server_fd < 0) return false;

	int opt = 1;
	setsockopt(_server_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

	struct sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(port);

	if (bind(_server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
		closesocket(_server_fd);
		_server_fd = -1;
		return false;
	}

	if (listen(_server_fd, 5) < 0) {
		closesocket(_server_fd);
		_server_fd = -1;
		return false;
	}

	_connected = true;
	start_sync_timer ();
	_net_thread = std::thread(&NovaColabNetwork::server_loop, this, port);

	NovaToast::show_success(_("Servidor Host iniciado en puerto 32550"));
	return true;
}

bool
NovaColabNetwork::connect_to_host (const std::string& host_ip, int port)
{
	disconnect ();

	_is_host = false;
	_running = true;
	_my_user_id = "colab_" + std::to_string(rand() % 800 + 100);
	_my_user_name = "User_B (Synced Playback)";

	std::string ip = host_ip;
	if (ip.find("http://") == 0) ip = ip.substr(7);
	if (ip.find("https://") == 0) ip = ip.substr(8);
	if (ip.find("nova.io/") != std::string::npos) ip = "127.0.0.1";
	if (ip.empty() || ip == "localhost") ip = "127.0.0.1";

	start_sync_timer ();
	_net_thread = std::thread(&NovaColabNetwork::client_loop, this, ip, port);
	return true;
}

void
NovaColabNetwork::disconnect ()
{
	stop_sync_timer ();
	_running = false;
	_connected = false;

	if (_socket_fd >= 0) {
		closesocket(_socket_fd);
		_socket_fd = -1;
	}
	if (_server_fd >= 0) {
		closesocket(_server_fd);
		_server_fd = -1;
	}

	{
		std::lock_guard<std::mutex> lock(_net_mutex);
		for (int s : _client_sockets) {
			closesocket(s);
		}
		_client_sockets.clear();
	}

	if (_net_thread.joinable()) {
		_net_thread.join();
	}
}

void
NovaColabNetwork::server_loop (int port)
{
	while (_running) {
		struct sockaddr_in client_addr;
		socklen_t addrlen = sizeof(client_addr);
		int new_socket = accept(_server_fd, (struct sockaddr*)&client_addr, &addrlen);

		if (new_socket >= 0) {
			{
				std::lock_guard<std::mutex> lock(_net_mutex);
				_client_sockets.push_back(new_socket);
			}

			// Enviar bienvenida al cliente recién conectado con info del Host
			std::string welcome = "JOIN|" + _my_user_id + "|" + _my_user_name + "\n";
			send_raw(new_socket, welcome);

			std::thread([this, new_socket]() {
				char buffer[1024];
				while (_running) {
					int valread = recv(new_socket, buffer, sizeof(buffer) - 1, 0);
					if (valread <= 0) break;
					buffer[valread] = '\0';
					handle_incoming_data(std::string(buffer), new_socket);
				}
				closesocket(new_socket);
			}).detach();
		}
	}
}

void
NovaColabNetwork::client_loop (std::string ip, int port)
{
	_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (_socket_fd < 0) return;

	struct sockaddr_in serv_addr;
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	inet_pton(AF_INET, ip.c_str(), &serv_addr.sin_addr);

	if (connect(_socket_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
		closesocket(_socket_fd);
		_socket_fd = -1;
		_connected = false;
		return;
	}

	_connected = true;

	// Saludar al Host
	std::string join_msg = "JOIN|" + _my_user_id + "|" + _my_user_name + "\n";
	send_raw(_socket_fd, join_msg);

	char buffer[1024];
	while (_running) {
		int valread = recv(_socket_fd, buffer, sizeof(buffer) - 1, 0);
		if (valread <= 0) break;
		buffer[valread] = '\0';
		handle_incoming_data(std::string(buffer), _socket_fd);
	}

	_connected = false;
}

void
NovaColabNetwork::send_raw (int socket_fd, const std::string& raw_str)
{
	if (socket_fd >= 0) {
		send(socket_fd, raw_str.c_str(), raw_str.length(), 0);
	}
}

void
NovaColabNetwork::broadcast_raw (const std::string& raw_str, int exclude_fd)
{
	std::lock_guard<std::mutex> lock(_net_mutex);
	for (int s : _client_sockets) {
		if (s != exclude_fd) {
			send_raw(s, raw_str);
		}
	}
}

void
NovaColabNetwork::send_my_position (samplepos_t pos)
{
	std::string msg = "POS|" + _my_user_id + "|" + std::to_string(pos) + "\n";
	if (_is_host) {
		broadcast_raw(msg);
	} else if (_socket_fd >= 0) {
		send_raw(_socket_fd, msg);
	}
}

void
NovaColabNetwork::send_transport_state (bool is_playing, samplepos_t pos)
{
	std::string msg = "TRP|" + _my_user_id + "|" + (is_playing ? "1" : "0") + "|" + std::to_string(pos) + "\n";
	if (_is_host) {
		broadcast_raw(msg);
	} else if (_socket_fd >= 0) {
		send_raw(_socket_fd, msg);
	}
}

void
NovaColabNetwork::handle_incoming_data (const std::string& data, int source_fd)
{
	std::stringstream ss(data);
	std::string line;
	while (std::getline(ss, line)) {
		if (line.empty()) continue;

		std::stringstream line_ss(line);
		std::string cmd, uid, param1;
		std::getline(line_ss, cmd, '|');
		std::getline(line_ss, uid, '|');

		// Ignorar mis propios paquetes devueltos
		if (uid == _my_user_id) continue;

		if (cmd == "POS") {
			std::getline(line_ss, param1, '|');
			if (!param1.empty()) {
				samplepos_t pos = std::stoll(param1);

				NovaNetPacket* pkt = new NovaNetPacket();
				pkt->type = NET_MSG_POSITION;
				pkt->user_id = uid;
				pkt->position = pos;
				Glib::signal_idle().connect(sigc::bind(sigc::ptr_fun(&NovaColabNetwork::on_idle_process_packet), pkt));

				if (_is_host) broadcast_raw(line + "\n", source_fd);
			}
		} else if (cmd == "JOIN") {
			std::getline(line_ss, param1, '|');
			NovaNetPacket* pkt = new NovaNetPacket();
			pkt->type = NET_MSG_JOIN;
			pkt->user_id = uid;
			pkt->user_name = param1;
			Glib::signal_idle().connect(sigc::bind(sigc::ptr_fun(&NovaColabNetwork::on_idle_process_packet), pkt));

			if (_is_host) broadcast_raw(line + "\n", source_fd);
		}
	}
}

bool
NovaColabNetwork::on_idle_process_packet (NovaNetPacket* pkt)
{
	if (!pkt) return false;

	if (pkt->type == NET_MSG_POSITION) {
		/* Actualizar la línea de tiempo REAL del usuario remoto */
		NovaColabPresence::instance().upsert_user(pkt->user_id, "User_B (Synced Playback)", "#00F0FF", pkt->position);
		NovaColabPresence::instance().set_position(pkt->user_id, pkt->position);
	} else if (pkt->type == NET_MSG_JOIN) {
		NovaToast::show_info(pkt->user_name + _(" conectado en tiempo real"));
		NovaColabPresence::instance().upsert_user(pkt->user_id, pkt->user_name, "#00F0FF", 0);
		if (NovaColabDialog::instance()) {
			NovaColabDialog::instance()->add_user(pkt->user_name, "Collaborator", "#00F0FF", false);
		}
	}

	delete pkt;
	return false;
}