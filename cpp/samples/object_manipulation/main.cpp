#include <array>
#include <ggl/buffer.hpp>
#include <ggl/map.hpp>
#include <ggl/object.hpp>
#include <ggl/schema.hpp>
#include <ggl/types.hpp>
#include <ios>
#include <iostream>
#include <optional>
#include <string_view>
#include <system_error>
#include <variant>

template <class... Ts> struct Overloads : Ts... {
    using Ts::operator()...;
};

int main() {
    std::array list { ggl::Object { "15" },
                      ggl::Object { 24 },
                      ggl::Object { 4.0 } };
    std::array pairs {
        ggl::KV { "key", ggl::Object { false } },
        ggl::KV { "another key", ggl::Object { "Value" } },
        ggl::KV { "key3", ggl::Object { "Anything" } },
        ggl::KV { "key4", ggl::Object { 25 } },
        ggl::KV { "key5",
                  ggl::Object { ggl::List { list.data(), list.size() } } }
    };
    ggl::Map map { pairs.data(), pairs.size() };
    ggl::Buffer buffer { "thing" };
    std::array items { ggl::Object { buffer },
                       ggl::Object { map },
                       ggl::Object { 10.0F } };

    Overloads print_object {
        [](std::monostate) { std::cout << "(null)\n"; },
        [](bool val) { std::cout << std::boolalpha << val << '\n'; },
        [](int64_t val) { std::cout << val << '\n'; },
        [](double val) { std::cout << val << '\n'; },
        [](std::span<uint8_t> value) {
            std::cout.write(
                reinterpret_cast<char *>(value.data()),
                static_cast<std::streamsize>(value.size())
            ) << '\n';
        },
        [](const ggl::List &list) { std::cout << list.size() << " items\n"; },
        [](const ggl::Map &pair_list) {
            std::cout << get<bool>(*pair_list["key"]) << '\n';
            std::cout << get<std::string_view>(*pair_list["another key"])
                      << '\n';
        },
    };

    for (auto &&object : items) {
        std::visit(print_object, object.to_variant());
    }

    std::optional<bool> x;

    ggl::Object required;

    int64_t y;

    ggl::Object object;
    ggl::Object *mutable_object;

    std::optional<ggl::Object> optional_object;
    ggl::Object *mutable_optional_object;

    std::error_code error = ggl::validate_map(
        map,
        ggl::MapSchema { "key", x },
        ggl::MapSchemaMissingEntry { "key2" },
        ggl::MapSchema { "key3", required },
        ggl::MapSchema { "key4", y },
        ggl::MapSchema { "key5", object, mutable_object },
        ggl::MapSchema {
            "optional_obj", optional_object, mutable_optional_object }
    );

    if (!error) {
        std::cout << "key: " << x.value_or(false) << '\n';
        std::cout << "key2: (missing)\n";
        std::cout << "key3: ";
        std::visit(print_object, required.to_variant());
        std::cout << '\n';

        std::cout << "key4: " << y << '\n';
        std::cout << "key5: ";
        std::visit(print_object, object.to_variant());
        std::cout << '\n';

        std::cout << "optional_obj:";
        std::visit(
            print_object, optional_object.value_or(ggl::Object {}).to_variant()
        );
        std::cout << '\n';
    } else {
        std::cerr << "Failed to validate: " << error.category().name() << ":"
                  << error.message() << "(" << error.value() << ")\n";
    }
}
