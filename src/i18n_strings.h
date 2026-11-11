#pragma once
// ============================================================
// UI string table — one row per text, one column per language
// ------------------------------------------------------------
// Column order MUST match enum class Language in i18n.h.
// Adding a language: add a column to EVERY row (the compiler
// reports any row that is missing a translation), then extend
// Language, LANGUAGE_NAMES and LANGUAGE_CODES in i18n.h/.cpp.
//
// The TFT fonts only contain ASCII: write umlauts as ae/oe/ue/ss.
// Strings containing printf placeholders must keep the same
// placeholders (same order, same types) in every language.
// ============================================================

//  X(ID,                 Deutsch (de),                                    English (en))
#define I18N_STRINGS(X) \
    /* --- Splash / standby --- */ \
    X(SPLASH_PROMPT,      "[ Taste oder Paddle druecken ]",                "[ Press knob or paddle ]") \
    X(SLEEP_TITLE_BAR,    "STANDBY-MODUS",                                 "STANDBY MODE") \
    X(SLEEP_HINT,         "Drehknopf druecken zum Aufwecken",              "Press the knob to wake up") \
    /* --- Units --- */ \
    X(UNIT_CPM,           "BpM",                                           "CPM") \
    X(STATUS_SPEED_FMT,   "%dW/%dB",                                       "%dW/%dC") \
    /* --- Decode screen --- */ \
    X(DECODE_HINT,        "DREHEN: WPM/BpM  |  KLICK: MENUE",              "TURN: WPM/CPM  |  CLICK: MENU") \
    /* --- Training screen --- */ \
    X(TRAIN_SEND,         "Gib:",                                          "Send:") \
    X(TRAIN_CORRECT,      "RICHTIG!",                                      "CORRECT!") \
    X(TRAIN_WRONG,        "FALSCH ",                                       "WRONG  ") \
    X(TRAIN_SENT_PREFIX,  "  (gesendet: ",                                 "  (sent: ") \
    X(TRAIN_HINT,         "KLICK: Menue | DREHEN: Tempo anpassen",         "CLICK: Menu | TURN: Adjust speed") \
    /* --- Menu screen --- */ \
    X(MENU_TITLE,         "EINSTELLUNGEN",                                 "SETTINGS") \
    X(MENU_HINT_EDIT,     "DREHEN: Wert anpassen  |  KLICK: Speichern",    "TURN: Adjust value  |  CLICK: Save") \
    X(MENU_HINT_BROWSE,   "DREHEN: Menue  |  KLICK: Waehlen/Aendern",      "TURN: Scroll  |  CLICK: Select/Change") \
    /* --- Score screen --- */ \
    X(SCORE_TITLE,        "ERGEBNIS",                                      "RESULT") \
    X(SCORE_CORRECT_FMT,  "%d / %d richtig",                               "%d / %d correct") \
    X(SCORE_AVG_FMT,      "Durchschnitt: %d WPM (%d BpM)",                 "Average: %d WPM (%d CPM)") \
    /* --- Menu items --- */ \
    X(M_LANGUAGE,         "Sprache",                                       "Language") \
    X(M_MODE,             "Betriebsart",                                   "Mode") \
    X(V_DECODE,           "Decodieren",                                    "Decode") \
    X(V_TRAINING,         "Training",                                      "Training") \
    X(M_SPEED,            "Tempo",                                         "Speed") \
    X(V_SPEED_FMT,        "%d WPM / %d BpM",                               "%d WPM / %d CPM") \
    X(M_AUTOSPEED,        "Auto-Tempo",                                    "Auto speed") \
    X(V_ON,               "AN",                                            "ON") \
    X(V_OFF,              "AUS",                                           "OFF") \
    X(M_KEYER_MODE,       "Keyer-Modus",                                   "Keyer mode") \
    X(V_MODE_A,           "Modus A",                                       "Mode A") \
    X(V_MODE_B,           "Modus B",                                       "Mode B") \
    X(M_PADDLE_SWAP,      "Paddle-Tausch",                                 "Paddle swap") \
    X(V_SWAPPED,          "Getauscht",                                     "Swapped") \
    X(V_NORMAL,           "Normal",                                        "Normal") \
    X(M_TONE,             "Tonhoehe",                                      "Tone pitch") \
    X(M_VOLUME,           "Lautstaerke",                                   "Volume") \
    X(M_BRIGHTNESS,       "Helligkeit",                                    "Brightness") \
    X(M_BLE_KEYBOARD,     "BLE-Tastatur",                                  "BLE keyboard") \
    X(V_BLE_OFF,          "Aus",                                           "Off") \
    X(V_BLE_CONNECTED,    "Verbunden",                                     "Connected") \
    X(V_BLE_WAITING,      "Wartet...",                                     "Waiting...") \
    X(M_BLE_LAYOUT,       "BLE-Layout",                                    "BLE layout") \
    X(M_BLE_BONDS,        "BT Paarung",                                    "BT pairing") \
    X(V_BONDS_CLEARED,    "Geloescht!",                                    "Cleared!") \
    X(V_BONDS_RESET,      "Klick=Reset",                                   "Click=Reset") \
    X(M_WIFI,             "WLAN SoftAP",                                   "Wi-Fi SoftAP") \
    X(M_TRAIN_SET,        "Uebungssatz",                                   "Training set") \
    X(V_SET_ALL,          "Alle",                                          "All") \
    X(V_SET_QSO,          "QSO-Abk.",                                      "QSO abbr.") \
    X(V_SET_PROSIGNS,     "Spruchz.",                                      "Prosigns") \
    X(M_KOCH_LEVEL,       "Koch-Stufe",                                    "Koch level") \
    X(V_LEVEL_FMT,        "Stufe %d",                                      "Level %d") \
    X(M_SLEEP,            "Auto-Standby",                                  "Auto standby") \
    X(V_SLEEP_OFF,        "Aus",                                           "Off") \
    X(V_MINUTES_FMT,      "%d Min",                                        "%d min") \
    X(M_BACK,             "< Zurueck",                                     "< Back")
