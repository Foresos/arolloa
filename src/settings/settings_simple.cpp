#include <iostream>
#include <fstream>
#include <string>

int main() {
    std::cout << "Arolloa Settings (Minimal)" << std::endl;
    std::cout << "=========================" << std::endl;
    std::cout << "This is a minimal settings interface." << std::endl;
    std::cout << "GUI version requires GTK3 development libraries." << std::endl;
    std::cout << "To install: sudo apt install libgtk-3-dev" << std::endl;

    // Create basic config if it doesn't exist
    const char* home = getenv("HOME");
    if (home) {
        std::string config_dir = std::string(home) + "/.config/arolloa";
        system(("mkdir -p " + config_dir).c_str());

        std::string config_file = config_dir + "/config";
        std::ifstream test(config_file);
        if (!test.good()) {
            std::ofstream config(config_file);
            config << "layout.mode=grid" << std::endl;
            config << "animation.enabled=true" << std::endl;
            config << "colors.accent=#cc0000" << std::endl;
            config.close();
            std::cout << "Created basic configuration at " << config_file << std::endl;
        }
    }

    return 0;
}
