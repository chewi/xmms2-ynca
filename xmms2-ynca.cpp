#include <boost/asio.hpp>
#include <boost/program_options.hpp>
#include <ctime>
#include <fstream>
#include <glib.h>
#include <iostream>
#include <string>
#include <vector>
#include <xmmsclient/xmmsclient++-glib.h>
#include <xmmsclient/xmmsclient++.h>

using boost::asio::ip::tcp;
namespace po = boost::program_options;

static GMainLoop *ml;

class Xmms2YncaHandler {
public:
	Xmms2YncaHandler(const std::string &host, const int port, const std::string &input);
	~Xmms2YncaHandler() = default;

	void setDisconnectCallback(const Xmms::DisconnectCallback::value_type &callback);

private:
	bool event_handler(const Xmms::Playback::Status status);
	bool error_handler(const std::string& error);

	const std::string command;
	boost::asio::io_service io_service;
	tcp::socket socket;
	Xmms::Client client;
	std::time_t last;

	tcp::resolver::iterator endpoints;
};

Xmms2YncaHandler::Xmms2YncaHandler(const std::string &host, const int port, const std::string &input) :
	command("@MAIN:PWR=On\r\n@MAIN:INP=" + input + "\r\n"),
	io_service(),
	socket(io_service),
	client("xmms2-ynca"),
	last(0)
{
	tcp::resolver resolver(io_service);
	tcp::resolver::query query(host, std::to_string(port));
	endpoints = resolver.resolve(query);

	client.connect(std::getenv("XMMS_PATH"));
	client.playback.broadcastStatus()(Xmms::bind(&Xmms2YncaHandler::event_handler, this), Xmms::bind(&Xmms2YncaHandler::error_handler, this));
	client.setMainloop(new Xmms::GMainloop(client.getConnection()));
}

void Xmms2YncaHandler::setDisconnectCallback(const Xmms::DisconnectCallback::value_type &callback) {
	client.setDisconnectCallback(callback);
}

bool Xmms2YncaHandler::event_handler(const Xmms::Playback::Status status) {
	if (status != XMMS_PLAYBACK_STATUS_PLAY)
		return true;

	auto now = std::time(nullptr);

	if (now - last < 5) {
		return true;
	}

	last = now;

	try {
		boost::asio::connect(socket, endpoints);
		socket.write_some(boost::asio::buffer(command));
	} catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
	}

	try { socket.close(); }
	catch(...) {}

	return true;
}

bool Xmms2YncaHandler::error_handler(const std::string& error) {
	std::cerr << "Error: " << error << std::endl;
	return true;
}

void onDisconnect() {
	g_main_loop_quit(ml);
}

int main() {
	po::variables_map vm;
	po::options_description config("Configuration");

	config.add_options()
		("host", po::value<std::string>(), "Yamaha AV hostname")
		("port", po::value<int>()->default_value(50000), "Yamaha AV port")
		("input", po::value<std::string>(), "Yamaha AV input for XMMS2")
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
		Xmms2YncaHandler handler(vm["host"].as<std::string>(), vm["port"].as<int>(), vm["input"].as<std::string>());

		ml = g_main_loop_new(NULL, FALSE);
		handler.setDisconnectCallback(onDisconnect);
		g_main_loop_run(ml);

		return EXIT_SUCCESS;
	} catch(Xmms::connection_error& err) {
		std::cerr << "Could not connect: " << err.what() << std::endl;
		return EXIT_FAILURE;
	}
}
