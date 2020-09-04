// xmms2-ynca - An XMMS2 client to control Yamaha AV receivers
// Copyright (C) 2018-2020  James Le Cuirot <chewi@gentoo.org>

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#include <boost/asio.hpp>
#include <boost/optional.hpp>
#include <boost/program_options.hpp>
#include <chrono>
#include <ctime>
#include <fstream>
#include <glib.h>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <xmmsclient/xmmsclient++-glib.h>
#include <xmmsclient/xmmsclient++.h>

using boost::asio::ip::tcp;
using boost::optional;
namespace po = boost::program_options;

static GMainLoop *ml;

class Xmms2YncaHandler {
public:
	Xmms2YncaHandler(const std::string &host, const int port, const std::string &input, const optional<std::string> &default_program);
	~Xmms2YncaHandler() = default;

	void setDisconnectCallback(const Xmms::DisconnectCallback::value_type &callback);

private:
	bool event_handler(const int id);
	bool error_handler(const std::string &error);

	void send_commands(const std::vector<const std::string*> &commands);
	bool send_commands_with_program(const Xmms::PropDict &info);

	inline static const std::string on_command = "@MAIN:PWR=On";
	const std::string input_command;
	std::vector<const std::string*> input_commands;

	const optional<std::string> default_program;
	boost::asio::io_service io_service;
	tcp::socket socket;
	tcp::resolver resolver;
	tcp::resolver::query resolver_query;
	Xmms::Client client;
	std::time_t last;
};

Xmms2YncaHandler::Xmms2YncaHandler(const std::string &host, const int port, const std::string &input, const optional<std::string> &default_program) :
	input_command("@MAIN:INP=" + input),
	default_program(default_program),
	io_service(),
	socket(io_service),
	resolver(io_service),
	resolver_query(host, std::to_string(port)),
	client("xmms2-ynca"),
	last(0)
{
	input_commands.push_back(&on_command);
	input_commands.push_back(&input_command);

	client.connect(std::getenv("XMMS_PATH"));
	client.playback.broadcastCurrentID()(Xmms::bind(&Xmms2YncaHandler::event_handler, this), Xmms::bind(&Xmms2YncaHandler::error_handler, this));
	client.setMainloop(new Xmms::GMainloop(client.getConnection()));
}

void Xmms2YncaHandler::setDisconnectCallback(const Xmms::DisconnectCallback::value_type &callback) {
	client.setDisconnectCallback(callback);
}

bool Xmms2YncaHandler::event_handler(const int id) {
	auto now = std::time(nullptr);

	if (now - last < 5) {
		return true;
	}

	last = now;

	if (default_program) {
		try {
			Xmms::PropDictResult res = client.medialib.getInfo(id);
			res.connect(Xmms::bind(&Xmms2YncaHandler::send_commands_with_program, this));
			res.connectError(Xmms::bind(&Xmms2YncaHandler::error_handler, this));
			res();
		} catch (std::exception &e) {
			std::cerr << e.what() << std::endl;
		}
	} else {
		send_commands(input_commands);
	}

	return true;
}

bool Xmms2YncaHandler::error_handler(const std::string &error) {
	std::cerr << "Error: " << error << std::endl;
	return true;
}

void Xmms2YncaHandler::send_commands(const std::vector<const std::string*> &commands) {
	try {
		tcp::resolver::iterator endpoints = resolver.resolve(resolver_query);
		boost::asio::connect(socket, endpoints);

		for (std::vector<const std::string*>::const_iterator command = commands.begin(); command != commands.end(); ++command) {
			socket.write_some(boost::asio::buffer(**command + "\r\n"));
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	} catch (std::exception &e) {
		std::cerr << e.what() << std::endl;
	}

	try { socket.close(); }
	catch(...) {}
}

bool Xmms2YncaHandler::send_commands_with_program(const Xmms::PropDict &info) {
	std::string command;

	try {
		std::string program = boost::get<std::string>(info["ynca_program"]);
		command = "@MAIN:SOUNDPRG=" + program;
	} catch (Xmms::no_such_key_error &err) {
		try {
			int channels = boost::get<int>(info["channels"]);

			if (channels > 2) {
				command = "@MAIN:STRAIGHT=On";
			} else {
				command = "@MAIN:SOUNDPRG=" + *default_program;
			}
		} catch (Xmms::no_such_key_error &err) {
			command = "@MAIN:SOUNDPRG=" + *default_program;
		}
	}

	std::vector<const std::string*> commands(input_commands);
	commands.push_back(&command);
	send_commands(commands);

	return false;
}

void onDisconnect() {
	g_main_loop_quit(ml);
}

int main() {
	po::variables_map vm;
	po::options_description config("Configuration");
	optional<std::string> default_program;

	config.add_options()
		("host", po::value<std::string>(), "Yamaha AV hostname")
		("port", po::value<int>()->default_value(50000), "Yamaha AV port")
		("input", po::value<std::string>(), "Yamaha AV input for XMMS2")
		("default-program", po::value(&default_program), "Yamaha AV default sound program name")
		;

	std::ifstream ifs(Xmms::getUserConfDir() + "/clients/ynca.conf");

	if (!ifs) {
		std::cerr << "Could not open ynca.conf." << std::endl;
		return EXIT_FAILURE;
	}

	store(parse_config_file(ifs, config), vm);
	notify(vm);

	if (!vm.count("host")) {
		std::cerr << "host not set in ynca.conf." << std::endl;
		return EXIT_FAILURE;
	}

	if (!vm.count("input")) {
		std::cerr << "input not set in ynca.conf." << std::endl;
		return EXIT_FAILURE;
	}

	try {
		Xmms2YncaHandler handler(
			vm["host"].as<std::string>(),
			vm["port"].as<int>(),
			vm["input"].as<std::string>(),
			default_program
		);

		ml = g_main_loop_new(NULL, FALSE);
		handler.setDisconnectCallback(onDisconnect);
		g_main_loop_run(ml);

		return EXIT_SUCCESS;
	} catch(Xmms::connection_error &err) {
		std::cerr << "Could not connect: " << err.what() << std::endl;
		return EXIT_FAILURE;
	}
}
