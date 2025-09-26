#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

void launch_settings_cli() {
    std::cout << "Arolloa Settings (Minimal)" << std::endl;
    std::cout << "=========================" << std::endl;
    std::cout << "This is a minimal settings interface." << std::endl;
    std::cout << "GUI version requires GTK3 development libraries." << std::endl;
    std::cout << "To install: sudo apt install libgtk-3-dev" << std::endl;
    std::cout << "" << std::endl;
    std::cout << "For a richer desktop experience install recommended fonts such as" << std::endl;
    std::cout << "Noto Sans or Inter using: sudo apt install fonts-noto fonts-inter" << std::endl;

    const char *home = std::getenv("HOME");
    if (!home) {
        return;
    }

    const std::filesystem::path config_dir = std::filesystem::path(home) / ".config" / "arolloa";
    std::error_code ec;
    std::filesystem::create_directories(config_dir, ec);

    const std::filesystem::path config_file = config_dir / "config";
    if (!std::filesystem::exists(config_file)) {
        std::ofstream config(config_file);
        config << "layout.mode=grid" << std::endl;
        config << "animation.enabled=true" << std::endl;
        config << "colors.accent=#cc0000" << std::endl;
        config.close();
        std::cout << "Created basic configuration at " << config_file << std::endl;
    }
}
