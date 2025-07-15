#include "toml_serde/trait.h"

#include <iostream>

namespace data
{
    struct Point
    {
        int64_t x;
        int64_t y;
    };

    TOML_DESERIALIZE(Point, {
        TOML_REQUIRE(x);
        TOML_REQUIRE(y);
    });
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <path-to-toml-file>" << std::endl;
        return 1;
    }
    try
    {
        auto point = data::parse_toml_file<data::Point>(argv[1]);
        std::cout << point.x << " " << point.y << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return 0;
}