#include "bghost.h"
#include "../crow_all.h"
#include <string>
#include <fstream>
#include <sstream>
#include <memory>
#include <thread>
#include <atomic>
#include <csignal>

namespace BgHost {
    
static std::unique_ptr<std::thread> serverThread;
static std::atomic<bool> running(false);
static std::atomic<bool> shouldStop(false);

bool start() {
    if (running.load()) {
        return false; // Already running
    }
    
    shouldStop.store(false);
    serverThread = std::make_unique<std::thread>([]() {
        // Ignore SIGPIPE to prevent crashes when clients disconnect
        std::signal(SIGPIPE, SIG_IGN);
        
        crow::SimpleApp app;
        
        CROW_ROUTE(app, "/")([](){
            std::ifstream file("index.html");
            if (file.is_open()) {
                std::stringstream buffer;
                buffer << file.rdbuf();
                return crow::response(buffer.str());
            }
            return crow::response(404, "File not found");
        });
        
        running.store(true);
        
        // Run the server with a custom server that can be stopped
        auto& app_server = app.port(8304).multithreaded();
        app_server.run();
        
        running.store(false);
    });
    
    // Detach the thread so it doesn't cause issues when main exits
    serverThread->detach();
    return true;
}

void stop() {
    if (running.load()) {
        shouldStop.store(true);
        // Note: Crow doesn't provide a clean way to stop the server from outside
        // This is a limitation that would need to be addressed with a different approach
        running.store(false);
    }
}

bool isRunning() {
    return running.load();
}

}