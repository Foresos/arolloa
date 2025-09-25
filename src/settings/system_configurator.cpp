#include "../../include/arolloa.h"
#include <gtk/gtk.h>
#include <fstream>
#include <sstream>

class SystemConfigurator {
private:
    GtkWidget *window;
    GtkWidget *tree_view;
    GtkWidget *detail_view;
    GtkTreeStore *tree_store;

    struct ConfigItem {
        std::string name;
        std::string path;
        std::string description;
        std::string current_value;
        bool is_file;
    };

    std::vector<ConfigItem> config_items;

public:
    void initialize() {
        // Populate system configuration items
        config_items = {
            {"Network", "/etc/NetworkManager/NetworkManager.conf", "Network Manager Configuration", "", true},
            {"DNS", "/etc/systemd/resolved.conf", "DNS Resolution Settings", "", true},
            {"Firewall", "/etc/ufw/ufw.conf", "Uncomplicated Firewall", "", true},
            {"Audio", "/etc/pulse/default.pa", "PulseAudio Configuration", "", true},
            {"Graphics", "/etc/X11/xorg.conf", "X11 Graphics Configuration", "", true},
            {"Boot Loader", "/etc/default/grub", "GRUB Boot Loader", "", true},
            {"System Services", "/etc/systemd/system/", "SystemD Services", "", false},
            {"User Accounts", "/etc/passwd", "System Users", "", true},
            {"Groups", "/etc/group", "User Groups", "", true},
            {"Mount Points", "/etc/fstab", "File System Table", "", true},
            {"Environment", "/etc/environment", "System Environment Variables", "", true},
            {"Locale", "/etc/locale.conf", "System Locale Settings", "", true},
            {"Time Zone", "/etc/localtime", "System Time Zone", "", false},
            {"Hostname", "/etc/hostname", "System Hostname", "", true},
            {"Hosts", "/etc/hosts", "Network Hosts File", "", true}
        };
    }

    void create_ui() {
        gtk_init(nullptr, nullptr);

        window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_title(GTK_WINDOW(window), "Arolloa System Configurator");
        gtk_window_set_default_size(GTK_WINDOW(window), 1000, 700);
        gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);

        // Swiss design styling
        apply_swiss_styling();

        // Create paned layout
        GtkWidget *paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
        gtk_container_add(GTK_CONTAINER(window), paned);

        // Left panel: Configuration tree
        create_tree_view();
        gtk_paned_pack1(GTK_PANED(paned), create_scrolled_tree(), TRUE, FALSE);

        // Right panel: Detail view
        create_detail_view();
        gtk_paned_pack2(GTK_PANED(paned), detail_view, TRUE, FALSE);

        gtk_paned_set_position(GTK_PANED(paned), 300);

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
            .swiss-header {
                font-size: 16px;
                font-weight: bold;
                color: #cc0000;
                padding: 8px;
                background: #f8f8f8;
                border-bottom: 1px solid #cccccc;
            }
            .config-tree {
                background: #fafafa;
                border-right: 1px solid #e0e0e0;
            }
            .config-detail {
                background: #ffffff;
                padding: 16px;
            }
            textview {
                font-family: "Monaco", "Consolas", monospace;
                font-size: 10px;
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

    void create_tree_view() {
        tree_store = gtk_tree_store_new(3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER);
        tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(tree_store));

        GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
        GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(
            "System Configuration", renderer, "text", 0, nullptr);
        gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);

        // Populate tree
        populate_tree();

        // Connect selection handler
        GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree_view));
        g_signal_connect(selection, "changed", G_CALLBACK(on_tree_selection_changed), this);
    }

    GtkWidget* create_scrolled_tree() {
        GtkWidget *scrolled = gtk_scrolled_window_new(nullptr, nullptr);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
            GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
        gtk_container_add(GTK_CONTAINER(scrolled), tree_view);

        GtkStyleContext *context = gtk_widget_get_style_context(scrolled);
        gtk_style_context_add_class(context, "config-tree");

        return scrolled;
    }

    void create_detail_view() {
        detail_view = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

        // Header
        GtkWidget *header = gtk_label_new("Select a configuration item");
        GtkStyleContext *header_context = gtk_widget_get_style_context(header);
        gtk_style_context_add_class(header_context, "swiss-header");
        gtk_box_pack_start(GTK_BOX(detail_view), header, FALSE, FALSE, 0);

        // Content area
        GtkWidget *content_frame = gtk_frame_new(nullptr);
        gtk_frame_set_shadow_type(GTK_FRAME(content_frame), GTK_SHADOW_IN);

        GtkWidget *text_view = gtk_text_view_new();
        gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), TRUE);
        gtk_text_view_set_monospace(GTK_TEXT_VIEW(text_view), TRUE);

        GtkWidget *scrolled_text = gtk_scrolled_window_new(nullptr, nullptr);
        gtk_container_add(GTK_CONTAINER(scrolled_text), text_view);
        gtk_container_add(GTK_CONTAINER(content_frame), scrolled_text);

        gtk_box_pack_start(GTK_BOX(detail_view), content_frame, TRUE, TRUE, 8);

        // Action buttons
        GtkWidget *button_box = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
        gtk_button_box_set_layout(GTK_BUTTON_BOX(button_box), GTK_BUTTONBOX_END);

        GtkWidget *backup_btn = gtk_button_new_with_label("Backup");
        GtkWidget *restore_btn = gtk_button_new_with_label("Restore");
        GtkWidget *save_btn = gtk_button_new_with_label("Save Changes");
        GtkStyleContext *save_context = gtk_widget_get_style_context(save_btn);
        gtk_style_context_add_class(save_context, "primary");

        gtk_container_add(GTK_CONTAINER(button_box), backup_btn);
        gtk_container_add(GTK_CONTAINER(button_box), restore_btn);
        gtk_container_add(GTK_CONTAINER(button_box), save_btn);

        gtk_box_pack_start(GTK_BOX(detail_view), button_box, FALSE, FALSE, 8);

        GtkStyleContext *detail_context = gtk_widget_get_style_context(detail_view);
        gtk_style_context_add_class(detail_context, "config-detail");
    }

    void populate_tree() {
        GtkTreeIter category_iter, item_iter;

        // Organize by categories
        std::map<std::string, std::vector<ConfigItem*>> categories = {
            {"Network", {}},
            {"System", {}},
            {"Hardware", {}},
            {"Security", {}},
            {"User Management", {}}
        };

        // Categorize items
        for (auto &item : config_items) {
            if (item.name.find("Network") != std::string::npos ||
                item.name.find("DNS") != std::string::npos ||
                item.name.find("Hosts") != std::string::npos) {
                categories["Network"].push_back(&item);
            } else if (item.name.find("Boot") != std::string::npos ||
                       item.name.find("Services") != std::string::npos ||
                       item.name.find("Environment") != std::string::npos) {
                categories["System"].push_back(&item);
            } else if (item.name.find("Graphics") != std::string::npos ||
                       item.name.find("Audio") != std::string::npos) {
                categories["Hardware"].push_back(&item);
            } else if (item.name.find("Firewall") != std::string::npos) {
                categories["Security"].push_back(&item);
            } else {
                categories["User Management"].push_back(&item);
            }
        }

        // Add to tree
        for (const auto &category : categories) {
            if (category.second.empty()) continue;

            gtk_tree_store_append(tree_store, &category_iter, nullptr);
            gtk_tree_store_set(tree_store, &category_iter,
                0, category.first.c_str(),
                1, "",
                2, nullptr,
                -1);

            for (const ConfigItem *item : category.second) {
                gtk_tree_store_append(tree_store, &item_iter, &category_iter);
                gtk_tree_store_set(tree_store, &item_iter,
                    0, item->name.c_str(),
                    1, item->path.c_str(),
                    2, (gpointer)item,
                    -1);
            }
        }

        gtk_tree_view_expand_all(GTK_TREE_VIEW(tree_view));
    }

    static void on_tree_selection_changed(GtkTreeSelection *selection, gpointer user_data) {
        SystemConfigurator *self = static_cast<SystemConfigurator*>(user_data);
        self->handle_selection_changed(selection);
    }

    void handle_selection_changed(GtkTreeSelection *selection) {
        GtkTreeIter iter;
        GtkTreeModel *model;

        if (!gtk_tree_selection_get_selected(selection, &model, &iter)) {
            return;
        }

        gchar *name;
        gchar *path;
        ConfigItem *item;
        gtk_tree_model_get(model, &iter, 0, &name, 1, &path, 2, &item, -1);

        if (item) {
            display_config_file(item);
        }

        g_free(name);
        g_free(path);
    }

    void display_config_file(const ConfigItem *item) {
        std::ifstream file(item->path);
        std::stringstream buffer;

        if (file.good()) {
            buffer << file.rdbuf();
            file.close();
        } else {
            buffer << "# Configuration file: " << item->path << "\n";
            buffer << "# " << item->description << "\n";
            buffer << "# File not found or not readable.\n";
            buffer << "# This may be normal for some system files.\n";
        }

        // Update header and content in detail view
        // Implementation would update the text view with file contents
        // and provide save/backup functionality
    }

    void run() {
        gtk_main();
    }
};

void launch_system_configurator() {
    SystemConfigurator configurator;
    configurator.initialize();
    configurator.create_ui();
    configurator.run();
}

int main() {
    launch_system_configurator();
    return 0;
}
