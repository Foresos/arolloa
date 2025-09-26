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

#include <gdkmm/screen.h>
#include <glibmm/miscutils.h>
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
    }
}

std::map<std::string, std::string> load_config() {
    std::map<std::string, std::string> config;
    std::ifstream file(config_path());
    if (!file.good()) {
        config["layout.mode"] = "grid";
        config["layout.gap"] = "8";
        config["appearance.primary_font"] = "Neue Haas Grotesk Text";
        config["appearance.panel_height"] = "36";
        config["colors.accent"] = "#d4001a";
        config["colors.panel"] = "#ffffff";
        config["colors.panel_text"] = "#1a1a1a";
        config["animation.enabled"] = "true";
        config["animation.duration"] = "0.2";
        config["notifications.enabled"] = "true";
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

void save_config_map(const std::map<std::string, std::string> &config) {
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

extern "C" {
void launch_flatpak_manager(void);
void launch_system_configurator(void);
void launch_oobe(void);
}

class SwissSettingsWindow : public Gtk::ApplicationWindow {
  public:
    SwissSettingsWindow();

  protected:
    bool on_delete_event(GdkEventAny *any_event) override;

  private:
    void build_ui();
    Gtk::Widget *create_layout_page();
    Gtk::Widget *create_appearance_page();
    Gtk::Widget *create_system_page();
    void apply_css();
    void refresh_css();
    void load_from_config();
    void apply_defaults();
    void update_config_from_ui();
    void save_and_notify();
    void show_status(const std::string &message);

    std::map<std::string, std::string> config_ = load_config();
    Glib::RefPtr<Gtk::CssProvider> css_provider_;

    Gtk::Stack stack_;
    Gtk::StackSidebar sidebar_;
    Gtk::Label status_label_;
    Gtk::Button save_button_{"Save changes"};
    Gtk::Button defaults_button_{"Restore defaults"};

    Gtk::ComboBoxText layout_combo_;
    Gtk::SpinButton gap_spin_;
    Gtk::SpinButton panel_height_spin_;
    Gtk::SpinButton border_spin_;
    Gtk::Entry font_entry_;
    Gtk::Switch animation_switch_;
    Gtk::Scale animation_duration_scale_;
    Gtk::ColorButton accent_button_;
    Gtk::ColorButton panel_color_button_;
    Gtk::ColorButton panel_text_button_;
    Gtk::Switch notifications_switch_;
};

SwissSettingsWindow::SwissSettingsWindow() {
    set_title("Arolloa Swiss Control Center");
    set_default_size(820, 560);
    set_border_width(0);

    apply_css();

    auto header = Gtk::make_managed<Gtk::HeaderBar>();
    header->set_title("Swiss Control Center");
    header->set_subtitle("Curate your Forest experience");
    header->set_show_close_button(true);
    set_titlebar(*header);

    build_ui();
    load_from_config();
    refresh_css();
    show_all_children();
}

void SwissSettingsWindow::apply_css() {
    css_provider_ = Gtk::CssProvider::create();
    const auto screen = Gdk::Screen::get_default();
    if (screen) {
        Gtk::StyleContext::add_provider_for_screen(screen, css_provider_, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    }
}

void SwissSettingsWindow::refresh_css() {
    const auto accent_hex = config_.count("colors.accent") ? config_["colors.accent"] : std::string("#d4001a");
    std::ostringstream css;
    css << R"(
    headerbar {
        background: )" << accent_hex << R"(; color: white; }
    headerbar label { color: white; }
    .sidebar { background: #f5f5f5; }
    .settings-card {
        background: #ffffff;
        border-radius: 18px;
        border: 1px solid rgba(0,0,0,0.05);
        padding: 24px;
    }
    button.suggested-action {
        background: )" << accent_hex << R"(;
        color: white;
        border-radius: 999px;
        padding: 6px 16px;
    }
    button.destructive-action {
        background: rgba(212,0,26,0.12);
        color: #a40016;
        border-radius: 999px;
        padding: 6px 16px;
    }
    )";
    try {
        css_provider_->load_from_data(css.str());
    } catch (const Glib::Error &) {
    }
}

void SwissSettingsWindow::build_ui() {
    auto root = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_VERTICAL, 0);
    add(*root);

    auto content = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_HORIZONTAL, 0);
    root->pack_start(*content, Gtk::PACK_EXPAND_WIDGET);

    sidebar_.set_stack(stack_);
    sidebar_.set_margin_top(24);
    sidebar_.set_margin_bottom(24);
    sidebar_.set_margin_left(24);
    sidebar_.set_size_request(220, -1);
    sidebar_.get_style_context()->add_class("sidebar");

    stack_.set_transition_type(Gtk::STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT);
    stack_.set_transition_duration(350);

    stack_.add(*create_layout_page(), "layout", "Layout");
    stack_.add(*create_appearance_page(), "appearance", "Appearance");
    stack_.add(*create_system_page(), "system", "System");

    content->pack_start(sidebar_, Gtk::PACK_SHRINK);
    content->pack_start(stack_, Gtk::PACK_EXPAND_WIDGET);

    auto footer = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_HORIZONTAL, 12);
    footer->set_margin_top(12);
    footer->set_margin_bottom(12);
    footer->set_margin_right(24);
    footer->set_margin_left(24);
    footer->pack_start(status_label_, Gtk::PACK_EXPAND_WIDGET);

    defaults_button_.get_style_context()->add_class("destructive-action");
    save_button_.get_style_context()->add_class("suggested-action");
    footer->pack_start(defaults_button_, Gtk::PACK_SHRINK);
    footer->pack_start(save_button_, Gtk::PACK_SHRINK);

    root->pack_start(*footer, Gtk::PACK_SHRINK);

    defaults_button_.signal_clicked().connect(sigc::mem_fun(*this, &SwissSettingsWindow::apply_defaults));
    save_button_.signal_clicked().connect(sigc::mem_fun(*this, &SwissSettingsWindow::save_and_notify));
}

Gtk::Widget *SwissSettingsWindow::create_layout_page() {
    auto page = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_VERTICAL, 24);
    page->set_margin(32);

    auto hero = Gtk::make_managed<Gtk::Label>();
    hero->set_markup("<span size='xx-large' weight='bold'>Precision layout</span>");
    hero->set_halign(Gtk::ALIGN_START);
    page->pack_start(*hero, Gtk::PACK_SHRINK);

    auto subtitle = Gtk::make_managed<Gtk::Label>(
        "Choose how windows align, their spacing, and how the shell responds to motion.");
    subtitle->set_line_wrap(true);
    subtitle->set_halign(Gtk::ALIGN_START);
    page->pack_start(*subtitle, Gtk::PACK_SHRINK);

    auto card = Gtk::make_managed<Gtk::Grid>();
    card->get_style_context()->add_class("settings-card");
    card->set_row_spacing(18);
    card->set_column_spacing(18);
    card->set_margin_top(8);
    page->pack_start(*card, Gtk::PACK_SHRINK);

    auto layout_label = Gtk::make_managed<Gtk::Label>("Layout mode");
    layout_label->set_halign(Gtk::ALIGN_END);
    card->attach(*layout_label, 0, 0, 1, 1);

    layout_combo_.append("grid", "Swiss grid – balanced columns");
    layout_combo_.append("asym", "Asymmetric – expressive layering");
    layout_combo_.append("floating", "Floating – free placement");
    layout_combo_.set_hexpand(true);
    card->attach(layout_combo_, 1, 0, 1, 1);

    auto gap_label = Gtk::make_managed<Gtk::Label>("Window gap");
    gap_label->set_halign(Gtk::ALIGN_END);
    card->attach(*gap_label, 0, 1, 1, 1);

    gap_spin_.set_range(0, 64);
    gap_spin_.set_increments(1, 4);
    gap_spin_.set_value(8);
    card->attach(gap_spin_, 1, 1, 1, 1);

    auto border_label = Gtk::make_managed<Gtk::Label>("Border width");
    border_label->set_halign(Gtk::ALIGN_END);
    card->attach(*border_label, 0, 2, 1, 1);

    border_spin_.set_range(0, 8);
    border_spin_.set_increments(1, 1);
    border_spin_.set_value(1);
    card->attach(border_spin_, 1, 2, 1, 1);

    auto panel_height_label = Gtk::make_managed<Gtk::Label>("Panel height");
    panel_height_label->set_halign(Gtk::ALIGN_END);
    card->attach(*panel_height_label, 0, 3, 1, 1);

    panel_height_spin_.set_range(24, 72);
    panel_height_spin_.set_increments(1, 4);
    panel_height_spin_.set_value(36);
    card->attach(panel_height_spin_, 1, 3, 1, 1);

    auto animation_label = Gtk::make_managed<Gtk::Label>("Animations");
    animation_label->set_halign(Gtk::ALIGN_END);
    card->attach(*animation_label, 0, 4, 1, 1);

    animation_switch_.set_halign(Gtk::ALIGN_START);
    card->attach(animation_switch_, 1, 4, 1, 1);

    auto duration_label = Gtk::make_managed<Gtk::Label>("Animation duration");
    duration_label->set_halign(Gtk::ALIGN_END);
    card->attach(*duration_label, 0, 5, 1, 1);

    animation_duration_scale_.set_range(0.05, 0.8);
    animation_duration_scale_.set_value(0.2);
    animation_duration_scale_.set_draw_value(true);
    animation_duration_scale_.set_digits(2);
    animation_duration_scale_.set_hexpand(true);
    card->attach(animation_duration_scale_, 1, 5, 1, 1);

    return page;
}

Gtk::Widget *SwissSettingsWindow::create_appearance_page() {
    auto page = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_VERTICAL, 24);
    page->set_margin(32);

    auto hero = Gtk::make_managed<Gtk::Label>();
    hero->set_markup("<span size='xx-large' weight='bold'>Swiss appearance</span>");
    hero->set_halign(Gtk::ALIGN_START);
    page->pack_start(*hero, Gtk::PACK_SHRINK);

    auto subtitle = Gtk::make_managed<Gtk::Label>(
        "Tune typography and colour accents to mirror Swiss-style clarity across the shell.");
    subtitle->set_line_wrap(true);
    subtitle->set_halign(Gtk::ALIGN_START);
    page->pack_start(*subtitle, Gtk::PACK_SHRINK);

    auto card = Gtk::make_managed<Gtk::Grid>();
    card->get_style_context()->add_class("settings-card");
    card->set_row_spacing(18);
    card->set_column_spacing(18);
    card->set_margin_top(8);
    page->pack_start(*card, Gtk::PACK_SHRINK);

    auto font_label = Gtk::make_managed<Gtk::Label>("Primary font");
    font_label->set_halign(Gtk::ALIGN_END);
    card->attach(*font_label, 0, 0, 1, 1);

    font_entry_.set_placeholder_text("Neue Haas Grotesk Text");
    font_entry_.set_hexpand(true);
    card->attach(font_entry_, 1, 0, 1, 1);

    auto accent_label = Gtk::make_managed<Gtk::Label>("Accent colour");
    accent_label->set_halign(Gtk::ALIGN_END);
    card->attach(*accent_label, 0, 1, 1, 1);

    accent_button_.set_use_alpha(false);
    accent_button_.set_title("Choose accent");
    card->attach(accent_button_, 1, 1, 1, 1);

    auto panel_label = Gtk::make_managed<Gtk::Label>("Panel background");
    panel_label->set_halign(Gtk::ALIGN_END);
    card->attach(*panel_label, 0, 2, 1, 1);

    panel_color_button_.set_use_alpha(false);
    card->attach(panel_color_button_, 1, 2, 1, 1);

    auto panel_text_label = Gtk::make_managed<Gtk::Label>("Panel text");
    panel_text_label->set_halign(Gtk::ALIGN_END);
    card->attach(*panel_text_label, 0, 3, 1, 1);

    panel_text_button_.set_use_alpha(false);
    card->attach(panel_text_button_, 1, 3, 1, 1);

    auto notify_label = Gtk::make_managed<Gtk::Label>("Notifications");
    notify_label->set_halign(Gtk::ALIGN_END);
    card->attach(*notify_label, 0, 4, 1, 1);

    notifications_switch_.set_halign(Gtk::ALIGN_START);
    card->attach(notifications_switch_, 1, 4, 1, 1);

    accent_button_.signal_color_set().connect(sigc::mem_fun(*this, &SwissSettingsWindow::refresh_css));

    return page;
}

Gtk::Widget *SwissSettingsWindow::create_system_page() {
    auto page = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_VERTICAL, 24);
    page->set_margin(32);

    auto hero = Gtk::make_managed<Gtk::Label>();
    hero->set_markup("<span size='xx-large' weight='bold'>System integrations</span>");
    hero->set_halign(Gtk::ALIGN_START);
    page->pack_start(*hero, Gtk::PACK_SHRINK);

    auto subtitle = Gtk::make_managed<Gtk::Label>(
        "Open companion experiences and verify the compositor responds to Swiss notifications.");
    subtitle->set_line_wrap(true);
    subtitle->set_halign(Gtk::ALIGN_START);
    page->pack_start(*subtitle, Gtk::PACK_SHRINK);

    auto card = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_VERTICAL, 18);
    card->get_style_context()->add_class("settings-card");
    card->set_margin_top(8);
    page->pack_start(*card, Gtk::PACK_SHRINK);

    auto flatpak_button = Gtk::make_managed<Gtk::Button>("Open Flatpak manager");
    flatpak_button->get_style_context()->add_class("suggested-action");
    flatpak_button->signal_clicked().connect([]() { launch_flatpak_manager(); });
    card->pack_start(*flatpak_button, Gtk::PACK_SHRINK);

    auto system_button = Gtk::make_managed<Gtk::Button>("Launch system configurator");
    system_button->signal_clicked().connect([]() { launch_system_configurator(); });
    card->pack_start(*system_button, Gtk::PACK_SHRINK);

    auto oobe_button = Gtk::make_managed<Gtk::Button>("Run onboarding experience");
    oobe_button->signal_clicked().connect([]() { launch_oobe(); });
    card->pack_start(*oobe_button, Gtk::PACK_SHRINK);

    auto notify_button = Gtk::make_managed<Gtk::Button>("Preview Swiss notification");
    notify_button->signal_clicked().connect([this]() {
        (void)std::system("notify-send 'Swiss design' 'Accent notifications active.'");
        show_status("Previewed desktop notification via notify-send.");
    });
    card->pack_start(*notify_button, Gtk::PACK_SHRINK);

    return page;
}

void SwissSettingsWindow::load_from_config() {
    const auto layout = config_.count("layout.mode") ? config_["layout.mode"] : std::string("grid");
    layout_combo_.set_active_id(layout);
    if (!layout_combo_.get_active()) {
        layout_combo_.set_active_id("grid");
    }

    if (config_.count("layout.gap")) {
        try {
            gap_spin_.set_value(std::stoi(config_["layout.gap"]));
        } catch (const std::exception &) {
        }
    }
    if (config_.count("layout.border_width")) {
        try {
            border_spin_.set_value(std::stoi(config_["layout.border_width"]));
        } catch (const std::exception &) {
        }
    }
    if (config_.count("appearance.panel_height")) {
        try {
            panel_height_spin_.set_value(std::stoi(config_["appearance.panel_height"]));
        } catch (const std::exception &) {
        }
    }

    font_entry_.set_text(config_["appearance.primary_font"]);

    animation_switch_.set_active(config_["animation.enabled"] != "false");
    if (config_.count("animation.duration")) {
        try {
            animation_duration_scale_.set_value(std::stod(config_["animation.duration"]));
        } catch (const std::exception &) {
        }
    }

    const auto accent_hex = config_.count("colors.accent") ? config_["colors.accent"] : std::string("#d4001a");
    const auto panel_hex = config_.count("colors.panel") ? config_["colors.panel"] : std::string("#ffffff");
    const auto panel_text_hex = config_.count("colors.panel_text") ? config_["colors.panel_text"] : std::string("#1a1a1a");

    Gdk::RGBA color;
    if (color.set(accent_hex)) {
        accent_button_.set_rgba(color);
    }
    if (color.set(panel_hex)) {
        panel_color_button_.set_rgba(color);
    }
    if (color.set(panel_text_hex)) {
        panel_text_button_.set_rgba(color);
    }

    notifications_switch_.set_active(config_["notifications.enabled"] != "false");
    show_status("");
}

void SwissSettingsWindow::apply_defaults() {
    config_["layout.mode"] = "grid";
    config_["layout.gap"] = "8";
    config_["layout.border_width"] = "1";
    config_["appearance.panel_height"] = "36";
    config_["appearance.primary_font"] = "Neue Haas Grotesk Text";
    config_["animation.enabled"] = "true";
    config_["animation.duration"] = "0.2";
    config_["colors.accent"] = "#d4001a";
    config_["colors.panel"] = "#ffffff";
    config_["colors.panel_text"] = "#1a1a1a";
    config_["notifications.enabled"] = "true";
    load_from_config();
    refresh_css();
    show_status("Restored Swiss defaults.");
}

void SwissSettingsWindow::update_config_from_ui() {
    if (auto active = layout_combo_.get_active_id(); !active.empty()) {
        config_["layout.mode"] = active;
    }
    config_["layout.gap"] = std::to_string(gap_spin_.get_value_as_int());
    config_["layout.border_width"] = std::to_string(border_spin_.get_value_as_int());
    config_["appearance.panel_height"] = std::to_string(panel_height_spin_.get_value_as_int());
    config_["appearance.primary_font"] = font_entry_.get_text();
    config_["animation.enabled"] = animation_switch_.get_active() ? "true" : "false";
    config_["animation.duration"] = Glib::Ascii::dtostr(animation_duration_scale_.get_value());

    const auto accent_hex = rgba_to_hex(accent_button_.get_rgba());
    const auto panel_hex = rgba_to_hex(panel_color_button_.get_rgba());
    const auto panel_text_hex = rgba_to_hex(panel_text_button_.get_rgba());

    if (is_hex_color(accent_hex)) {
        config_["colors.accent"] = accent_hex;
    }
    if (is_hex_color(panel_hex)) {
        config_["colors.panel"] = panel_hex;
    }
    if (is_hex_color(panel_text_hex)) {
        config_["colors.panel_text"] = panel_text_hex;
    }

    config_["notifications.enabled"] = notifications_switch_.get_active() ? "true" : "false";
}

void SwissSettingsWindow::save_and_notify() {
    update_config_from_ui();
    save_config_map(config_);
    refresh_css();
    show_status("Saved configuration to " + config_path());
}

void SwissSettingsWindow::show_status(const std::string &message) {
    status_label_.set_text(message);
}

bool SwissSettingsWindow::on_delete_event(GdkEventAny *any_event) {
    save_and_notify();
    return Gtk::ApplicationWindow::on_delete_event(any_event);
}

} // namespace

int main(int argc, char **argv) {
    auto app = Gtk::Application::create(argc, argv, "org.arolloa.settings");
    SwissSettingsWindow window;
    return app->run(window);
}
