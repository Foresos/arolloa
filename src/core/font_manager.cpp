#include "../../include/arolloa.h"

#include <fontconfig/fontconfig.h>

namespace {
std::string pick_font_family(std::span<const char *const> candidates) {
    if (candidates.empty()) {
        return "sans-serif";
    }

    if (!FcInit()) {
        // Fontconfig failed to initialise; fall back to the first candidate.
        return candidates.front() ? candidates.front() : "sans-serif";
    }

    FcConfig *config = FcConfigGetCurrent();
    if (!config) {
        return candidates.front() ? candidates.front() : "sans-serif";
    }

    for (const char *candidate : candidates) {
        if (!candidate || candidate[0] == '\0') {
            continue;
        }

        FcPattern *pattern = FcNameParse(reinterpret_cast<const FcChar8 *>(candidate));
        if (!pattern) {
            continue;
        }

        FcResult result = FcResultNoMatch;
        FcPattern *match = FcFontMatch(config, pattern, &result);
        FcPatternDestroy(pattern);

        if (match && result == FcResultMatch) {
            FcPatternDestroy(match);
            return candidate;
        }

        if (match) {
            FcPatternDestroy(match);
        }
    }

    return candidates.front() ? candidates.front() : "sans-serif";
}
} // namespace

void initialize_font_stack(ArolloaServer *server) {
    if (!server) {
        return;
    }

    server->primary_font = pick_font_family(SwissDesign::PRIMARY_FONT_CANDIDATES);
    server->secondary_font = pick_font_family(SwissDesign::SECONDARY_FONT_CANDIDATES);
    server->mono_font = pick_font_family(SwissDesign::MONO_FONT_CANDIDATES);

    if (!server->pango_layout) {
        return;
    }

    const std::string &primary = server->primary_font.empty() ? SwissDesign::PRIMARY_FONT : server->primary_font;
    std::string description = primary + " 10";

    PangoFontDescription *desc = pango_font_description_from_string(description.c_str());
    if (!desc) {
        return;
    }

    pango_layout_set_font_description(server->pango_layout, desc);
    pango_font_description_free(desc);
}
