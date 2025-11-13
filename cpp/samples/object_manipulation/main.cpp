#include <gg/list.hpp>
#include <gg/map.hpp>
#include <gg/object.hpp>
#include <gg/schema.hpp>
#include <stdint.h>
#include <array>
#include <iostream>
#include <optional>
#include <span>
#include <string> // IWYU pragma: keep (operator<<)
#include <string_view>
#include <system_error>
#include <variant>

template <class... Ts> struct Overloads : Ts... {
    using Ts::operator()...;
};

int main() {
    std::array list { gg::Object { "15" },
                      gg::Object { 24 },
                      gg::Object { 4.0 } };
    std::array pairs { gg::KV { "key", gg::Object { false } },
                       gg::KV { "another key", "Value" },
                       gg::KV { "key3", "Anything" },
                       gg::KV { "key4", 25 },
                       gg::KV { "key5",
                                gg::List { list.data(), list.size() } } };
    gg::Map map { pairs.data(), pairs.size() };
    std::array items { gg::Object { "String value" },
                       gg::Object { map },
                       gg::Object { 10.0F } };

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
        [](const gg::List &list) { std::cout << list.size() << " items\n"; },
        [](const gg::Map &pair_list) {
            std::cout << get<bool>(*pair_list["key"]) << '\n';
            std::cout << get<std::string_view>(*pair_list["another key"])
                      << '\n';
        },
    };

    for (auto &&object : items) {
        std::visit(print_object, object.to_variant());
    }

    std::optional<bool> x;

    gg::Object required;

    int64_t y;

    gg::Object object;
    gg::Object *mutable_object;

    std::optional<gg::Object> optional_object;
    gg::Object *mutable_optional_object;

    std::error_code error = gg::validate_map(
        map,
        gg::MapSchema { "key", x },
        gg::MapSchemaMissingEntry { "key2" },
        gg::MapSchema { "key3", required },
        gg::MapSchema { "key4", y },
        gg::MapSchema { "key5", object, mutable_object },
        gg::MapSchema {
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
            print_object, optional_object.value_or(gg::Object {}).to_variant()
        );
        std::cout << '\n';
    } else {
        std::cerr << "Failed to validate: " << error.category().name() << ":"
                  << error.message() << "(" << error.value() << ")\n";
    }
}
