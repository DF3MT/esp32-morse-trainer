#include "i18n.h"

// Each X(...) row expands to one { "de", "en", ... } entry. A row with
// the wrong number of translations fails to compile (static_assert below).
#define I18N_TABLE_ROW(id, ...) { __VA_ARGS__ },
static const char* const STRING_TABLE[(size_t)Str::COUNT][LANGUAGE_COUNT] = {
    I18N_STRINGS(I18N_TABLE_ROW)
};
#undef I18N_TABLE_ROW

template <typename... T>
static constexpr size_t countTranslations(T...) { return sizeof...(T); }

#define I18N_COUNT_ARGS(id, ...) \
    static_assert(countTranslations(__VA_ARGS__) == LANGUAGE_COUNT, \
                  "i18n_strings.h: row " #id " has the wrong number of translations");
I18N_STRINGS(I18N_COUNT_ARGS)
#undef I18N_COUNT_ARGS

static const char* const LANGUAGE_NAMES[LANGUAGE_COUNT] = { "Deutsch", "English" };
static const char* const LANGUAGE_CODES[LANGUAGE_COUNT] = { "de", "en" };

static Language _language = Language::DE;

namespace I18n {

void setLanguage(Language lang) {
    if ((uint8_t)lang >= LANGUAGE_COUNT) lang = Language::DE;
    _language = lang;
}

Language language() {
    return _language;
}

const char* get(Str id) {
    if ((size_t)id >= (size_t)Str::COUNT) return "";
    return STRING_TABLE[(size_t)id][(uint8_t)_language];
}

const char* languageName(Language lang) {
    if ((uint8_t)lang >= LANGUAGE_COUNT) return "?";
    return LANGUAGE_NAMES[(uint8_t)lang];
}

const char* languageCode(Language lang) {
    if ((uint8_t)lang >= LANGUAGE_COUNT) return "de";
    return LANGUAGE_CODES[(uint8_t)lang];
}

} // namespace I18n
