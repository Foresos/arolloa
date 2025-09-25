#include "../../include/arolloa.h"
#include <gtk/gtk.h>
#include <cstdlib>

class FlatpakManager {
private:
    GtkWidget *window;
    GtkWidget *search_entry;
    GtkWidget *app_list;
    GtkWidget *install_btn;
    GtkWidget *remove_btn;
    GtkWidget *update_btn;
    GtkWidget *status_label;
    GtkListStore *list_store;

public:
    void create_ui() {
        gtk_init(nullptr, nullptr);

        window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_title(GTK_WINDOW(window), "Arolloa Package Manager");
        gtk_window_set_default_size(GTK_WINDOW(window), 900, 600);
        gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);

        apply_swiss_styling();

        GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, SwissDesign::GRID_UNIT);
        gtk_container_add(GTK_CONTAINER(window), vbox);

        // Header
        create_header(vbox);

        // Search bar
        create_search_bar(vbox);

        // Application list
        create_app_list(vbox);

        // Action buttons
        create_action_buttons(vbox);

        // Status bar
        create_status_bar(vbox);

        // Initialize
        refresh_installed_apps();

        g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), nullptr);
        gtk_widget_show_all(window);
    }

    void apply_swiss_styling() {
        GtkCssProvider *provider = gtk_css_provider_new();
        gtk_css_provider_load_from_data(provider, R"CSS(
            window {
                background: #ffffff;
                font-family: "Helvetica", "Arial", sans-serif;
            }
            .app-header {
                background: #f8f8f8;
                padding: 16px;
                border-bottom: 1px solid #e0e0e0;
            }
            .app-title {
                font-size: 18px;
                font-weight: bold;
                color: #cc0000;
            }
            .search-bar {
                padding: 8px;
                background: #fafafa;
            }
            .app-list {
                border: 1px solid #e0e0e0;
            }
            button {
                background: #ffffff;
                border: 1px solid #cccccc;
                padding: 8px 16px;
                margin: 4px;
            }
            button:hover {
                background: #f0f0f0;
            }
            button.install {
                background: #4caf50;
                color: white;
                border-color: #45a049;
            }
            button.remove {
                background: #f44336;
                color: white;
                border-color: #da190b;
            }
            button.update {
                background: #2196f3;
                color: white;
                border-color: #0b7dda;
            }
            .status-bar {
                background: #f0f0f0;
                padding: 8px;
                border-top: 1px solid #e0e0e0;
            }
        )CSS", -1, nullptr);

        GdkScreen *screen = gdk_screen_get_default();
        gtk_style_context_add_provider_for_screen(screen,
            GTK_STYLE_PROVIDER(provider),
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    }

    void create_header(GtkWidget *parent) {
        GtkWidget *header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
        GtkStyleContext *header_context = gtk_widget_get_style_context(header);
        gtk_style_context_add_class(header_context, "app-header");

        GtkWidget *title = gtk_label_new("Flatpak Package Manager");
        GtkStyleContext *title_context = gtk_widget_get_style_context(title);
        gtk_style_context_add_class(title_context, "app-title");

        GtkWidget *subtitle = gtk_label_new("Install and manage applications safely with Flatpak");
        gtk_widget_set_margin_left(subtitle, 16);

        gtk_box_pack_start(GTK_BOX(header), title, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(header), subtitle, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(parent), header, FALSE, FALSE, 0);
    }

    void create_search_bar(GtkWidget *parent) {
        GtkWidget *search_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, SwissDesign::GRID_UNIT);
        GtkStyleContext *search_context = gtk_widget_get_style_context(search_box);
        gtk_style_context_add_class(search_context, "search-bar");

        GtkWidget *search_label = gtk_label_new("Search:");
        search_entry = gtk_entry_new();
        gtk_entry_set_placeholder_text(GTK_ENTRY(search_entry), "Search for applications...");

        GtkWidget *search_btn = gtk_button_new_with_label("Search Flathub");
        g_signal_connect(search_btn, "clicked", G_CALLBACK(on_search_clicked), this);

        gtk_box_pack_start(GTK_BOX(search_box), search_label, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(search_box), search_entry, TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(search_box), search_btn, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(parent), search_box, FALSE, FALSE, 0);
    }

    void create_app_list(GtkWidget *parent) {
        // Create list store
        list_store = gtk_list_store_new(5,
            G_TYPE_STRING,  // Name
            G_TYPE_STRING,  // ID
            G_TYPE_STRING,  // Version
            G_TYPE_STRING,  // Description
            G_TYPE_STRING   // Status
        );

        // Create tree view
        app_list = gtk_tree_view_new_with_model(GTK_TREE_MODEL(list_store));
        GtkStyleContext *list_context = gtk_widget_get_style_context(app_list);
        gtk_style_context_add_class(list_context, "app-list");

        // Add columns
        add_column("Application", 0);
        add_column("ID", 1);
        add_column("Version", 2);
        add_column("Description", 3);
        add_column("Status", 4);

        // Create scrolled window
        GtkWidget *scrolled = gtk_scrolled_window_new(nullptr, nullptr);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
            GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
        gtk_container_add(GTK_CONTAINER(scrolled), app_list);

        gtk_box_pack_start(GTK_BOX(parent), scrolled, TRUE, TRUE, 0);
    }

    void add_column(const char* title, int column_id) {
        GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
        GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(
            title, renderer, "text", column_id, nullptr);
        gtk_tree_view_column_set_resizable(column, TRUE);
        gtk_tree_view_append_column(GTK_TREE_VIEW(app_list), column);
    }

    void create_action_buttons(GtkWidget *parent) {
        GtkWidget *button_box = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
        gtk_button_box_set_layout(GTK_BUTTON_BOX(button_box), GTK_BUTTONBOX_CENTER);

        install_btn = gtk_button_new_with_label("Install Application");
        GtkStyleContext *install_context = gtk_widget_get_style_context(install_btn);
        gtk_style_context_add_class(install_context, "install");
        g_signal_connect(install_btn, "clicked", G_CALLBACK(on_install_clicked), this);

        remove_btn = gtk_button_new_with_label("Remove Application");
        GtkStyleContext *remove_context = gtk_widget_get_style_context(remove_btn);
        gtk_style_context_add_class(remove_context, "remove");
        g_signal_connect(remove_btn, "clicked", G_CALLBACK(on_remove_clicked), this);

        update_btn = gtk_button_new_with_label("Update All");
        GtkStyleContext *update_context = gtk_widget_get_style_context(update_btn);
        gtk_style_context_add_class(update_context, "update");
        g_signal_connect(update_btn, "clicked", G_CALLBACK(on_update_clicked), this);

        GtkWidget *refresh_btn = gtk_button_new_with_label("Refresh List");
        g_signal_connect(refresh_btn, "clicked", G_CALLBACK(on_refresh_clicked), this);

        gtk_container_add(GTK_CONTAINER(button_box), install_btn);
        gtk_container_add(GTK_CONTAINER(button_box), remove_btn);
        gtk_container_add(GTK_CONTAINER(button_box), update_btn);
        gtk_container_add(GTK_CONTAINER(button_box), refresh_btn);

        gtk_box_pack_start(GTK_BOX(parent), button_box, FALSE, FALSE, SwissDesign::GRID_UNIT);
    }

    void create_status_bar(GtkWidget *parent) {
        GtkWidget *status_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
        GtkStyleContext *status_context = gtk_widget_get_style_context(status_box);
        gtk_style_context_add_class(status_context, "status-bar");

        status_label = gtk_label_new("Ready");
        gtk_box_pack_start(GTK_BOX(status_box), status_label, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(parent), status_box, FALSE, FALSE, 0);
    }

    void refresh_installed_apps() {
        gtk_list_store_clear(list_store);

        FILE *fp = popen("flatpak list --columns=name,application,version,description 2>/dev/null", "r");
        if (!fp) {
            update_status("Error: Could not execute flatpak command");
            return;
        }

        char line[1024];
        while (fgets(line, sizeof(line), fp)) {
            std::string str_line(line);
            if (str_line.empty() || str_line.find("Name") == 0) continue;

            std::vector<std::string> parts = split_string(str_line, '\t');
            if (parts.size() >= 3) {
                GtkTreeIter iter;
                gtk_list_store_append(list_store, &iter);
                gtk_list_store_set(list_store, &iter,
                    0, parts[0].c_str(),
                    1, parts.size() > 1 ? parts[1].c_str() : "",
                    2, parts.size() > 2 ? parts[2].c_str() : "",
                    3, parts.size() > 3 ? parts[3].c_str() : "",
                    4, "Installed",
                    -1);
            }
        }
        pclose(fp);

        update_status("Application list refreshed");
    }

    std::vector<std::string> split_string(const std::string& str, char delimiter) {
        std::vector<std::string> result;
        std::stringstream ss(str);
        std::string item;

        while (std::getline(ss, item, delimiter)) {
            result.push_back(item);
        }
        return result;
    }

    void update_status(const std::string& message) {
        gtk_label_set_text(GTK_LABEL(status_label), message.c_str());
    }

    static void on_search_clicked(GtkWidget *widget, gpointer user_data) {
        FlatpakManager *manager = static_cast<FlatpakManager*>(user_data);
        manager->search_applications();
    }

    static void on_install_clicked(GtkWidget *widget, gpointer user_data) {
        FlatpakManager *manager = static_cast<FlatpakManager*>(user_data);
        manager->install_selected_app();
    }

    static void on_remove_clicked(GtkWidget *widget, gpointer user_data) {
        FlatpakManager *manager = static_cast<FlatpakManager*>(user_data);
        manager->remove_selected_app();
    }

    static void on_update_clicked(GtkWidget *widget, gpointer user_data) {
        FlatpakManager *manager = static_cast<FlatpakManager*>(user_data);
        manager->update_all_apps();
    }

    static void on_refresh_clicked(GtkWidget *widget, gpointer user_data) {
        FlatpakManager *manager = static_cast<FlatpakManager*>(user_data);
        manager->refresh_installed_apps();
    }

    void search_applications() {
        const char* search_text = gtk_entry_get_text(GTK_ENTRY(search_entry));
        if (strlen(search_text) < 3) {
            update_status("Please enter at least 3 characters to search");
            return;
        }

        gtk_list_store_clear(list_store);
        update_status("Searching Flathub...");

        std::string command = "flatpak search " + std::string(search_text) + " 2>/dev/null";
        FILE *fp = popen(command.c_str(), "r");
        if (!fp) {
            update_status("Error: Could not search Flathub");
            return;
        }

        char line[1024];
        while (fgets(line, sizeof(line), fp)) {
            std::string str_line(line);
            if (str_line.empty() || str_line.find("Name") == 0) continue;

            std::vector<std::string> parts = split_string(str_line, '\t');
            if (parts.size() >= 2) {
                GtkTreeIter iter;
                gtk_list_store_append(list_store, &iter);
                gtk_list_store_set(list_store, &iter,
                    0, parts[0].c_str(),
                    1, parts.size() > 1 ? parts[1].c_str() : "",
                    2, "",
                    3, parts.size() > 2 ? parts[2].c_str() : "",
                    4, "Available",
                    -1);
            }
        }
        pclose(fp);

        update_status("Search completed");
    }

    void install_selected_app() {
        GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(app_list));
        GtkTreeIter iter;
        GtkTreeModel *model;

        if (!gtk_tree_selection_get_selected(selection, &model, &iter)) {
            update_status("Please select an application to install");
            return;
        }

        gchar *app_id;
        gtk_tree_model_get(model, &iter, 1, &app_id, -1);

        std::string command = "pkexec flatpak install -y flathub " + std::string(app_id);
        update_status("Installing application... Please wait");

        int result = system(command.c_str());
        if (result == 0) {
            update_status("Application installed successfully");
            refresh_installed_apps();
        } else {
            update_status("Installation failed");
        }

        g_free(app_id);
    }

    void remove_selected_app() {
        GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(app_list));
        GtkTreeIter iter;
        GtkTreeModel *model;

        if (!gtk_tree_selection_get_selected(selection, &model, &iter)) {
            update_status("Please select an application to remove");
            return;
        }

        gchar *app_id;
        gchar *status;
        gtk_tree_model_get(model, &iter, 1, &app_id, 4, &status, -1);

        if (strcmp(status, "Installed") != 0) {
            update_status("Application is not installed");
            g_free(app_id);
            g_free(status);
            return;
        }

        std::string command = "pkexec flatpak uninstall -y " + std::string(app_id);
        update_status("Removing application... Please wait");

        int result = system(command.c_str());
        if (result == 0) {
            update_status("Application removed successfully");
            refresh_installed_apps();
        } else {
            update_status("Removal failed");
        }

        g_free(app_id);
        g_free(status);
    }

    void update_all_apps() {
        update_status("Updating all applications... Please wait");
        int result = system("pkexec flatpak update -y");
        if (result == 0) {
            update_status("All applications updated successfully");
            refresh_installed_apps();
        } else {
            update_status("Update failed");
        }
    }

    void run() {
        gtk_main();
    }
};

void launch_flatpak_manager() {
    FlatpakManager manager;
    manager.create_ui();
    manager.run();
}

int main() {
    launch_flatpak_manager();
    return 0;
}
