#pragma once

// =============================================================================
//  config.h
//
//  Lightweight key=value config store for openusm.
//
//  File format (one entry per line):
//      hero_venom=true
//      hero_spiderman=false
//      some_int=42
//      # comments and blank lines are ignored
//
//  Usage:
//      config::load("openusm.cfg");                 // optional, at startup
//      bool on = config::get_bool("hero_venom");    // defaults to false
//      config::set_bool("hero_venom", true);        // writes hero_venom=true
//      config::save("openusm.cfg");                 // optional, to persist
//
//  set_*() mutates the in-memory store immediately; call save() to flush to
//  disk. get_*() reads from the in-memory store only.
// =============================================================================

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unordered_map>

class config
{
public:
    // -- lookups ------------------------------------------------------------
    // All getters return def when the key is absent or cannot be parsed.

    static bool get_bool(const char* key, bool def = false)
    {
        const std::string* v = find(key);
        if (!v)
        {
            return def;
        }
        return parse_bool(*v, def);
    }

    static int get_int(const char* key, int def = 0)
    {
        const std::string* v = find(key);
        if (!v)
        {
            return def;
        }
        char* end = nullptr;
        long n = strtol(v->c_str(), &end, 0);
        if (end == v->c_str())
        {
            return def;
        }
        return static_cast<int>(n);
    }

    static float get_float(const char* key, float def = 0.0f)
    {
        const std::string* v = find(key);
        if (!v)
        {
            return def;
        }
        char* end = nullptr;
        float f = strtof(v->c_str(), &end);
        if (end == v->c_str())
        {
            return def;
        }
        return f;
    }

    static const char* get_string(const char* key, const char* def = "")
    {
        const std::string* v = find(key);
        return v ? v->c_str() : def;
    }

    static bool has(const char* key)
    {
        return find(key) != nullptr;
    }

    // -- mutators -----------------------------------------------------------
    // These update the in-memory store. Call save() to persist to disk.

    static void set_bool(const char* key, bool value)
    {
        store()[key] = value ? "true" : "false";
    }

    static void set_int(const char* key, int value)
    {
        char buf[32];
        snprintf(buf, sizeof(buf), "%d", value);
        store()[key] = buf;
    }

    static void set_float(const char* key, float value)
    {
        char buf[64];
        snprintf(buf, sizeof(buf), "%g", value);
        store()[key] = buf;
    }

    static void set_string(const char* key, const char* value)
    {
        store()[key] = value ? value : "";
    }

    static void erase(const char* key)
    {
        store().erase(key);
    }

    static void clear()
    {
        store().clear();
    }

    // -- persistence --------------------------------------------------------

    // Loads entries from path, merging into the current store
    // (existing keys are overwritten). Returns false if the file
    // could not be opened.
    static bool load(const char* path)
    {
        FILE* fp = fopen(path, "r");
        if (!fp)
        {
            printf("config: could not open '%s' for reading\n", path);
            return false;
        }

        char line[512];
        while (fgets(line, sizeof(line), fp))
        {
            std::string s = trim(line);
            if (s.empty() || s[0] == '#' || s[0] == ';')
            {
                continue;
            }

            const size_t eq = s.find('=');
            if (eq == std::string::npos)
            {
                continue; // malformed line, skip
            }

            std::string key = trim(s.substr(0, eq));
            std::string val = trim(s.substr(eq + 1));
            if (!key.empty())
            {
                store()[key] = val;
            }
        }

        fclose(fp);
        return true;
    }

    // Writes the entire store to path, one key=value per line.
    // Returns false if the file could not be opened for writing.
    static bool save(const char* path)
    {
        FILE* fp = fopen(path, "w");
        if (!fp)
        {
            printf("config: could not open '%s' for writing\n", path);
            return false;
        }

        for (const auto& kv : store())
        {
            fprintf(fp, "%s=%s\n", kv.first.c_str(), kv.second.c_str());
        }

        fclose(fp);
        return true;
    }

private:
    // Meyers singleton: the in-memory store survives static init order.
    static std::unordered_map<std::string, std::string>& store()
    {
        static std::unordered_map<std::string, std::string> s_store;
        return s_store;
    }

    static const std::string* find(const char* key)
    {
        auto& s = store();
        auto it = s.find(key);
        return (it != s.end()) ? &it->second : nullptr;
    }

    static bool parse_bool(const std::string& v, bool def)
    {
        // accept: true/false, 1/0, yes/no, on/off (case-insensitive)
        if (iequals(v, "true") || iequals(v, "1") ||
            iequals(v, "yes")  || iequals(v, "on"))
        {
            return true;
        }
        if (iequals(v, "false") || iequals(v, "0") ||
            iequals(v, "no")    || iequals(v, "off"))
        {
            return false;
        }
        return def;
    }

    static bool iequals(const std::string& a, const char* b)
    {
        size_t i = 0;
        for (; i < a.size() && b[i]; ++i)
        {
            if (tolower(static_cast<unsigned char>(a[i])) !=
                tolower(static_cast<unsigned char>(b[i])))
            {
                return false;
            }
        }
        return i == a.size() && b[i] == '\0';
    }

    static std::string trim(const std::string& s)
    {
        size_t b = 0;
        size_t e = s.size();
        while (b < e && isspace(static_cast<unsigned char>(s[b]))) ++b;
        while (e > b && isspace(static_cast<unsigned char>(s[e - 1]))) --e;
        return s.substr(b, e - b);
    }
};