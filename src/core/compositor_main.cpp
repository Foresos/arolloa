#include "../../include/arolloa.h"

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>
#include <unistd.h>

namespace {
void launch_oobe_if_needed() {
    const char *home = getenv("HOME");
    std::string setup_file = home ? (std::string(home) + "/.config/arolloa/setup_complete") : "/tmp/arolloa_setup_complete";

    if (access(setup_file.c_str(), F_OK) == -1) {
        pid_t child = fork();
        if (child == 0) {
            execlp("./build/arolloa-oobe", "arolloa-oobe", static_cast<char *>(nullptr));
            _exit(1);
        }
    }
}

void print_usage(const char *argv0) {
    std::fprintf(stdout, "Usage: %s [--debug] [--verbose]\n", argv0);
    std::fprintf(stdout, "  --debug    Run nested inside an existing compositor for development.\n");
    std::fprintf(stdout, "  --verbose  Enable verbose wlroots logging.\n");
}
} // namespace

int main(int argc, char **argv) {
    bool debug_mode = false;
    bool verbose_logging = false;

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--debug") == 0) {
            debug_mode = true;
        } else if (std::strcmp(argv[i], "--verbose") == 0) {
            verbose_logging = true;
        } else if (std::strcmp(argv[i], "--help") == 0 || std::strcmp(argv[i], "-h") == 0) {
            print_usage(argv[0]);
            return 0;
        } else {
            std::fprintf(stderr, "Unknown option '%s'\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }

    wlr_log_init((debug_mode || verbose_logging) ? WLR_DEBUG : WLR_INFO, nullptr);

    load_swiss_config();

    ArolloaServer server{};
    server.debug_mode = debug_mode;
    server.nested_backend_active = false;
    server.initialized = false;
    server.startup_opacity = 0.0f;

    server_init(&server);
    if (!server.initialized) {
        wlr_log(WLR_ERROR, "Failed to initialise Arolloa compositor");
        server_destroy(&server);
        return 1;
    }

    launch_oobe_if_needed();

    server_run(&server);
    server_destroy(&server);

    return 0;
}
