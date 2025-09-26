#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace {
const char *kConfigRelativePath = "/.config/arolloa/config";

std::string config_path() {
    const char *home = getenv("HOME");
    if (!home) {
        home = "/tmp";
    }
    return std::string(home) + kConfigRelativePath;
}

void ensure_config_directory() {
    const char *home = getenv("HOME");
    if (!home) {
        return;
    }
    std::string command = std::string("mkdir -p ") + home + "/.config/arolloa";
    std::system(command.c_str());
}

std::map<std::string, std::string> load_config() {
    std::map<std::string, std::string> config;
    std::ifstream file(config_path());
    if (!file.good()) {
        config["layout.mode"] = "grid";
        config["animation.enabled"] = "true";
        config["colors.accent"] = "#3a5f2f";
        config["panel.tray"] = "net,vol,pwr";
        return config;
    }

    std::string line;
    while (std::getline(file, line)) {
        auto pos = line.find('=');
        if (pos == std::string::npos) {
            continue;
        }
        config[line.substr(0, pos)] = line.substr(pos + 1);
    }
    return config;
}

void save_config(const std::map<std::string, std::string> &config) {
    ensure_config_directory();
    std::ofstream file(config_path());
    for (const auto &entry : config) {
        file << entry.first << "=" << entry.second << '\n';
    }
    file.close();
}

void clear_screen() {
    std::cout << "\033[2J\033[H";
}

void show_header() {
    std::cout << "╔══════════════════════════════════════════════╗\n";
    std::cout << "║   Arolloa Forest Settings (Console Edition)   ║\n";
    std::cout << "╠══════════════════════════════════════════════╣\n";
    std::cout << "║ Configure your compositor without GTK or GUI ║\n";
    std::cout << "╚══════════════════════════════════════════════╝\n\n";
}

void show_status(const std::map<std::string, std::string> &config) {
    std::cout << "Current profile:\n";
    std::cout << "  • Window layout : " << config.at("layout.mode") << '\n';
    std::cout << "  • Animations    : " << (config.at("animation.enabled") == "true" ? "enabled" : "disabled") << '\n';
    std::cout << "  • Accent color  : " << config.at("colors.accent") << '\n';
    auto it = config.find("panel.tray");
    if (it != config.end()) {
        std::cout << "  • Tray icons    : " << it->second << '\n';
    }
    std::cout << '\n';
}

void pause_for_enter() {
    std::cout << "Press Enter to return to the menu...";
    std::cout.flush();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

std::string prompt_line(const std::string &prompt, const std::string &fallback = "") {
    std::cout << prompt;
    std::string input;
    std::getline(std::cin, input);
    if (input.empty()) {
        return fallback;
    }
    return input;
}

void configure_layout(std::map<std::string, std::string> &config) {
    clear_screen();
    show_header();
    std::cout << "Choose window layout:\n";
    std::vector<std::pair<std::string, std::string>> layouts = {
        {"grid", "Balanced grid for tiled workspaces"},
        {"asym", "Asymmetrical layout for creative flows"},
        {"floating", "Floating windows for freestyle arrangement"}
    };

    for (std::size_t i = 0; i < layouts.size(); ++i) {
        std::cout << "  [" << (i + 1) << "] " << layouts[i].first << " — " << layouts[i].second << '\n';
    }
    std::cout << '\n';

    std::string choice = prompt_line("Enter number (current: " + config["layout.mode"] + "): ");
    if (choice.empty()) {
        return;
    }

    std::size_t index = 0;
    try {
        index = std::stoul(choice);
    } catch (...) {
        std::cout << "Invalid selection." << std::endl;
        pause_for_enter();
        return;
    }

    if (index == 0 || index > layouts.size()) {
        std::cout << "Invalid selection." << std::endl;
        pause_for_enter();
        return;
    }
    config["layout.mode"] = layouts[index - 1].first;
}

void toggle_animation(std::map<std::string, std::string> &config) {
    bool enabled = config["animation.enabled"] == "true";
    enabled = !enabled;
    config["animation.enabled"] = enabled ? "true" : "false";
    std::cout << "Animations are now " << (enabled ? "enabled" : "disabled") << ".\n";
}

bool is_hex_color(const std::string &value) {
    if (value.size() != 7 || value[0] != '#') {
        return false;
    }
    return std::all_of(value.begin() + 1, value.end(), [](unsigned char c) {
        return std::isxdigit(c);
    });
}

void configure_accent(std::map<std::string, std::string> &config) {
    std::string input = prompt_line("Enter a hex accent color (e.g. #3a5f2f): ", config["colors.accent"]);
    if (!is_hex_color(input)) {
        std::cout << "Invalid color format. Keeping " << config["colors.accent"] << "\n";
        return;
    }
    config["colors.accent"] = input;
}

void configure_tray(std::map<std::string, std::string> &config) {
    std::cout << "Define tray indicators (comma separated tags, e.g. net,vol,pwr):\n";
    std::string input = prompt_line("Tray icons [" + config["panel.tray"] + "]: ", config["panel.tray"]);
    if (!input.empty()) {
        config["panel.tray"] = input;
    }
}

void reset_defaults(std::map<std::string, std::string> &config) {
    config["layout.mode"] = "grid";
    config["animation.enabled"] = "true";
    config["colors.accent"] = "#3a5f2f";
    config["panel.tray"] = "net,vol,pwr";
    std::cout << "Defaults restored." << std::endl;
}

} // namespace

int main() {
    auto config = load_config();

    while (true) {
        clear_screen();
        show_header();
        show_status(config);
        std::cout << "Forest options:\n";
        std::cout << "  [1] Window layout\n";
        std::cout << "  [2] Toggle animations\n";
        std::cout << "  [3] Accent color\n";
        std::cout << "  [4] Tray icons\n";
        std::cout << "  [5] Restore defaults\n";
        std::cout << "  [0] Save and exit\n\n";
        std::cout << "Select an option: ";

        std::string choice;
        std::getline(std::cin, choice);
        if (choice == "0" || choice == "q" || choice == "Q") {
            break;
        }

        if (choice == "1") {
            configure_layout(config);
        } else if (choice == "2") {
            toggle_animation(config);
            pause_for_enter();
        } else if (choice == "3") {
            configure_accent(config);
            pause_for_enter();
        } else if (choice == "4") {
            configure_tray(config);
            pause_for_enter();
        } else if (choice == "5") {
            reset_defaults(config);
            pause_for_enter();
        } else {
            std::cout << "Unknown choice." << std::endl;
            pause_for_enter();
        }
    }

    save_config(config);
    std::cout << "Configuration saved to " << config_path() << "\n";
    std::cout << "Launch the compositor to see your forest changes." << std::endl;
    return 0;
}
