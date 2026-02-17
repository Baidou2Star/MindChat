#include "ConfigMgr.h"
#include "LlmHttpServer.h"
#include "LlmProviderPool.h"

#include <iostream>

int main() {
    try {
        auto& cfg = ConfigMgr::Inst();

        std::string host = cfg.GetValue("SelfServer", "Host");
        if (host.empty()) {
            host = "0.0.0.0";
        }

        const std::string port_text = cfg.GetValue("SelfServer", "Port");
        const auto port = static_cast<unsigned short>(port_text.empty() ? 8190 : std::stoi(port_text));

        LlmProviderPool pool;
        if (!pool.InitFromConfig()) {
            std::cerr << "[LLMServer] provider pool init failed" << std::endl;
            return 1;
        }

        LlmHttpServer server(host, port, pool);
        server.Run();
    } catch (const std::exception& e) {
        std::cerr << "[LLMServer] fatal error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
