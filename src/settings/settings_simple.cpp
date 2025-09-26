#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <map>
#include <sstream>
#include <string>

#include <gtkmm.h>

namespace {
const char *kConfigRelativePath = "/.config/arolloa/config";

std::string config_path() {
    const char *home = std::getenv("HOME");
    if (!home) {
        return std::string("/tmp") + kConfigRelativePath;
    }
    return std::string(home) + kConfigRelativePath;
}

void ensure_config_directory() {
    const char *home = std::getenv("HOME");
    if (!home) {
        return;
    }
    const std::filesystem::path directory = std::filesystem::path(home) / ".config" / "arolloa";
    try {
        std::filesystem::create_directories(directory);
    } catch (const std::filesystem::filesystem_error &) {
        // Best-effort: ignore errors when we cannot create the directory.
    }
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
        file << entry.first << '=' << entry.second << '\n';
    }
}

std::string rgba_to_hex(const Gdk::RGBA &color) {
    const auto to_hex = [](double component) {
        const int value = std::clamp(static_cast<int>(std::round(component * 255.0)), 0, 255);
        std::ostringstream oss;
        oss << std::uppercase << std::hex << std::setfill('0') << std::setw(2) << value;
        return oss.str();
    };

    return '#' + to_hex(color.get_red()) + to_hex(color.get_green()) + to_hex(color.get_blue());
}

bool is_hex_color(const std::string &value) {
    if (value.size() != 7 || value.front() != '#') {
        return false;
    }
    for (std::size_t i = 1; i < value.size(); ++i) {
        if (!std::isxdigit(static_cast<unsigned char>(value[i]))) {
            return false;
        }
    }
    return true;
}

class SettingsWindow : public Gtk::ApplicationWindow {
  public:
    SettingsWindow();

  protected:
    bool on_delete_event(GdkEventAny *any_event) override;

  private:
    void load_from_config();
    void apply_defaults();
    void save_and_notify();
    void update_config_from_ui();

    std::map<std::string, std::string> config_ = load_config();

    Gtk::ComboBoxText layout_combo_;
    Gtk::Switch animation_switch_;
    Gtk::ColorButton accent_button_;
    Gtk::Entry tray_entry_;
    Gtk::Label status_label_;
    Gtk::Button save_button_{"Save"};
    Gtk::Button defaults_button_{"Restore defaults"};
    Gtk::Button close_button_{"Close"};
};

SettingsWindow::SettingsWindow() {
    set_title("Arolloa Forest Settings");
    set_border_width(18);
    set_default_size(520, 360);

    auto main_box = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_VERTICAL, 12);
    add(*main_box);

    auto title = Gtk::make_managed<Gtk::Label>();
    title->set_markup("<span size='large' weight='bold'>Forest Desktop Experience</span>");
    title->set_halign(Gtk::ALIGN_START);
    main_box->pack_start(*title, Gtk::PACK_SHRINK);

    auto subtitle = Gtk::make_managed<Gtk::Label>(
        "Tune layout, animations, and panel accents with a graphical dashboard.");
    subtitle->set_halign(Gtk::ALIGN_START);
    subtitle->set_line_wrap(true);
    main_box->pack_start(*subtitle, Gtk::PACK_SHRINK);

    auto grid = Gtk::make_managed<Gtk::Grid>();
    grid->set_row_spacing(12);
    grid->set_column_spacing(18);
    main_box->pack_start(*grid, Gtk::PACK_EXPAND_WIDGET);

    auto layout_label = Gtk::make_managed<Gtk::Label>("Window layout:");
    layout_label->set_halign(Gtk::ALIGN_END);
    grid->attach(*layout_label, 0, 0, 1, 1);

    layout_combo_.append("grid", "Grid — balanced tiling");
    layout_combo_.append("asym", "Asym — creative stagger");
    layout_combo_.append("floating", "Floating — free placement");
    layout_combo_.set_hexpand(true);
    grid->attach(layout_combo_, 1, 0, 1, 1);

    auto animation_label = Gtk::make_managed<Gtk::Label>("Animations:");
    animation_label->set_halign(Gtk::ALIGN_END);
    grid->attach(*animation_label, 0, 1, 1, 1);

    animation_switch_.set_hexpand(true);
    animation_switch_.set_halign(Gtk::ALIGN_START);
    grid->attach(animation_switch_, 1, 1, 1, 1);

    auto accent_label = Gtk::make_managed<Gtk::Label>("Accent color:");
    accent_label->set_halign(Gtk::ALIGN_END);
    grid->attach(*accent_label, 0, 2, 1, 1);

    accent_button_.set_use_alpha(false);
    accent_button_.set_hexpand(true);
    accent_button_.set_title("Choose the highlight color");
    grid->attach(accent_button_, 1, 2, 1, 1);

    auto tray_label = Gtk::make_managed<Gtk::Label>("Tray indicators:");
    tray_label->set_halign(Gtk::ALIGN_END);
    grid->attach(*tray_label, 0, 3, 1, 1);

    tray_entry_.set_hexpand(true);
    tray_entry_.set_placeholder_text("e.g. net,vol,pwr");
    grid->attach(tray_entry_, 1, 3, 1, 1);

    auto button_box = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_HORIZONTAL, 6);
    button_box->set_halign(Gtk::ALIGN_END);
    button_box->pack_start(defaults_button_, Gtk::PACK_SHRINK);
    button_box->pack_start(save_button_, Gtk::PACK_SHRINK);
    button_box->pack_start(close_button_, Gtk::PACK_SHRINK);
    main_box->pack_start(*button_box, Gtk::PACK_SHRINK);

    status_label_.set_halign(Gtk::ALIGN_START);
    status_label_.set_margin_top(6);
    main_box->pack_start(status_label_, Gtk::PACK_SHRINK);

    defaults_button_.signal_clicked().connect(sigc::mem_fun(*this, &SettingsWindow::apply_defaults));
    save_button_.signal_clicked().connect(sigc::mem_fun(*this, &SettingsWindow::save_and_notify));
    close_button_.signal_clicked().connect([this]() {
        save_and_notify();
        hide();
    });

    load_from_config();
    show_all_children();
}

bool SettingsWindow::on_delete_event(GdkEventAny *any_event) {
    save_and_notify();
    return Gtk::ApplicationWindow::on_delete_event(any_event);
}

void SettingsWindow::load_from_config() {
    const auto layout = config_.count("layout.mode") ? config_["layout.mode"] : "grid";
    layout_combo_.set_active_id(layout);
    if (!layout_combo_.get_active()) {
        layout_combo_.set_active_id("grid");
    }

    const bool animations_enabled = config_["animation.enabled"] != "false";
    animation_switch_.set_state(animations_enabled);

    const auto accent = config_.count("colors.accent") ? config_["colors.accent"] : "#3a5f2f";
    Gdk::RGBA color;
    if (color.set(accent)) {
        accent_button_.set_rgba(color);
    }

    tray_entry_.set_text(config_["panel.tray"]);
    status_label_.set_text("");
}

void SettingsWindow::apply_defaults() {
    config_["layout.mode"] = "grid";
    config_["animation.enabled"] = "true";
    config_["colors.accent"] = "#3a5f2f";
    config_["panel.tray"] = "net,vol,pwr";
    load_from_config();
    status_label_.set_text("Defaults restored. Remember to save.");
}

void SettingsWindow::update_config_from_ui() {
    if (auto active = layout_combo_.get_active_id(); !active.empty()) {
        config_["layout.mode"] = active;
    }

    config_["animation.enabled"] = animation_switch_.get_state() ? "true" : "false";

    const auto hex = rgba_to_hex(accent_button_.get_rgba());
    if (is_hex_color(hex)) {
        config_["colors.accent"] = hex;
    }

    config_["panel.tray"] = tray_entry_.get_text();
}

void SettingsWindow::save_and_notify() {
    update_config_from_ui();
    save_config(config_);
    status_label_.set_text("Settings saved to " + config_path());
}

} // namespace

int main(int argc, char **argv) {
    auto app = Gtk::Application::create(argc, argv, "org.arolloa.settings");
    SettingsWindow window;
    return app->run(window);
}
