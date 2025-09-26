#include "../../include/arolloa.h"
#include <array>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

// Simple configuration without external dependencies
static std::map<std::string, std::string> config;
static std::string config_path;

namespace {

std::string resolve_executable(const std::string &binary_name) {
    const char *override_dir = std::getenv("AROLLOA_BIN_DIR");
    const std::filesystem::path candidate = override_dir ? std::filesystem::path(override_dir) / binary_name
                                                         : std::filesystem::path();
    if (!candidate.empty() && std::filesystem::exists(candidate)) {
        return candidate.string();
    }

    std::array<std::filesystem::path, 3> fallbacks = {
        std::filesystem::path("./build") / binary_name,
        std::filesystem::path("./") / binary_name,
        std::filesystem::path(binary_name),
    };

    for (const auto &path : fallbacks) {
        if (std::filesystem::exists(path)) {
            return path.string();
        }
    }

    return binary_name;
}

void spawn_detached(const std::vector<std::string> &args) {
    if (args.empty()) {
        return;
    }

    pid_t child = fork();
    if (child < 0) {
        wlr_log(WLR_ERROR, "Failed to fork for %s: %s", args.front().c_str(), std::strerror(errno));
        return;
    }

    if (child > 0) {
        int status = 0;
        waitpid(child, &status, 0);
        return;
    }

    if (setsid() < 0) {
        _exit(EXIT_FAILURE);
    }

    pid_t grandchild = fork();
    if (grandchild < 0) {
        _exit(EXIT_FAILURE);
    }

    if (grandchild > 0) {
        _exit(EXIT_SUCCESS);
    }

    std::vector<char *> argv;
    argv.reserve(args.size() + 1);
    for (const auto &arg : args) {
        argv.push_back(const_cast<char *>(arg.c_str()));
    }
    argv.push_back(nullptr);

    execvp(argv.front(), argv.data());
    wlr_log(WLR_ERROR, "Failed to exec %s: %s", argv.front(), std::strerror(errno));
    _exit(EXIT_FAILURE);
}

} // namespace

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

        save_swiss_config();
    }
}

void save_swiss_config() {
    // Ensure config directory exists
    const char* home = getenv("HOME");
    if (home) {
        std::filesystem::create_directories(std::string(home) + "/.config/arolloa");
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
    spawn_detached({resolve_executable("arolloa-settings")});
}

void launch_flatpak_manager() {
    spawn_detached({resolve_executable("arolloa-flatpak")});
}

void launch_system_configurator() {
    spawn_detached({resolve_executable("arolloa-sysconfig")});
}

void launch_oobe() {
    spawn_detached({resolve_executable("arolloa-oobe")});
}
