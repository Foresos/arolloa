#include "../../include/arolloa.h"
#include <gtk/gtk.h>
#include <string>

class SwissSettings {
private:
    GtkWidget *window;
    GtkWidget *notebook;

public:
    void create_ui() {
        // Swiss design: Clean, minimal window
        window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_title(GTK_WINDOW(window), "Arolloa Settings");
        gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
        gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);

        // Apply Swiss design styling
        GtkCssProvider *provider = gtk_css_provider_new();
        const std::string font_stack = SwissDesign::font_stack_css();
        const std::string css =
            "window {\n"
            "    background: #ffffff;\n"
            "    font-family: " + font_stack + ";\n"
            "}\n"
            ".swiss-header {\n"
            "    font-size: 18px;\n"
            "    font-weight: bold;\n"
            "    color: #cc0000;\n"
            "    margin: 16px;\n"
            "}\n"
            ".swiss-section {\n"
            "    padding: 8px;\n"
            "    border: 1px solid #f2f2f2;\n"
            "    margin: 8px;\n"
            "}\n"
            "button {\n"
            "    background: #ffffff;\n"
            "    border: 1px solid #cccccc;\n"
            "    padding: 8px 16px;\n"
            "    font-family: " + font_stack + ";\n"
            "}\n"
            "button:hover {\n"
            "    background: #f8f8f8;\n"
            "}\n";

        gtk_css_provider_load_from_data(provider, css.c_str(), css.size(), nullptr);

        GdkScreen *screen = gdk_screen_get_default();
        gtk_style_context_add_provider_for_screen(screen,
            GTK_STYLE_PROVIDER(provider),
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
        g_object_unref(provider);

        // Create notebook for organized settings
        notebook = gtk_notebook_new();
        gtk_container_add(GTK_CONTAINER(window), notebook);

        create_appearance_tab();
        create_behavior_tab();
        create_system_tab();
        create_applications_tab();

        g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), nullptr);
        gtk_widget_show_all(window);
    }

    void create_appearance_tab() {
        GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, SwissDesign::GRID_UNIT);
        gtk_container_set_border_width(GTK_CONTAINER(vbox), SwissDesign::GRID_UNIT * 2);

        // Layout options
        GtkWidget *layout_frame = gtk_frame_new("Window Layout");
        GtkWidget *layout_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, SwissDesign::GRID_UNIT);

        GtkWidget *grid_radio = gtk_radio_button_new_with_label(nullptr, "Grid Layout");
        GtkWidget *async_radio = gtk_radio_button_new_with_label_from_widget(
            GTK_RADIO_BUTTON(grid_radio), "Asymmetrical Layout");
        GtkWidget *float_radio = gtk_radio_button_new_with_label_from_widget(
            GTK_RADIO_BUTTON(grid_radio), "Floating Layout");

        gtk_box_pack_start(GTK_BOX(layout_box), grid_radio, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(layout_box), async_radio, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(layout_box), float_radio, FALSE, FALSE, 0);
        gtk_container_add(GTK_CONTAINER(layout_frame), layout_box);
        gtk_box_pack_start(GTK_BOX(vbox), layout_frame, FALSE, FALSE, 0);

        // Animation settings
        GtkWidget *anim_frame = gtk_frame_new("Animations");
        GtkWidget *anim_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, SwissDesign::GRID_UNIT);

        GtkWidget *anim_check = gtk_check_button_new_with_label("Enable smooth animations");
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(anim_check), TRUE);

        GtkWidget *duration_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.1, 1.0, 0.1);
        gtk_range_set_value(GTK_RANGE(duration_scale), SwissDesign::ANIMATION_DURATION);

        GtkWidget *duration_label = gtk_label_new("Animation Duration (seconds)");

        gtk_box_pack_start(GTK_BOX(anim_box), anim_check, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(anim_box), duration_label, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(anim_box), duration_scale, FALSE, FALSE, 0);
        gtk_container_add(GTK_CONTAINER(anim_frame), anim_box);
        gtk_box_pack_start(GTK_BOX(vbox), anim_frame, FALSE, FALSE, 0);

        gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox, gtk_label_new("Appearance"));
    }

    void create_behavior_tab() {
        GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, SwissDesign::GRID_UNIT);
        gtk_container_set_border_width(GTK_CONTAINER(vbox), SwissDesign::GRID_UNIT * 2);

        // Workspace settings
        GtkWidget *workspace_frame = gtk_frame_new("Workspaces");
        GtkWidget *workspace_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, SwissDesign::GRID_UNIT);

        GtkWidget *workspace_spin = gtk_spin_button_new_with_range(1, 10, 1);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(workspace_spin), 4);
        GtkWidget *workspace_label = gtk_label_new("Number of Workspaces:");

        gtk_box_pack_start(GTK_BOX(workspace_box), workspace_label, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(workspace_box), workspace_spin, FALSE, FALSE, 0);
        gtk_container_add(GTK_CONTAINER(workspace_frame), workspace_box);
        gtk_box_pack_start(GTK_BOX(vbox), workspace_frame, FALSE, FALSE, 0);

        // Window behavior
        GtkWidget *window_frame = gtk_frame_new("Window Behavior");
        GtkWidget *window_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, SwissDesign::GRID_UNIT);

        GtkWidget *focus_check = gtk_check_button_new_with_label("Focus follows mouse");
        GtkWidget *auto_raise_check = gtk_check_button_new_with_label("Auto-raise windows");
        GtkWidget *click_raise_check = gtk_check_button_new_with_label("Click to raise");
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(click_raise_check), TRUE);

        gtk_box_pack_start(GTK_BOX(window_box), focus_check, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(window_box), auto_raise_check, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(window_box), click_raise_check, FALSE, FALSE, 0);
        gtk_container_add(GTK_CONTAINER(window_frame), window_box);
        gtk_box_pack_start(GTK_BOX(vbox), window_frame, FALSE, FALSE, 0);

        gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox, gtk_label_new("Behavior"));
    }

    void create_system_tab() {
        GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, SwissDesign::GRID_UNIT);
        gtk_container_set_border_width(GTK_CONTAINER(vbox), SwissDesign::GRID_UNIT * 2);

        // Quick system settings
        GtkWidget *system_frame = gtk_frame_new("System Settings");
        GtkWidget *system_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, SwissDesign::GRID_UNIT);

        // Add buttons for common system configurations
        auto add_system_button = [&](const char* label, const char* command) {
            GtkWidget *btn = gtk_button_new_with_label(label);
            g_signal_connect(btn, "clicked", G_CALLBACK(+[](GtkWidget*, gpointer data) {
                system((const char*)data);
            }), (gpointer)command);
            gtk_box_pack_start(GTK_BOX(system_box), btn, FALSE, FALSE, 0);
        };

        add_system_button("Network Settings", "nm-connection-editor");
        add_system_button("Audio Settings", "pavucontrol");
        add_system_button("Display Settings", "arandr");
        add_system_button("Power Management", "xfce4-power-manager-settings");
        add_system_button("System Monitor", "gnome-system-monitor");

        // System Configurator launch button
        GtkWidget *sysconfig_btn = gtk_button_new_with_label("Advanced System Configurator");
        g_signal_connect(sysconfig_btn, "clicked", G_CALLBACK(+[](GtkWidget*, gpointer) {
            launch_system_configurator();
        }), nullptr);
        gtk_box_pack_start(GTK_BOX(system_box), sysconfig_btn, FALSE, FALSE, 0);

        gtk_container_add(GTK_CONTAINER(system_frame), system_box);
        gtk_box_pack_start(GTK_BOX(vbox), system_frame, FALSE, FALSE, 0);

        gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox, gtk_label_new("System"));
    }

    void create_applications_tab() {
        GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, SwissDesign::GRID_UNIT);
        gtk_container_set_border_width(GTK_CONTAINER(vbox), SwissDesign::GRID_UNIT * 2);

        // Application management
        GtkWidget *app_frame = gtk_frame_new("Application Management");
        GtkWidget *app_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, SwissDesign::GRID_UNIT);

        GtkWidget *flatpak_btn = gtk_button_new_with_label("Flatpak Package Manager");
        g_signal_connect(flatpak_btn, "clicked", G_CALLBACK(+[](GtkWidget*, gpointer) {
            launch_flatpak_manager();
        }), nullptr);

        GtkWidget *autostart_btn = gtk_button_new_with_label("Manage Autostart Applications");
        g_signal_connect(autostart_btn, "clicked", G_CALLBACK(+[](GtkWidget*, gpointer) {
            system("dex-autostart --list");
        }), nullptr);

        gtk_box_pack_start(GTK_BOX(app_box), flatpak_btn, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(app_box), autostart_btn, FALSE, FALSE, 0);
        gtk_container_add(GTK_CONTAINER(app_frame), app_box);
        gtk_box_pack_start(GTK_BOX(vbox), app_frame, FALSE, FALSE, 0);

        gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox, gtk_label_new("Applications"));
    }

    void run() {
        gtk_main();
    }
};

bool launch_settings_gui() {
    if (!gtk_init_check(nullptr, nullptr)) {
        return false;
    }

    SwissSettings settings;
    settings.create_ui();
    settings.run();
    return true;
}
