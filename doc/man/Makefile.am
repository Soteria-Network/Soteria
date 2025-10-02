dist_man1_MANS=

if BUILD_SOTERIAD
  dist_man1_MANS+=soteriad.1
endif

if ENABLE_QT
  dist_man1_MANS+=soteria-qt.1
endif

if BUILD_SOTERIA_CLI
  dist_man1_MANS+=soteria-cli.1
endif

if BUILD_SOTERIA_TX
  dist_man1_MANS+=soteria-tx.1
endif

# Add regeneration rules (optional)
%.1: %
	help2man --no-discard-stderr $< > $@
