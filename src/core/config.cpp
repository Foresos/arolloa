#include "../../include/arolloa.h"
#include <cstring>

// Simple configuration without external dependencies
static std::map<std::string, std::string> config;
static std::string config_path;

void load_swiss_config() {
    const char* home = getenv("HOME");
    if (!home) home = "/tmp";
    config_path = std::string(home) + "/.config/arolloa/config";

    std::ifstream config_file(config_path);
    if (config_file.good()) {
        std::string line;
        while (std::getline(config_file, line)) {
            auto pos = line.find('=');
            if (pos != std::string::npos) {
                std::string key = line.substr(0, pos);
                std::string value = line.substr(pos + 1);
                config[key] = value;
            }
        }
        config_file.close();
    } else {
        // Default Swiss design configuration
        config["layout.mode"] = "grid";
        config["layout.gap"] = std::to_string(SwissDesign::WINDOW_GAP);
        config["layout.border_width"] = std::to_string(SwissDesign::BORDER_WIDTH);
        config["appearance.primary_font"] = SwissDesign::PRIMARY_FONT;
        config["appearance.panel_height"] = std::to_string(SwissDesign::PANEL_HEIGHT);
        config["appearance.corner_radius"] = std::to_string(SwissDesign::CORNER_RADIUS);
        config["animation.enabled"] = "true";
        config["animation.duration"] = std::to_string(SwissDesign::ANIMATION_DURATION);
        config["colors.background"] = "#ffffff";
        config["colors.foreground"] = "#000000";
        config["colors.accent"] = "#cc0000";
        config["colors.panel"] = "#ffffff";
        config["colors.panel_text"] = "#202020";
        config["notifications.enabled"] = "true";

        save_swiss_config();
    }
}

void save_swiss_config() {
    // Ensure config directory exists
    const char* home = getenv("HOME");
    if (home) {
        std::string mkdir_cmd = "mkdir -p " + std::string(home) + "/.config/arolloa";
        system(mkdir_cmd.c_str());
    }

    std::ofstream config_file(config_path);
    for (const auto& pair : config) {
        config_file << pair.first << "=" << pair.second << std::endl;
    }
    config_file.close();
}

std::string get_config_string(const std::string& key, const std::string& default_value) {
    auto it = config.find(key);
    return (it != config.end()) ? it->second : default_value;
}

int get_config_int(const std::string& key, int default_value) {
    auto it = config.find(key);
    if (it != config.end()) {
        try {
            return std::stoi(it->second);
        } catch (const std::exception&) {
            return default_value;
        }
    }
    return default_value;
}

bool get_config_bool(const std::string& key, bool default_value) {
    auto it = config.find(key);
    if (it != config.end()) {
        return it->second == "true" || it->second == "1";
    }
    return default_value;
}

// C wrapper functions
extern "C" {
    void load_swiss_config_c() { load_swiss_config(); }
    void save_swiss_config_c() { save_swiss_config(); }
}

// Stub implementations for missing functions
void launch_settings() {
    system("./build/arolloa-settings &");
}

void launch_flatpak_manager() {
    system("./build/arolloa-flatpak &");
}

void launch_system_configurator() {
    system("./build/arolloa-sysconfig &");
}

void launch_oobe() {
    system("./build/arolloa-oobe &");
}
