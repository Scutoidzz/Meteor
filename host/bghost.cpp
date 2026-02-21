#include "bghost.h"
#include "../crow_all.h"

int main(string* htmlfile) {
crow::SimpleApp app;

CROW_ROUTE(app, "/")([htmlfile](){
    std::ifstream file(*htmlfile);
    if (file.is_open()) {
        std::stringstream buffer;
        buffer << file.rdbuf();
        return crow::response(buffer.str());
    }
    return crow::response(404, "File not found");
});

app.port(8080).multithreaded().run();

}