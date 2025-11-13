#include <gg/buffer.hpp>
#include <gg/ipc/client.hpp>
#include <gg/object.hpp>
#include <gg/types.hpp>
#include <chrono>
#include <iostream>
#include <string_view>
#include <system_error>
#include <thread>

namespace {
std::ostream &operator<<(std::ostream &os, const gg::Buffer &buffer) {
    os.write(
        reinterpret_cast<const char *>(buffer.data()),
        static_cast<std::streamsize>(buffer.size())
    );
    return os;
}
}

class PubsubHandler : public gg::ipc::LocalTopicCallback {
    void operator()(
        std::string_view topic,
        gg::Object payload,
        gg::ipc::Subscription &handle
    ) override {
        (void) handle;
        std::cout << "Message received on " << topic << "\n";
        if (payload.index() == GG_TYPE_MAP) {
            std::cout << "(gg::Map of unknown schema)\n";
            return;
        }
        std::cout << get<gg::Buffer>(payload) << '\n';
    }
};

int main(int argc, char *argv[]) {
    if (argc < 3) {
        std::cerr << "Too few args\n";
        return 1;
    }
    std::string_view topic = argv[1];
    std::string_view payload = argv[2];

    auto &client = gg::ipc::Client::get();

    auto error = client.connect();

    if (error) {
        std::cerr << "Failed to connect (" << error << ")\n";
        return 1;
    }

    // handlers must be static lifetime if subscription handle is not held
    static PubsubHandler handler;
    error = client.subscribe_to_topic(topic, handler);

    std::cout << "Attempting to publish to local topic: \"" << topic << "\"\n";

    while (true) {
        error = client.publish_to_topic(topic, payload);

        if (error) {
            std::cerr << "Failed to publish to local topic (" << error << ")\n";
            return 1;
        }

        using namespace std::chrono_literals;
        std::this_thread::sleep_for(1s);
    }
}
