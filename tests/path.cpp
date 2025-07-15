#include "toml_serde/trait.h"

#include <iostream>

namespace data
{
    struct Path
    {
        fs::path path;
    };

    TOML_DESERIALIZE(Path, {
        TOML_REQUIRE(path);
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
        auto point = data::parse_toml_file<data::Path>(argv[1]);
        std::cout << point.path << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return 0;
}