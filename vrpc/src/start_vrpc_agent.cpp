
// [[Rcpp::depends(BH)]]

#include <iomanip>
#include <iostream>
#include <map>
#include <bitset>

#include <Rcpp.h>
#include <json.hpp>
#include <mqtt_client_cpp.hpp>
#include <mqtt/setup_log.hpp>

using namespace std::chrono_literals;

// Variables
struct Options {
  bool is_ssl;
  std::string host;
  std::string port;
  std::string domain;
  std::string agent;
  std::string username;
  std::string password;
  std::string token;
  std::string version;
  std::vector<std::string> functions;
};

std::function<void()> shutdown_handler;

// Functions
std::vector<std::string> tokenize(const std::string& input,
                                  char const* delimiters) {
  std::vector<std::string> output;
  std::bitset<255> delims;
  while (*delimiters) {
    unsigned char code = *delimiters++;
    delims[code] = true;
  }
  std::string::const_iterator beg;
  bool in_token = false;
  for (std::string::const_iterator it = input.begin(), end = input.end();
       it != end; ++it) {
    if (delims[*it & 0xff]) {
      if (in_token) {
        output.push_back(std::string(beg, it));
        in_token = false;
      }
    } else if (!in_token) {
      beg = it;
      in_token = true;
    }
  }
  if (in_token) {
    output.push_back(std::string(beg, input.end()));
  }
  return output;
}

std::string get_username() {
#ifdef __linux__
  char username[LOGIN_NAME_MAX];
  getlogin_r(username, LOGIN_NAME_MAX);
#else
  std::string username('unkown')
#endif
  return std::string(username);
}

std::string get_current_path() {
#ifdef __linux__
  char result[PATH_MAX];
  ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
  return std::string(result, (count > 0) ? count : 0);
#else
  return detail::get_hostname()
#endif
}

std::string get_current_path_id() {
  const std::string path(get_current_path());
  return std::to_string(std::hash<std::string>{}(path)).substr(0, 4);
}

std::string get_hostname() {
  char hostname[HOST_NAME_MAX];
  gethostname(hostname, HOST_NAME_MAX);
  return std::string(hostname);
}

std::string get_platform() {
#ifdef _WIN32
  return "win32";
#elif _WIN64
  return "win64";
#elif __APPLE__ || __MACH__
  return "darwin";
#elif __linux__
  return "linux";
#elif __FreeBSD__
  return "freebsd";
#elif __unix || __unix__
  return "unix";
#else
  return "other";
#endif
}

template <class T>
void publish_agent_info(const T& client, const Options& options) {
  vrpc::json j;
  j["status"] = "online";
  j["hostname"] = get_hostname();
  j["version"] = options.version;
  const std::string topic(options.domain + "/" + options.agent +
                          "/__agentInfo__");
  client->publish(topic, j.dump(),
                  MQTT_NS::qos::at_least_once | MQTT_NS::retain::yes);
}

template <class T>
void publish_class_info(const T& client, const Options& options) {
  vrpc::json j;
  j["className"] = "Session";
  j["instances"] = std::vector<std::string>{"current"};
  j["memberFunctions"] = options.functions;
  j["staticFunctions"] = std::vector<std::string>{};
  j["meta"] = vrpc::json{};
  const std::string topic(options.domain + "/" + options.agent +
                          "/Session/__classInfo__");
  client->publish(topic, j.dump(),
                  MQTT_NS::qos::at_least_once | MQTT_NS::retain::yes);
}

std::string generate_client_id(const Options& options) {
  const std::string tmp = options.domain + options.agent;
  std::to_string(std::hash<std::string>{}(tmp)).substr(0, 18);
  return "vrpca" + std::to_string(std::hash<std::string>{}(tmp)).substr(0, 18);
}

std::string generate_agent_name() {
  const std::string username(get_username());
  // TODO once c++17 is standard we will use filesystem::current_path() here
  const std::string pathId(get_current_path_id());
  const std::string hostname(get_hostname());
  const std::string platform(get_platform());
  return username + "-" + pathId + "@" + hostname + "-" + platform + "-r";
}

void extract_broker_info(Options& options, const std::string& url) {
  const size_t pos1 = url.find_first_of("://");
  if (pos1 == std::string::npos) {
    throw std::runtime_error(
        "Missing scheme in broker url (use e.g. mqtts://<hostname>)");
  }
  options.is_ssl =
      url.substr(0, pos1) == "ssl" || url.substr(0, pos1) == "mqtts";
  options.host = url.substr(pos1 + 3);
  const size_t pos2 = options.host.find_first_of(":");
  if (pos2 == std::string::npos) {
    options.port = options.is_ssl ? "8883" : "1883";
  } else {
    options.port = options.host.substr(pos2 + 1);
    options.host = options.host.substr(0, pos2);
  }
}

Options parse_arguments(const Rcpp::List& args) {
  Options options;
  extract_broker_info(options, Rcpp::as<std::string>(args["broker"]));
  options.domain = Rcpp::as<std::string>(args["domain"]);
  options.agent = args["agent"] == R_NilValue
                      ? generate_agent_name()
                      : Rcpp::as<std::string>(args["agent"]);
  options.functions = Rcpp::as<std::vector<std::string>>(args["functions"]);
  return options;
}

MQTT_NS::will create_will_message(const Options& options) {
  return {MQTT_NS::allocate_buffer(options.domain + "/" + options.agent +
                                   "/__agentInfo__"),
          MQTT_NS::allocate_buffer(vrpc::json{
              {"status", "offline"},
              {"hostname",
               get_hostname()}}.dump()),

          MQTT_NS::qos::at_least_once | MQTT_NS::retain::yes};
}

// [[Rcpp::export]]
void start_vrpc_agent(const Rcpp::List& args) {
  const Options options = parse_arguments(args);

  std::cout << "Domain : " << options.domain << std::endl;
  std::cout << "Agent  : " << options.agent << std::endl;
  std::cout << "Broker : " << options.host << ":" << options.port << std::endl;
  std::cout << "Functions: " << options.functions[0] << std::endl;
  std::cout << "Client ID: " << generate_client_id(options) << std::endl;

  MQTT_NS::setup_log();
  boost::asio::io_context ioc;

  // Create no TLS client
  auto c = MQTT_NS::make_sync_client(ioc, options.host, options.port);
  using packet_id_t =
      typename std::remove_reference_t<decltype(*c)>::packet_id_t;

  // Setup client
  c->set_client_id(generate_client_id(options));
  c->set_clean_session(true);
  c->set_will(MQTT_NS::will(MQTT_NS::allocate_buffer(options.domain + "/" + options.agent +
                                        "/__agentInfo__"),
               MQTT_NS::allocate_buffer(vrpc::json{{"status", "offline"},
                                                   {"hostname", get_hostname()}}
                                            .dump()),
               MQTT_NS::qos::at_least_once | MQTT_NS::retain::yes));

  // Setup handlers
  c->set_connack_handler(
      [&](bool sp, MQTT_NS::connect_return_code connack_return_code) {
        if (connack_return_code == MQTT_NS::connect_return_code::accepted) {
          std::cout << "[OK]" << std::endl;
          publish_agent_info(c, options);
          c->subscribe(options.domain + "/" + options.agent + "/Session/__static__/__create__", MQTT_NS::qos::at_least_once);
          c->subscribe(options.domain + "/" + options.agent + "/Session/current/+", MQTT_NS::qos::at_least_once);
          publish_class_info(c, options);
        }
        return true;
      });
  c->set_close_handler([]() { std::cout << "closed." << std::endl; });
  c->set_error_handler([](MQTT_NS::error_code ec) {
    std::cout << "error: " << ec.message() << std::endl;
  });
  c->set_publish_handler([&](MQTT_NS::optional<packet_id_t> packet_id,
                             MQTT_NS::publish_options pubopts,
                             MQTT_NS::buffer topic, MQTT_NS::buffer contents) {
    std::cout << "message received."
              << " dup: " << pubopts.get_dup() << " qos: " << pubopts.get_qos()
              << " retain: " << pubopts.get_retain() << std::endl;
    std::cout << "topic: " << topic << std::endl;
    std::cout << "contents: " << contents << std::endl;
    const auto tokens = tokenize(std::string(topic), "/");
    if (tokens.size() == 4 && tokens[3] == "__clientInfo__") {
      std::cout << "received clientInfo" << std::endl;
      return true;
    }
    if (tokens.size() != 5) {
      std::cout << "Received message with invalid topic URI" << std::endl;
      return true;
    }
    auto j = vrpc::json::parse(std::string(contents));
    const std::string sender = j.at("sender");
    try {
      const std::string class_name = tokens[2];
      const std::string instance = tokens[3];
      const std::string method = tokens[4];
      j["context"] = instance == "__static__" ? class_name : instance;
      j["method"] = method;
      vrpc::json args;
      // It is unfortunate that VRPC's RPC protocol does not send args as
      // array...
      const vrpc::json& data = j["data"];
      for (const auto& x : data.items()) {
        args.push_back(x.value());
      }
      // std::cout << "data array :" << args.dump() << std::endl;
      auto f = Rcpp::Function("json_call");
      Rcpp::CharacterVector cv = f(method, args.dump());
      const std::string ret = Rcpp::as<std::string>(cv);
      if (ret.size() >= 7 && ret.substr(0, 7) == "__err__") {
        j["data"]["e"] = ret.substr(7);
      } else {
        j["data"]["r"] = vrpc::json::parse(ret);
      }
      c->publish(sender, j.dump(), MQTT_NS::qos::at_least_once);
    } catch (const std::exception& e) {
      j["data"]["e"] = "Error while calling remote function: " + std::string(e.what());
      c->publish(sender, j.dump(), MQTT_NS::qos::at_least_once);
    }
    return true;
  });

  // Connect
  c->connect();

  // Disconnect (Ctrl-C)
  shutdown_handler = [&]() {
    c->publish(
        options.domain + "/" + options.agent + "/__agentInfo__",
        vrpc::json{{"status", "offline"}, {"hostname", get_hostname()}}.dump(),
        MQTT_NS::qos::at_least_once | MQTT_NS::retain::yes);
    c->disconnect(5s);
  };
  std::signal(SIGINT, [](int) { shutdown_handler(); });
  std::signal(SIGKILL, [](int) { shutdown_handler(); });

  // Start event loop
  ioc.run();
}
