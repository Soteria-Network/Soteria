// Copyright (c) 2011-2016 The Bitcoin Core developers
// Copyright (c) 2017-2019 The Raven Core developers
// Copyright (c) 2025 The Soteria Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "validation.h"

#ifndef SOTERIA_QT_GUICONSTANTS_H
#define SOTERIA_QT_GUICONSTANTS_H

/* Milliseconds between model updates */
static const int MODEL_UPDATE_DELAY = 250;

/* AskPassphraseDialog -- Maximum passphrase length */
static const int MAX_PASSPHRASE_SIZE = 1024;

/* SoteriaGUI -- Size of icons in status bar */
static const int STATUSBAR_ICONSIZE = 20;

static const bool DEFAULT_SPLASHSCREEN = true;

/* Invalid field background style */
//#define STYLE_INVALID "border: 1px solid red; padding: 0px; background: transparent;"
#define STYLE_INVALID "background:#FF8080"
#define STYLE_VALID "border: 1px solid lightgray; padding: 0px; background: transparent;"

/* Transaction list -- unconfirmed transaction */
#define COLOR_UNCONFIRMED QColor(128, 128, 128)
/* Transaction list -- negative amount */
#define COLOR_NEGATIVE QColor(255, 0, 0)
/* Transaction list -- bare address (without label) */
#define COLOR_BAREADDRESS QColor(140, 140, 140)
/* Transaction list -- TX status decoration - open until date */
#define COLOR_TX_STATUS_OPENUNTILDATE QColor(64, 64, 255)
/* Transaction list -- TX status decoration - danger, tx needs attention */
#define COLOR_TX_STATUS_DANGER QColor(200, 100, 100)
/* Transaction list -- TX status decoration - default color */
#define COLOR_BLACK QColor(0, 0, 0)
/* Widget Background color - default color */
#define COLOR_WHITE QColor(255, 255, 255)

#define COLOR_SOTERIA_2B737F QColor("#2B737F")
#define COLOR_SOTERIA_19827B QColor("#19827B")
#define COLOR_SOTERIA_18A7B7 QColor("#18A7B7")
#define COLOR_SOTERIA_34E2D6 QColor("#34E2D6")


#define COLOR_WALLETFRAME_SHADOW QColor(0,0,0,71)

/* Color of labels */
#define COLOR_LABELS QColor("#FFFFFF")

/** LIGHT MODE */
/* Background color, very light gray */
#define COLOR_BACKGROUND_LIGHT QColor("#ffffff")
/* Widget background color */
#define COLOR_WIDGET_BACKGROUND QColor("#ffffff")
/* Soteria dark orange */
#define COLOR_DARK_ORANGE QColor("#f05339")
/* Soteria light orange */
#define COLOR_LIGHT_ORANGE QColor("#f79433")
/* Soteria dark blue - Sidedar start */
#define COLOR_DARK_BLUE QColor("#3d3d3d") // #08fcf0
/* Soteria light blue - Sidebar end */
#define COLOR_LIGHT_BLUE QColor("#3d3d3d")
/* Soteria dark green - Asset */
//#define COLOR_DARK_GREEN QColor("#2a737f")
#define COLOR_DARK_GREEN QColor("#03d9fc")

/* Soteria light green - Asset */
//#define COLOR_LIGHT_GREEN QColor("#34f5c6")
#define COLOR_LIGHT_GREEN QColor("#05ffc2")

/* Soteria asset text */
#define COLOR_ASSET_TEXT QColor(255, 255, 255)
// #define COLOR_ASSET_TEXT QColor(#34f5c6)
/* Soteria shadow color - light mode */
#define COLOR_SHADOW_LIGHT QColor("#cacaca")
/* Toolbar not selected text color */
//#define COLOR_TOOLBAR_NOT_SELECTED_TEXT QColor("#e8e6e6")
#define COLOR_TOOLBAR_NOT_SELECTED_TEXT QColor("#03fcad")
/* Toolbar selected text color */
//#define COLOR_TOOLBAR_SELECTED_TEXT QColor("#000000")
#define COLOR_TOOLBAR_SELECTED_TEXT QColor("#02ddfa")
/* Send entries background color */
//#define COLOR_SENDENTRIES_BACKGROUND QColor("#ffffff")
#define COLOR_SENDENTRIES_BACKGROUND QColor("#c9c5c5")

/** DARK MODE */
/* Widget background color, dark mode */
#define COLOR_WIDGET_BACKGROUND_DARK QColor("#2E2E2E")
/* Soteria shadow color - dark mode */
#define COLOR_SHADOW_DARK QColor("#1B1B1B")
/* Soteria Light blue - dark mode - dark mode */
#define COLOR_LIGHT_BLUE_DARK QColor("#1D1D1D")
/* Soteria Dark blue - dark mode - dark mode */
#define COLOR_DARK_BLUE_DARK QColor("#1D1D1D")
/* Pricing widget background color */
#define COLOR_PRICING_WIDGET QColor("#2E2E2E")
/* Soteria dark mode administrator background color */
#define COLOR_ADMIN_CARD_DARK COLOR_BLACK
/* Soteria dark mode regular asset background color */
#define COLOR_REGULAR_CARD_DARK_BLUE_DARK_MODE QColor("#2E2E2E")
/* Soteria dark mode regular asset background color */
#define COLOR_REGULAR_CARD_LIGHT_BLUE_DARK_MODE QColor("#2E2E2E")
/* Toolbar not selected text color */
#define COLOR_TOOLBAR_NOT_SELECTED_TEXT_DARK_MODE QColor("#03fcb1") 
/* Toolbar selected text color */
#define COLOR_TOOLBAR_SELECTED_TEXT_DARK_MODE QColor("#ffffff")
/* Send entries background color dark mode */
#define COLOR_SENDENTRIES_BACKGROUND_DARK QColor("#2E2E2E")

/* Soteria label color as a string */
#define STRING_LABEL_COLOR "color: #000000"
#define STRING_LABEL_COLOR_WHITE "color: #FFFFFF"
//#define STRING_LABEL_COLOR_WHITE "color: #07facd"
#define STRING_LABEL_COLOR_WARNING "color: #ff6666"

/* Tooltips longer than this (in characters) are converted into rich text,
   so that they can be word-wrapped.
 */
static const int TOOLTIP_WRAP_THRESHOLD = 80;

/* Maximum allowed URI length */
static const int MAX_URI_LENGTH = 255;

/* QRCodeDialog -- size of exported QR Code image */
#define QR_IMAGE_SIZE 300

/* Number of frames in spinner animation */
#define SPINNER_FRAMES 36

#define QAPP_ORG_NAME "Soteria"
#define QAPP_ORG_DOMAIN "soteria-network.site"
#define QAPP_APP_NAME_DEFAULT "Soteria-Qt"
#define QAPP_APP_NAME_TESTNET "Soteria-Qt-testnet"
#define QAPP_APP_NAME_REGTEST "Soteria-Qt-regtest"

/* Default third party browser urls */
#define DEFAULT_THIRD_PARTY_BROWSERS "https://explorer.soteria-network.site/tx/?txid=%s|https://blockexplorer.soteria-network.site/tx/%s"
//#define DEFAULT_THIRD_PARTY_BROWSERS "https://explorer-eu.soteria-network.site/tx/%s|https://explorer-us.soteria-network.site/tx/%s"
/* Default IPFS viewer */
#define DEFAULT_IPFS_VIEWER "https://ipfs.io/ipfs/%s"

/* One gigabyte (GB) in bytes */
static constexpr uint64_t GB_BYTES{1000000000};

/**
 * Convert configured prune target bytes to displayed GB. Round up to avoid underestimating max disk usage.
 */
constexpr inline int PruneBytestoGB(uint64_t bytes) { return (bytes + GB_BYTES - 1) / GB_BYTES; }

/**
 * Convert displayed prune target GB to configured MiB. Round down so roundtrip GB -> MiB -> GB conversion is stable.
 */
constexpr inline int64_t PruneGBtoMiB(int gb) { return gb * GB_BYTES / 1024 / 1024; }

// Default prune target displayed in GUI.
static constexpr int DEFAULT_PRUNE_TARGET_GB{PruneBytestoGB(MIN_DISK_SPACE_FOR_BLOCK_FILES)};

#endif // SOTERIA_QT_GUICONSTANTS_H
