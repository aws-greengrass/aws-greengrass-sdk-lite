#include <ggl/buffer.hpp>
#include <ggl/ipc/client.hpp>
#include <ggl/object.hpp>
#include <array>
#include <iostream>
#include <optional>
#include <system_error>
#include <thread>

constexpr int publish_count = 10;

int main() {
    auto &client = ggl::ipc::Client::get();

    auto error = client.connect();

    if (error) {
        std::cerr << "Failed to connect (" << error << ")\n";
        return 1;
    }

    const std::array TOPIC_CONFIG_KEY
        = { ggl::Buffer { "config_ipc" }, ggl::Buffer { "topic" } };

    error = client.update_config(TOPIC_CONFIG_KEY, "/my/topic");

    if (error) {
        std::cerr << "Failed to set config value (" << error << ")\n";
        return 1;
    }

    std::string topic;
    error = client.get_config(TOPIC_CONFIG_KEY, std::nullopt, topic);
    if (error) {
        std::cerr << "Failed to retrieve config value (" << error << ")\n";
        return 1;
    }

    std::cout << "Attempting to publish to local topic: \"" << topic << "\"\n";

    for (int i = 0; i < publish_count; ++i) {
        error = client.publish_to_topic(
            topic, ggl::Buffer { "Hello from sample_config_ipc!" }
        );

        if (error) {
            std::cerr << "Failed to publish to local topic (" << error << ")\n";
            return 1;
        }

        using namespace std::chrono_literals;
        std::this_thread::sleep_for(1s);
    }

    return 0;
}
