#include "../../include/arolloa.h"

void launch_settings_cli();

#if defined(AROLLOA_HAS_GTK)
bool launch_settings_gui();
#endif

int main() {
#if defined(AROLLOA_HAS_GTK)
    if (launch_settings_gui()) {
        return 0;
    }
#endif
    launch_settings_cli();
    return 0;
}
