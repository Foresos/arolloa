#include "../../include/arolloa.h"
#include <gtk/gtk.h>
#include <sys/stat.h>

class ArolloaOOBE {
private:
    GtkWidget *window;
    GtkWidget *stack;
    GtkWidget *header_bar;
    int current_page;
    std::vector<std::string> pages;

public:
    void create_ui() {
        gtk_init(nullptr, nullptr);

        // Swiss design: Clean, welcoming interface
        window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_title(GTK_WINDOW(window), "Welcome to Arolloa");
        gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
        gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
        gtk_window_set_resizable(GTK_WINDOW(window), FALSE);

        apply_swiss_styling();

        // Create header bar
        create_header_bar();

        // Create main stack for pages
        stack = gtk_stack_new();
        gtk_stack_set_transition_type(GTK_STACK(stack), GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT);
        gtk_stack_set_transition_duration(GTK_STACK(stack), 300);

        // Create pages
        create_welcome_page();
        create_region_page();
        create_user_page();
        create_appearance_page();
        create_applications_page();
        create_privacy_page();
        create_complete_page();

        gtk_container_add(GTK_CONTAINER(window), stack);

        current_page = 0;
        pages = {"welcome", "region", "user", "appearance", "applications", "privacy", "complete"};

        g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), nullptr);
        gtk_widget_show_all(window);
    }

    void apply_swiss_styling() {
        GtkCssProvider *provider = gtk_css_provider_new();
        gtk_css_provider_load_from_data(provider, R"CSS(
            window {
                background: linear-gradient(135deg, #ffffff 0%, #f8f8f8 100%);
                font-family: "Helvetica", "Arial", sans-serif;
            }
            .oobe-header {
                background: #ffffff;
                border-bottom: 1px solid #e0e0e0;
                min-height: 48px;
            }
            .oobe-title {
                font-size: 20px;
                font-weight: bold;
                color: #cc0000;
            }
            .oobe-page {
                padding: 48px;
                background: #ffffff;
                border-radius: 8px;
                margin: 32px;
                box-shadow: 0 2px 10px rgba(0,0,0,0.1);
            }
            .page-title {
                font-size: 24px;
                font-weight: bold;
                color: #333333;
                margin-bottom: 16px;
            }
            .page-subtitle {
                font-size: 14px;
                color: #666666;
                margin-bottom: 32px;
            }
            .welcome-logo {
                font-size: 48px;
                font-weight: bold;
                color: #cc0000;
                text-align: center;
                margin-bottom: 24px;
            }
            button {
                background: #ffffff;
                border: 1px solid #cccccc;
                padding: 12px 24px;
                margin: 8px;
                border-radius: 4px;
                font-family: "Helvetica", "Arial", sans-serif;
            }
            button:hover {
                background: #f0f0f0;
            }
            button.primary {
                background: #cc0000;
                color: #ffffff;
                border-color: #aa0000;
            }
            button.primary:hover {
                background: #aa0000;
            }
        )CSS", -1, nullptr);

        GdkScreen *screen = gdk_screen_get_default();
        gtk_style_context_add_provider_for_screen(screen,
            GTK_STYLE_PROVIDER(provider),
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    }

    void create_header_bar() {
        header_bar = gtk_header_bar_new();
        gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(header_bar), FALSE);
        gtk_header_bar_set_title(GTK_HEADER_BAR(header_bar), "Arolloa Setup");

        GtkStyleContext *header_context = gtk_widget_get_style_context(header_bar);
        gtk_style_context_add_class(header_context, "oobe-header");

        gtk_window_set_titlebar(GTK_WINDOW(window), header_bar);
    }

    void create_welcome_page() {
        GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 24);
        GtkStyleContext *page_context = gtk_widget_get_style_context(page);
        gtk_style_context_add_class(page_context, "oobe-page");

        // Logo/Title
        GtkWidget *logo = gtk_label_new("Arolloa");
        GtkStyleContext *logo_context = gtk_widget_get_style_context(logo);
        gtk_style_context_add_class(logo_context, "welcome-logo");

        // Welcome message
        GtkWidget *title = gtk_label_new("Welcome to Arolloa Desktop Environment");
        GtkStyleContext *title_context = gtk_widget_get_style_context(title);
        gtk_style_context_add_class(title_context, "page-title");

        GtkWidget *subtitle = gtk_label_new("A Swiss-inspired desktop environment designed for clarity, functionality, and elegance.\nLet's set up your system for the best possible experience.");
        GtkStyleContext *subtitle_context = gtk_widget_get_style_context(subtitle);
        gtk_style_context_add_class(subtitle_context, "page-subtitle");
        gtk_label_set_justify(GTK_LABEL(subtitle), GTK_JUSTIFY_CENTER);

        // Features list
        GtkWidget *features_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
        add_feature(features_box, "🎯", "Clean Swiss Design", "Minimal, functional interface following International Typographic Style");
        add_feature(features_box, "⚡", "Modern Wayland", "Smooth performance with cutting-edge display technology");
        add_feature(features_box, "🔧", "Easy Configuration", "No terminal required - everything through intuitive GUI");
        add_feature(features_box, "📱", "Flatpak Integration", "Safe, sandboxed applications from Flathub");

        // Navigation
        GtkWidget *nav_box = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
        gtk_button_box_set_layout(GTK_BUTTON_BOX(nav_box), GTK_BUTTONBOX_END);

        GtkWidget *next_btn = gtk_button_new_with_label("Get Started");
        GtkStyleContext *next_context = gtk_widget_get_style_context(next_btn);
        gtk_style_context_add_class(next_context, "primary");
        g_signal_connect(next_btn, "clicked", G_CALLBACK(on_next_clicked), this);

        gtk_container_add(GTK_CONTAINER(nav_box), next_btn);

        gtk_box_pack_start(GTK_BOX(page), logo, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(page), title, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(page), subtitle, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(page), features_box, TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(page), nav_box, FALSE, FALSE, 0);

        gtk_stack_add_named(GTK_STACK(stack), page, "welcome");
    }

    void add_feature(GtkWidget *container, const char* icon, const char* title, const char* description) {
        GtkWidget *feature_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 16);

        GtkWidget *icon_label = gtk_label_new(icon);
        gtk_widget_set_size_request(icon_label, 32, 32);

        GtkWidget *text_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
        GtkWidget *title_label = gtk_label_new(title);
        GtkWidget *desc_label = gtk_label_new(description);

        gtk_label_set_markup(GTK_LABEL(title_label), ("<b>" + std::string(title) + "</b>").c_str());
        gtk_label_set_line_wrap(GTK_LABEL(desc_label), TRUE);
        gtk_widget_set_halign(title_label, GTK_ALIGN_START);
        gtk_widget_set_halign(desc_label, GTK_ALIGN_START);

        gtk_box_pack_start(GTK_BOX(text_box), title_label, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(text_box), desc_label, FALSE, FALSE, 0);

        gtk_box_pack_start(GTK_BOX(feature_box), icon_label, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(feature_box), text_box, TRUE, TRUE, 0);

        gtk_box_pack_start(GTK_BOX(container), feature_box, FALSE, FALSE, 0);
    }

    void create_region_page() {
        GtkWidget *page = create_standard_page("Region & Language",
            "Configure your location, language, and keyboard layout");

        // Language selection
        GtkWidget *lang_frame = gtk_frame_new("Language");
        GtkWidget *lang_combo = gtk_combo_box_text_new();
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(lang_combo), "English (United States)");
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(lang_combo), "English (United Kingdom)");
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(lang_combo), "Deutsch (Deutschland)");
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(lang_combo), "Français (France)");
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(lang_combo), "Español (España)");
        gtk_combo_box_set_active(GTK_COMBO_BOX(lang_combo), 0);
        gtk_container_add(GTK_CONTAINER(lang_frame), lang_combo);

        // Timezone selection
        GtkWidget *tz_frame = gtk_frame_new("Time Zone");
        GtkWidget *tz_combo = gtk_combo_box_text_new();
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(tz_combo), "Europe/Zurich");
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(tz_combo), "Europe/London");
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(tz_combo), "Europe/Berlin");
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(tz_combo), "America/New_York");
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(tz_combo), "America/Los_Angeles");
        gtk_combo_box_set_active(GTK_COMBO_BOX(tz_combo), 0);
        gtk_container_add(GTK_CONTAINER(tz_frame), tz_combo);

        // Keyboard layout
        GtkWidget *kbd_frame = gtk_frame_new("Keyboard Layout");
        GtkWidget *kbd_combo = gtk_combo_box_text_new();
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(kbd_combo), "US English (QWERTY)");
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(kbd_combo), "UK English (QWERTY)");
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(kbd_combo), "German (QWERTZ)");
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(kbd_combo), "French (AZERTY)");
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(kbd_combo), "Swiss German");
        gtk_combo_box_set_active(GTK_COMBO_BOX(kbd_combo), 4);
        gtk_container_add(GTK_CONTAINER(kbd_frame), kbd_combo);

        add_to_page_content(page, lang_frame);
        add_to_page_content(page, tz_frame);
        add_to_page_content(page, kbd_frame);
        add_navigation(page);

        gtk_stack_add_named(GTK_STACK(stack), page, "region");
    }

    void create_user_page() {
        GtkWidget *page = create_standard_page("User Account",
            "Your account information and security settings");

        // User info
        GtkWidget *user_grid = gtk_grid_new();
        gtk_grid_set_row_spacing(GTK_GRID(user_grid), 12);
        gtk_grid_set_column_spacing(GTK_GRID(user_grid), 12);

        gtk_grid_attach(GTK_GRID(user_grid), gtk_label_new("Full Name:"), 0, 0, 1, 1);
        GtkWidget *name_entry = gtk_entry_new();
        gtk_entry_set_placeholder_text(GTK_ENTRY(name_entry), "Enter your full name");
        gtk_grid_attach(GTK_GRID(user_grid), name_entry, 1, 0, 1, 1);

        gtk_grid_attach(GTK_GRID(user_grid), gtk_label_new("Username:"), 0, 1, 1, 1);
        GtkWidget *user_entry = gtk_entry_new();
        gtk_entry_set_placeholder_text(GTK_ENTRY(user_entry), "username");
        gtk_grid_attach(GTK_GRID(user_grid), user_entry, 1, 1, 1, 1);

        // Privacy options
        GtkWidget *privacy_frame = gtk_frame_new("Privacy & Security");
        GtkWidget *privacy_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);

        GtkWidget *auto_login = gtk_check_button_new_with_label("Enable automatic login");
        GtkWidget *guest_account = gtk_check_button_new_with_label("Enable guest account");
        GtkWidget *remote_desktop = gtk_check_button_new_with_label("Allow remote desktop access");

        gtk_box_pack_start(GTK_BOX(privacy_box), auto_login, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(privacy_box), guest_account, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(privacy_box), remote_desktop, FALSE, FALSE, 0);
        gtk_container_add(GTK_CONTAINER(privacy_frame), privacy_box);

        add_to_page_content(page, user_grid);
        add_to_page_content(page, privacy_frame);
        add_navigation(page);

        gtk_stack_add_named(GTK_STACK(stack), page, "user");
    }

    void create_appearance_page() {
        GtkWidget *page = create_standard_page("Desktop Appearance",
            "Customize your Swiss-inspired desktop experience");

        // Layout selection
        GtkWidget *layout_frame = gtk_frame_new("Window Layout Style");
        GtkWidget *layout_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);

        GtkWidget *grid_radio = gtk_radio_button_new_with_label(nullptr, "Grid Layout - Organized windows in mathematical precision");
        GtkWidget *async_radio = gtk_radio_button_new_with_label_from_widget(
            GTK_RADIO_BUTTON(grid_radio), "Asymmetrical Layout - Dynamic balance inspired by Swiss posters");
        GtkWidget *float_radio = gtk_radio_button_new_with_label_from_widget(
            GTK_RADIO_BUTTON(grid_radio), "Floating Layout - Traditional overlapping windows");

        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(grid_radio), TRUE);

        gtk_box_pack_start(GTK_BOX(layout_box), grid_radio, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(layout_box), async_radio, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(layout_box), float_radio, FALSE, FALSE, 0);
        gtk_container_add(GTK_CONTAINER(layout_frame), layout_box);

        // Animation preferences
        GtkWidget *anim_frame = gtk_frame_new("Animation Settings");
        GtkWidget *anim_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);

        GtkWidget *smooth_anim = gtk_check_button_new_with_label("Enable smooth animations");
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(smooth_anim), TRUE);

        GtkWidget *reduce_motion = gtk_check_button_new_with_label("Reduce motion for accessibility");

        gtk_box_pack_start(GTK_BOX(anim_box), smooth_anim, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(anim_box), reduce_motion, FALSE, FALSE, 0);
        gtk_container_add(GTK_CONTAINER(anim_frame), anim_box);

        add_to_page_content(page, layout_frame);
        add_to_page_content(page, anim_frame);
        add_navigation(page);

        gtk_stack_add_named(GTK_STACK(stack), page, "appearance");
    }

    void create_applications_page() {
        GtkWidget *page = create_standard_page("Essential Applications",
            "Install useful applications to get started");

        // Essential apps
        GtkWidget *apps_frame = gtk_frame_new("Recommended Applications");
        GtkWidget *apps_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);

        add_app_option(apps_box, "Firefox", "org.mozilla.firefox", "Modern web browser", true);
        add_app_option(apps_box, "LibreOffice", "org.libreoffice.LibreOffice", "Office suite", true);
        add_app_option(apps_box, "GIMP", "org.gimp.GIMP", "Image editor", false);
        add_app_option(apps_box, "VLC", "org.videolan.VLC", "Media player", true);
        add_app_option(apps_box, "Thunderbird", "org.mozilla.Thunderbird", "Email client", false);
        add_app_option(apps_box, "VS Code", "com.visualstudio.code", "Code editor", false);

        gtk_container_add(GTK_CONTAINER(apps_frame), apps_box);

        add_to_page_content(page, apps_frame);
        add_navigation(page);

        gtk_stack_add_named(GTK_STACK(stack), page, "applications");
    }

    void add_app_option(GtkWidget *container, const char* name, const char* id, const char* description, bool selected) {
        GtkWidget *app_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);

        GtkWidget *check = gtk_check_button_new();
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check), selected);
        g_object_set_data_full(G_OBJECT(check), "app_id", g_strdup(id), g_free);

        GtkWidget *info_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
        GtkWidget *name_label = gtk_label_new(name);
        GtkWidget *desc_label = gtk_label_new(description);

        gtk_label_set_markup(GTK_LABEL(name_label), ("<b>" + std::string(name) + "</b>").c_str());
        gtk_widget_set_halign(name_label, GTK_ALIGN_START);
        gtk_widget_set_halign(desc_label, GTK_ALIGN_START);

        gtk_box_pack_start(GTK_BOX(info_box), name_label, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(info_box), desc_label, FALSE, FALSE, 0);

        gtk_box_pack_start(GTK_BOX(app_box), check, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(app_box), info_box, TRUE, TRUE, 0);

        gtk_box_pack_start(GTK_BOX(container), app_box, FALSE, FALSE, 0);
    }

    void create_privacy_page() {
        GtkWidget *page = create_standard_page("Privacy & Data",
            "Control how Arolloa handles your data and privacy");

        // Telemetry options
        GtkWidget *telemetry_frame = gtk_frame_new("Usage Statistics");
        GtkWidget *telemetry_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);

        GtkWidget *usage_stats = gtk_check_button_new_with_label("Help improve Arolloa by sending anonymous usage statistics");
        GtkWidget *crash_reports = gtk_check_button_new_with_label("Send crash reports to help fix bugs");

        gtk_box_pack_start(GTK_BOX(telemetry_box), usage_stats, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(telemetry_box), crash_reports, FALSE, FALSE, 0);
        gtk_container_add(GTK_CONTAINER(telemetry_frame), telemetry_box);

        // Network options
        GtkWidget *network_frame = gtk_frame_new("Network Settings");
        GtkWidget *network_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);

        GtkWidget *auto_updates = gtk_check_button_new_with_label("Automatically check for system updates");
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(auto_updates), TRUE);

        GtkWidget *flatpak_updates = gtk_check_button_new_with_label("Automatically update Flatpak applications");
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(flatpak_updates), TRUE);

        gtk_box_pack_start(GTK_BOX(network_box), auto_updates, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(network_box), flatpak_updates, FALSE, FALSE, 0);
        gtk_container_add(GTK_CONTAINER(network_frame), network_box);

        add_to_page_content(page, telemetry_frame);
        add_to_page_content(page, network_frame);
        add_navigation(page);

        gtk_stack_add_named(GTK_STACK(stack), page, "privacy");
    }

    void create_complete_page() {
        GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 32);
        GtkStyleContext *page_context = gtk_widget_get_style_context(page);
        gtk_style_context_add_class(page_context, "oobe-page");

        // Success icon
        GtkWidget *success_label = gtk_label_new("🎉");
        gtk_widget_set_size_request(success_label, 64, 64);

        // Completion message
        GtkWidget *title = gtk_label_new("Setup Complete!");
        GtkStyleContext *title_context = gtk_widget_get_style_context(title);
        gtk_style_context_add_class(title_context, "page-title");

        GtkWidget *subtitle = gtk_label_new("Welcome to Arolloa! Your Swiss-inspired desktop environment is ready to use.\nEnjoy the clean, functional, and elegant computing experience.");
        GtkStyleContext *subtitle_context = gtk_widget_get_style_context(subtitle);
        gtk_style_context_add_class(subtitle_context, "page-subtitle");
        gtk_label_set_justify(GTK_LABEL(subtitle), GTK_JUSTIFY_CENTER);

        // Quick tips
        GtkWidget *tips_frame = gtk_frame_new("Quick Tips");
        GtkWidget *tips_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);

        add_tip(tips_box, "Super + Enter", "Open terminal");
        add_tip(tips_box, "Super + D", "Show applications");
        add_tip(tips_box, "Super + L", "Lock screen");
        add_tip(tips_box, "Super + Space", "Switch keyboard layout");

        gtk_container_add(GTK_CONTAINER(tips_frame), tips_box);

        // Finish button
        GtkWidget *finish_btn = gtk_button_new_with_label("Start Using Arolloa");
        GtkStyleContext *finish_context = gtk_widget_get_style_context(finish_btn);
        gtk_style_context_add_class(finish_context, "primary");
        g_signal_connect(finish_btn, "clicked", G_CALLBACK(on_finish_clicked), this);

        gtk_box_pack_start(GTK_BOX(page), success_label, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(page), title, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(page), subtitle, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(page), tips_frame, TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(page), finish_btn, FALSE, FALSE, 0);

        gtk_stack_add_named(GTK_STACK(stack), page, "complete");
    }

    void add_tip(GtkWidget *container, const char* shortcut, const char* description) {
        GtkWidget *tip_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 16);

        GtkWidget *shortcut_label = gtk_label_new(shortcut);
        gtk_widget_set_size_request(shortcut_label, 120, -1);
        gtk_label_set_markup(GTK_LABEL(shortcut_label), ("<tt><b>" + std::string(shortcut) + "</b></tt>").c_str());

        GtkWidget *desc_label = gtk_label_new(description);
        gtk_widget_set_halign(shortcut_label, GTK_ALIGN_START);
        gtk_widget_set_halign(desc_label, GTK_ALIGN_START);

        gtk_box_pack_start(GTK_BOX(tip_box), shortcut_label, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(tip_box), desc_label, TRUE, TRUE, 0);

        gtk_box_pack_start(GTK_BOX(container), tip_box, FALSE, FALSE, 0);
    }

    GtkWidget* create_standard_page(const char* title, const char* subtitle) {
        GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 24);
        GtkStyleContext *page_context = gtk_widget_get_style_context(page);
        gtk_style_context_add_class(page_context, "oobe-page");

        GtkWidget *title_label = gtk_label_new(title);
        GtkStyleContext *title_context = gtk_widget_get_style_context(title_label);
        gtk_style_context_add_class(title_context, "page-title");

        GtkWidget *subtitle_label = gtk_label_new(subtitle);
        GtkStyleContext *subtitle_context = gtk_widget_get_style_context(subtitle_label);
        gtk_style_context_add_class(subtitle_context, "page-subtitle");

        gtk_box_pack_start(GTK_BOX(page), title_label, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(page), subtitle_label, FALSE, FALSE, 0);

        return page;
    }

    void add_to_page_content(GtkWidget *page, GtkWidget *content) {
        gtk_box_pack_start(GTK_BOX(page), content, FALSE, FALSE, 0);
    }

    void add_navigation(GtkWidget *page) {
        GtkWidget *nav_box = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
        gtk_button_box_set_layout(GTK_BUTTON_BOX(nav_box), GTK_BUTTONBOX_SPREAD);

        GtkWidget *back_btn = gtk_button_new_with_label("Back");
        g_signal_connect(back_btn, "clicked", G_CALLBACK(on_back_clicked), this);

        GtkWidget *next_btn = gtk_button_new_with_label("Next");
        GtkStyleContext *next_context = gtk_widget_get_style_context(next_btn);
        gtk_style_context_add_class(next_context, "primary");
        g_signal_connect(next_btn, "clicked", G_CALLBACK(on_next_clicked), this);

        gtk_container_add(GTK_CONTAINER(nav_box), back_btn);
        gtk_container_add(GTK_CONTAINER(nav_box), next_btn);

        gtk_box_pack_end(GTK_BOX(page), nav_box, FALSE, FALSE, 0);
    }

    static void on_next_clicked(GtkWidget *widget, gpointer user_data) {
        ArolloaOOBE *oobe = static_cast<ArolloaOOBE*>(user_data);
        oobe->next_page();
    }

    static void on_back_clicked(GtkWidget *widget, gpointer user_data) {
        ArolloaOOBE *oobe = static_cast<ArolloaOOBE*>(user_data);
        oobe->previous_page();
    }

    static void on_finish_clicked(GtkWidget *widget, gpointer user_data) {
        ArolloaOOBE *oobe = static_cast<ArolloaOOBE*>(user_data);
        oobe->finish_setup();
    }

    void next_page() {
        if (current_page < pages.size() - 1) {
            current_page++;
            gtk_stack_set_visible_child_name(GTK_STACK(stack), pages[current_page].c_str());
        }
    }

    void previous_page() {
        if (current_page > 0) {
            current_page--;
            gtk_stack_set_visible_child_name(GTK_STACK(stack), pages[current_page].c_str());
        }
    }

    void finish_setup() {
        // Create config directory
        system("mkdir -p ~/.config/arolloa");

        // Mark setup as complete
        std::ofstream setup_file("~/.config/arolloa/setup_complete");
        setup_file << "1\n";
        setup_file.close();

        // Apply selected settings
        apply_settings();

        gtk_main_quit();
    }

    void apply_settings() {
        // Apply language and locale settings
        system("localectl set-locale LANG=en_US.UTF-8");

        // Set up Flatpak if not already done
        system("flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo");

        // Install selected applications in background
        std::thread([this]() {
            // This would install selected apps from the applications page
            system("flatpak install -y flathub org.mozilla.firefox");
            system("flatpak install -y flathub org.libreoffice.LibreOffice");
            system("flatpak install -y flathub org.videolan.VLC");
        }).detach();
    }

    void run() {
        gtk_main();
    }
};

void launch_oobe() {
    ArolloaOOBE oobe;
    oobe.create_ui();
    oobe.run();
}
