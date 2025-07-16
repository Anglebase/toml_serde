#pragma once

#include "toml++/toml.hpp"
#include <string>
#include <vector>
#include <map>
#include <optional>
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <unordered_set>
namespace fs = std::filesystem;

namespace data
{
    template <typename T>
    struct Deserializer
    {
        static inline T deserialize(const toml::node &v, const std::string &key = "");
    };

    template <typename T>
    concept Deserializable = requires(const toml::node &v) {
        { Deserializer<T>::deserialize(v, "") } -> std::same_as<T>;
    };

#define TOML_DESERIALIZE(T)                                                            \
    template <>                                                                        \
    struct Deserializer<T>                                                             \
    {                                                                                  \
        static inline T deserialize(const toml::node &v, const std::string &key = "")  \
        {                                                                              \
            if (!v.is<T>())                                                            \
                throw std::runtime_error("toml value for '" + key + "' is not a " #T); \
            return *v.value<T>();                                                      \
        }                                                                              \
    }

    using Boolean = bool;
    using Integer = int64_t;
    using Float = double;
    using String = std::string;
    using Time = toml::time;
    using Date = toml::date;
    using DateTime = toml::date_time;

    TOML_DESERIALIZE(Boolean);
    TOML_DESERIALIZE(Integer);
    TOML_DESERIALIZE(Float);
    TOML_DESERIALIZE(Time);
    TOML_DESERIALIZE(Date);
    TOML_DESERIALIZE(DateTime);

#undef TOML_DESERIALIZE

    template <>
    struct Deserializer<String>
    {
        static inline String deserialize(const toml::node &v, const std::string &key = "")
        {
            if (!v.is_string())
                throw std::runtime_error("toml value for '" + key + "' is not a string");
            return *v.value<std::string>();
        }
    };

    template <>
    struct Deserializer<fs::path>
    {
        using Self = Deserializer<fs::path>;
        static inline fs::path BASE_PATH = fs::current_path();
        static inline fs::path deserialize(const toml::node &v, const std::string &key = "")
        {
            if (!v.is_string())
                throw std::runtime_error("toml value for '" + key + "' is not a string");
            fs::path path = *v.value<std::string>();
            if (path.is_relative())
                path = Self::BASE_PATH / path;
            return path.lexically_normal();
        }
    };

    template <Deserializable E>
    struct Deserializer<std::vector<E>>
    {
        static inline std::vector<E> deserialize(const toml::node &v, const std::string &key = "")
        {
            if (!v.is_array())
                throw std::runtime_error("toml value for '" + key + "' is not an array");
            auto array = *v.as_array();
            std::vector<E> result;
            for (size_t i = 0; i < array.size(); ++i)
                result.push_back(Deserializer<E>::deserialize(array.at(i), key + "[" + std::to_string(i) + "]"));
            return result;
        }
    };

    inline std::string _to_key(const std::string &s)
    {
        if (s.end() == std::find_if(s.begin(), s.end(), [](char c)
                                    { return !std::isalnum(c) && c != '_'; }))
            return s;
        else
            return '"' + s + '"';
    }

    template <Deserializable V>
    struct Deserializer<std::map<std::string, V>>
    {
        static inline std::map<std::string, V> deserialize(const toml::node &v, const std::string &key = "")
        {
            if (!v.is_table())
                throw std::runtime_error("toml value for '" + key + "' is not a table");
            std::map<std::string, V> result;
            for (const auto &[k, v] : *v.as_table())
                result[std::string(k.str())] = Deserializer<V>::deserialize(v, key + "." + _to_key(std::string(k.str())));
            return result;
        }
    };

    template <Deserializable E>
    using Array = std::vector<E>;
    template <Deserializable V>
    using Table = std::map<std::string, V>;
    template <Deserializable T>
    using Optional = std::optional<T>;

    template <Deserializable T>
    void require(const toml::table &table, const std::string &key, T &value, const std::string &key_desc = "")
    {
        const auto prefix = key_desc.empty() ? key : key_desc + "." + _to_key(key);
        value = Deserializer<T>::deserialize(table.at(key), prefix);
    }

    template <Deserializable T>
    void options(const toml::table &table, const std::string &key, Optional<T> &value, const std::string &key_desc = "")
    {
        const auto prefix = key_desc.empty() ? key : key_desc + "." + _to_key(key);
        table.contains(key)
            ? value = Deserializer<T>::deserialize(table.at(key), prefix)
            : value = std::nullopt;
    }

    template <Deserializable T>
    T parse_toml_file(const fs::path &path)
    {
        fs::path &base = Deserializer<fs::path>::BASE_PATH;
        base = path.parent_path();
        if (base.is_relative())
            base = fs::current_path() / base;
        return Deserializer<T>::deserialize(toml::parse_file(path.string()));
    }
}

#define TOML_DESERIALIZE(T, MEMBER)                                                             \
    template <>                                                                                 \
    struct Deserializer<T>                                                                      \
    {                                                                                           \
        static inline T deserialize(const toml::node &v, const std::string &key = "")           \
        {                                                                                       \
            if (!v.is_table())                                                                  \
                throw std::runtime_error(key + " is not a table");                              \
            auto table = *v.as_table();                                                         \
            std::unordered_set<std::string> _had_keys;                                          \
            T result;                                                                           \
            MEMBER                                                                              \
            for (const auto &[k, v] : table)                                                    \
                if (!_had_keys.contains(std::string(k.str())))                                  \
                    std::cout << "[TOML] Warning: '" << key << (key.empty() ? "" : ".")         \
                              << _to_key(std::string(k.str())) << "' is ignored." << std::endl; \
            return result;                                                                      \
        }                                                                                       \
    }

#define _TOML_REQUIRE(k, name)            \
    require(table, name, result.k, key); \
    _had_keys.insert(name)
#define _TOML_OPTIONS(k, name)            \
    options(table, name, result.k, key); \
    _had_keys.insert(name)

#define TOML_REQUIRE(k) _TOML_REQUIRE(k, #k)
#define TOML_OPTIONS(k) _TOML_OPTIONS(k, #k)