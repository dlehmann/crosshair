#include "config.hpp"

#include <fstream>
#include <iostream>

#include <yaml-cpp/yaml.h>

namespace {

// The only hard-coded strings. Used both as the in-memory fallback when
// crosshair.yml is missing/unreadable, and as the content written out for a
// freshly generated crosshair.yml. Edit crosshair.yml directly to translate
// or customize the text - see README.md's Localization guide.
constexpr const char* kDefaultTitle = "Crosshair";
constexpr const char* kDefaultInfo = "Do you want to exit Crosshair?";
constexpr const char* kDefaultExitButton = "Exit";
constexpr const char* kDefaultCancelButton = "Cancel";

} // namespace

Config::Config(const std::string& base_dir)
    : yml_path_(base_dir + "/crosshair.yml"),
      strings_{kDefaultTitle, kDefaultInfo, kDefaultExitButton, kDefaultCancelButton} {
    load_or_create();
}

void Config::write_default_file(const std::string& path, const DialogStrings& defaults) {
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "translations" << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "exit_dialog" << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "title" << YAML::Value << defaults.title;
    out << YAML::Key << "info" << YAML::Value << defaults.info;
    out << YAML::Key << "exit_button" << YAML::Value << defaults.exit_button;
    out << YAML::Key << "cancel_button" << YAML::Value << defaults.cancel_button;
    out << YAML::EndMap;
    out << YAML::EndMap;
    out << YAML::EndMap;

    std::ofstream file(path);
    if (!file) {
        std::cerr << "Warning: could not write default crosshair.yml to " << path << "\n";
        return;
    }
    file << out.c_str() << "\n";
    std::cout << "Generated default translation file: " << path << "\n";
}

void Config::load_or_create() {
    std::ifstream probe(yml_path_);
    if (!probe.good()) {
        std::cout << "No crosshair.yml found, using built-in English strings "
                     "and generating a default file.\n";
        write_default_file(yml_path_, strings_);
        return;
    }
    probe.close();

    try {
        YAML::Node root = YAML::LoadFile(yml_path_);
        YAML::Node dialog = root["translations"]["exit_dialog"];
        if (!dialog) {
            std::cerr << "Warning: crosshair.yml has no 'translations: "
                         "exit_dialog' entry, using built-in fallback strings.\n";
            return;
        }

        if (dialog["title"]) {
            strings_.title = dialog["title"].as<std::string>();
        }
        if (dialog["info"]) {
            strings_.info = dialog["info"].as<std::string>();
        }
        if (dialog["exit_button"]) {
            strings_.exit_button = dialog["exit_button"].as<std::string>();
        }
        if (dialog["cancel_button"]) {
            strings_.cancel_button = dialog["cancel_button"].as<std::string>();
        }
        std::cout << "Loaded translations from " << yml_path_ << "\n";
    } catch (const YAML::Exception& ex) {
        std::cerr << "Warning: failed to parse crosshair.yml (" << ex.what()
                  << "), using built-in fallback strings.\n";
    }
}
