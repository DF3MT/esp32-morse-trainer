#pragma once
#include <Arduino.h>
#include "i18n_strings.h"

// ============================================================
// Internationalisation (UI language)
// ------------------------------------------------------------
// Usage:  display.print(tr(Str::MENU_TITLE));
// The active language is a global setting (menu "Sprache/Language",
// stored in NVS). See i18n_strings.h for how to add a language.
// ============================================================

enum class Language : uint8_t {
    DE = 0,     // Deutsch
    EN,         // English
    COUNT
};

static constexpr uint8_t LANGUAGE_COUNT = (uint8_t)Language::COUNT;

#define I18N_ENUM_ENTRY(id, ...) id,
enum class Str : uint16_t {
    I18N_STRINGS(I18N_ENUM_ENTRY)
    COUNT
};
#undef I18N_ENUM_ENTRY

namespace I18n {
    void        setLanguage(Language lang);
    Language    language();
    const char* get(Str id);

    // Native display name, e.g. "Deutsch", "English"
    const char* languageName(Language lang);
    // ISO 639-1 code, e.g. "de", "en" (used by the web terminal)
    const char* languageCode(Language lang);
}

inline const char* tr(Str id) { return I18n::get(id); }
