
// [[Rcpp::depends(BH)]]

#include <iomanip>
#include <iostream>
#include <map>

#include <Rcpp.h>
#include <json.hpp>
#include <mqtt_client_cpp.hpp>
#include <mqtt/setup_log.hpp>

// Types
typedef std::vector<
    std::tuple<MQTT_NS::string_view, MQTT_NS::subscribe_options>>
    Topics;

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
Topics generate_topics(const Options& options) {
  Topics topics;
  const std::string base_topic = options.domain + "/" + options.agent;
  auto create_topic = std::make_shared<std::string>(base_topic + "/Session/__static__/__create__");
  // In the following we will completely fake a Session class...
  topics.push_back({*create_topic,
                    MQTT_NS::qos::at_least_once});
  // and fake an instance "current" reflecting the current R session
  auto inst_topic = std::make_shared<std::string>(base_topic + "/Session/current/+");
  topics.push_back({*inst_topic, MQTT_NS::qos::at_least_once});
  return topics;
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
  auto topic = std::make_shared<std::string>(options.domain + "/" + options.agent + "/__agentInfo__");
  auto message = std::make_shared<std::string>(
      vrpc::json{{"status", "offline"}, {"hostname", get_hostname()}}.dump());
  return MQTT_NS::will(MQTT_NS::buffer(MQTT_NS::string_view(topic->data(), topic->size())),
                       MQTT_NS::buffer(MQTT_NS::string_view(*message)),
                       MQTT_NS::qos::at_least_once | MQTT_NS::retain::yes);
}

// [[Rcpp::export]]
void start_vrpc_agent(const Rcpp::List& args) {
  const Options options = parse_arguments(args);

  std::cout << "Domain : " << options.domain << std::endl;
  std::cout << "Agent  : " << options.agent << std::endl;
  std::cout << "Broker : " << options.host << ":" << options.port << std::endl;
  std::cout << "Functions: " << options.functions[0] << std::endl;
  std::cout << "Client ID: " << generate_client_id(options) << std::endl;

  const Topics topics = generate_topics(options);

  MQTT_NS::setup_log();
  boost::asio::io_context ioc;

  // Create no TLS client
  auto c = MQTT_NS::make_sync_client(ioc, options.host, options.port);
  using packet_id_t =
      typename std::remove_reference_t<decltype(*c)>::packet_id_t;

  // Setup client
  c->set_client_id(generate_client_id(options));
  c->set_clean_session(true);
  // c->set_will(create_will_message(options));

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
                             MQTT_NS::buffer topic_name,
                             MQTT_NS::buffer contents) {
    std::cout << "publish received."
              << " dup: " << pubopts.get_dup() << " qos: " << pubopts.get_qos()
              << " retain: " << pubopts.get_retain() << std::endl;
    if (packet_id)
      std::cout << "packet_id: " << *packet_id << std::endl;
    std::cout << "topic_name: " << topic_name << std::endl;
    std::cout << "contents: " << contents << std::endl;
    return true;
  });

  // Connect
  c->connect();

  // Disconnect (Ctrl-C)
  shutdown_handler = [&c]() { c->disconnect(); };
  std::signal(SIGINT, [](int) { shutdown_handler(); });
  std::signal(SIGKILL, [](int) { shutdown_handler(); });

  // Start event loop
  ioc.run();
}
