#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <thread>

// Forward-decl to avoid pulling httplib into the public header in full.
namespace httplib { class Server; }

namespace reaforge {
namespace host {

class HttpServer {
public:
    HttpServer();
    ~HttpServer();

    HttpServer(const HttpServer&) = delete;
    HttpServer& operator=(const HttpServer&) = delete;

    // Start the server on 0.0.0.0:port (port from env REAFORGE_PORT, default 7800).
    // Returns true if the server bound and started.
    bool start(int port = -1);

    // Bind to any free port on 127.0.0.1 and start. Returns the bound port
    // on success, or -1 on failure. Useful for tests that need an ephemeral port.
    int start_any();

    // Stop the server (blocks until the worker thread joins).
    void stop();

    bool is_running() const { return running_.load(); }
    int bound_port() const { return port_; }

    // Build the shared error envelope: {"error":code, "message":..., ...extras}.
    static std::string error_envelope(const std::string& code,
                                      const std::string& message,
                                      const std::string& extra_json = "");

private:
    void register_routes();

    std::unique_ptr<httplib::Server> server_;
    std::thread thread_;
    std::atomic<bool> running_{false};
    int port_ = 7800;
};

}
}
