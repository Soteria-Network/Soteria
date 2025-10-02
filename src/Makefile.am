# Copyright (c) 2011-2016 The Bitcoin Core developers
# Copyright (c) 2017-2019 The Raven Core developers
# Copyright (c) 2025 The Soteria Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

DIST_SUBDIRS = secp256k1 univalue

AM_LDFLAGS = $(PTHREAD_CFLAGS) $(LIBTOOL_LDFLAGS) $(HARDENED_LDFLAGS)
AM_CXXFLAGS = $(HARDENED_CXXFLAGS) $(ERROR_CXXFLAGS)
AM_CPPFLAGS = $(HARDENED_CPPFLAGS)
EXTRA_LIBRARIES =

if EMBEDDED_UNIVALUE
LIBUNIVALUE = univalue/libunivalue.la

$(LIBUNIVALUE): $(wildcard univalue/lib/*) $(wildcard univalue/include/*)
	$(AM_V_at)$(MAKE) $(AM_MAKEFLAGS) -C $(@D) $(@F)
else
LIBUNIVALUE = $(UNIVALUE_LIBS)
endif

SOTERIA_INCLUDES=-I$(builddir) -I$(builddir)/obj $(BDB_CPPFLAGS) $(BOOST_CPPFLAGS) $(LEVELDB_CPPFLAGS) $(CRYPTO_CFLAGS) $(SSL_CFLAGS)

SOTERIA_INCLUDES += -I$(srcdir)/secp256k1/include
SOTERIA_INCLUDES += $(UNIVALUE_CFLAGS)

LIBSOTERIA_SERVER=libsoteria_server.a
LIBSOTERIA_COMMON=libsoteria_common.a
LIBSOTERIA_CONSENSUS=libsoteria_consensus.a
LIBSOTERIA_CLI=libsoteria_cli.a
LIBSOTERIA_UTIL=libsoteria_util.a
LIBSOTERIA_CRYPTO=crypto/libsoteria_crypto.a
LIBSOTERIAQT=qt/libsoteriaqt.a
LIBSECP256K1=secp256k1/libsecp256k1.la

if ENABLE_ZMQ
LIBSOTERIA_ZMQ=libsoteria_zmq.a
endif
if BUILD_SOTERIA_LIBS
LIBSOTERIACONSENSUS=libsoteriaconsensus.la
endif
if ENABLE_WALLET
LIBSOTERIA_WALLET=libsoteria_wallet.a
endif

$(LIBSECP256K1): $(wildcard secp256k1/src/*) $(wildcard secp256k1/include/*)
	$(AM_V_at)$(MAKE) $(AM_MAKEFLAGS) -C $(@D) $(@F)

# Make is not made aware of per-object dependencies to avoid limiting building parallelization
# But to build the less dependent modules first, we manually select their order here:
EXTRA_LIBRARIES += \
  $(LIBSOTERIA_CRYPTO) \
  $(LIBSOTERIA_UTIL) \
  $(LIBSOTERIA_COMMON) \
  $(LIBSOTERIA_CONSENSUS) \
  $(LIBSOTERIA_SERVER) \
  $(LIBSOTERIA_CLI) \
  $(LIBSOTERIA_WALLET) \
  $(LIBSOTERIA_ZMQ)

lib_LTLIBRARIES = $(LIBSOTERIACONSENSUS)

bin_PROGRAMS =
noinst_PROGRAMS =
TESTS =
BENCHMARKS =

if BUILD_SOTERIAD
  bin_PROGRAMS += soteriad
endif

if BUILD_SOTERIA_CLI
  bin_PROGRAMS += soteria-cli
endif

if BUILD_SOTERIA_TX
  bin_PROGRAMS += soteria-tx
endif

.PHONY: FORCE check-symbols check-security
# soteria core #
SOTERIA_CORE_H = \
  addrdb.h \
  addressindex.h \
  algo/extra.c \
  algo/extra.h \
  algo/soterg/sph_groestl.h \
  algo/soterg/sph_types.h \
  algo/soterg/sph_blake.h \
  algo/soterg/sph_bmw.h \
  algo/soterg/sph_jh.h \
  algo/soterg/sph_keccak.h \
  algo/soterg/sph_skein.h \
  algo/soterg/sph_luffa.h \
  algo/soterg/sph_cubehash.h \
  algo/soterg/sph_simd.h \
  algo/soterg/sph_echo.h \
  algo/soterg/sph_shavite.h \
  algo/soterg/sph_sha2.h \
  algo/soterg/sph_haval.h \
  algo/soterg/sph_tiger.h \
  algo/soterg/lyra2.h \
  algo/soterg/sponge.h \
  algo/soterg/gost_streebog.h \
  algo/soterg/groestl.c \
  algo/soterg/blake.c \
  algo/soterg/bmw.c \
  algo/soterg/cubehash.c \
  algo/soterg/echo.c \
  algo/soterg/jh.c \
  algo/soterg/keccak.c \
  algo/soterg/luffa.c \
  algo/soterg/shavite.c \
  algo/soterg/simd.c \
  algo/soterg/skein.c \
  algo/soterg/hamsi.c \
  algo/soterg/whirlpool.c \
  algo/soterg/shabal.c \
  algo/soterg/fugue.c \
  algo/soterg/sha2.c \
  algo/soterg/sph_sha2big.c \
  algo/soterg/haval.c \
  algo/soterg/tiger.cpp \
  algo/soterg/lyra2.cpp \
  algo/soterg/sponge.cpp \
  algo/soterg/sph_sha2.c \
  algo/soterg/gost_streebog.c \
  algo/soterc/blake2.h \
  algo/soterc/blake2-impl.h \
  algo/soterc/blake2s-ref.c \
  algo/soterc/hashblake.h \
  algo/soterc/hashgroestl.h \
  algo/soterc/hashquark.h \
  algo/soterc/hashqubit.h \
  algo/soterc/hashskein.h \
  algo/soterc/hashx11.h \
  algo/soterc/hashx13.h \
  algo/soterc/hashx15.h \
  algo/soterc/hashx17.h \
  algo/soterc/soterc.h \
  algo/soterc/yespower/yespower.c \
  algo/soterc/yespower/crypto/sha256.c \
  spentindex.h \
  timestampindex.h \
  addrman.h \
  assets/assets.h \
  assets/assetdb.h \
  assets/assettypes.h \
  assets/messages.h \
  assets/myassetsdb.h \
  assets/restricteddb.h \
  assets/snapshotrequestdb.h \
  assets/assetsnapshotdb.h \
  assets/rewards.h \
  assets/sns.h \
  base58.h \
  bloom.h \
  blockencodings.h \
  chain.h \
  chainparams.h \
  chainparamsbase.h \
  chainparamsseeds.h \
  checkpoints.h \
  checkqueue.h \
  clientversion.h \
  coins.h \
  compat.h \
  compat/byteswap.h \
  compat/endian.h \
  compat/sanity.h \
  compressor.h \
  consensus/consensus.h \
  consensus/tx_verify.h \
  core_io.h \
  core_memusage.h \
  cuckoocache.h \
  flat-database.h \
  fs.h \
  httprpc.h \
  httpserver.h \
  indirectmap.h \
  init.h \
  key.h \
  keystore.h \
  dbwrapper.h \
  limitedmap.h \
  memusage.h \
  merkleblock.h \
  miner.h \
  net.h \
  net_processing.h \
  netaddress.h \
  netbase.h \
  netmessagemaker.h \
  noui.h \
  policy/feerate.h \
  policy/fees.h \
  policy/policy.h \
  policy/rbf.h \
  pow.h \
  protocol.h \
  random.h \
  reverse_iterator.h \
  reverselock.h \
  rpc/blockchain.h \
  rpc/smartcontracts.h \
  rpc/client.h \
  rpc/mining.h \
  rpc/protocol.h \
  rpc/safemode.h \
  rpc/server.h \
  rpc/register.h \
  scheduler.h \
  script/sigcache.h \
  script/sign.h \
  script/standard.h \
  script/ismine.h \
  streams.h \
  support/allocators/secure.h \
  support/allocators/zeroafterfree.h \
  support/cleanse.h \
  support/events.h \
  support/lockedpool.h \
  sync.h \
  threadsafety.h \
  threadinterrupt.h \
  timedata.h \
  torcontrol.h \
  txdb.h \
  txmempool.h \
  ui_interface.h \
  undo.h \
  unordered_lru_cache.h \
  util/system.h \
  util/moneystr.h \
  util/time.h \
  validation.h \
  validationinterface.h \
  versionbits.h \
  wallet/coincontrol.h \
  wallet/crypter.h \
  wallet/db.h \
  wallet/feebumper.h \
  wallet/fees.h \
  wallet/init.h \
  wallet/rpcwallet.h \
  wallet/wallet.h \
  wallet/walletdb.h \
  wallet/bip39.h \
  wallet/bip39_english.h \
  warnings.h \
  zmq/zmqabstractnotifier.h \
  zmq/zmqconfig.h\
  zmq/zmqnotificationinterface.h \
  zmq/zmqpublishnotifier.h \
  smartcontracts/soterialib.h \
  smartcontracts/smartcontracts.h \
  smartcontracts/json/strbuf.h \
  smartcontracts/json/fpconv.h \
  lua/lapi.h \
  lua/lauxlib.h \
  lua/lbaselib.h \
  lua/lcode.h \
  lua/lcorolib.h \
  lua/lctype.h \
  lua/ldblib.h \
  lua/ldebug.h \
  lua/ldo.h \
  lua/ldump.h \
  lua/lfunc.h \
  lua/lgc.h \
  lua/linit.h \
  lua/liolib.h \
  lua/llex.h \
  lua/lmathlib.h \
  lua/lmem.h \
  lua/loadlib.h \
  lua/lobject.h \
  lua/lopcodes.h \
  lua/loslib.h \
  lua/lparser.h \
  lua/lstate.h \
  lua/lstring.h \
  lua/lstrlib.h \
  lua/ltable.h \
  lua/ltablib.h \
  lua/ltm.h \
  lua/lundump.h \
  lua/lutf8lib.h \
  lua/lvm.h \
  lua/lzio.h

obj/build.h: FORCE
	@$(MKDIR_P) $(builddir)/obj
	@$(top_srcdir)/share/genbuild.sh "$(abs_top_builddir)/src/obj/build.h" \
	  "$(abs_top_srcdir)"
libsoteria_util_a-clientversion.$(OBJEXT): obj/build.h

# server: shared between soteriad and soteria-qt
libsoteria_server_a_CPPFLAGS = $(AM_CPPFLAGS) $(SOTERIA_INCLUDES) $(MINIUPNPC_CPPFLAGS) $(EVENT_CFLAGS) $(EVENT_PTHREADS_CFLAGS)
libsoteria_server_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libsoteria_server_a_SOURCES = \
  addrdb.cpp \
  addrman.cpp \
  bloom.cpp \
  blockencodings.cpp \
  chain.cpp \
  checkpoints.cpp \
  consensus/consensus.cpp \
  consensus/tx_verify.cpp \
  httprpc.cpp \
  httpserver.cpp \
  init.cpp \
  dbwrapper.cpp \
  merkleblock.cpp \
  miner.cpp \
  net.cpp \
  net_processing.cpp \
  noui.cpp \
  assets/assets.cpp \
  assets/assetdb.cpp \
  assets/assettypes.cpp \
  assets/messages.cpp \
  assets/myassetsdb.cpp \
  assets/restricteddb.cpp \
  assets/snapshotrequestdb.cpp \
  assets/assetsnapshotdb.cpp \
  assets/rewards.cpp \
  assets/sns.cpp \
  policy/fees.cpp \
  policy/policy.cpp \
  policy/rbf.cpp \
  pow.cpp \
  rest.cpp \
  rpc/smartcontracts.cpp \
  rpc/assets.cpp \
  rpc/blockchain.cpp \
  rpc/messages.cpp \
  rpc/mining.cpp \
  rpc/misc.cpp \
  rpc/net.cpp \
  rpc/rawtransaction.cpp \
  rpc/rewards.cpp \
  rpc/safemode.cpp \
  rpc/server.cpp \
  rpc/client.cpp \
  script/sigcache.cpp \
  script/ismine.cpp \
  timedata.cpp \
  torcontrol.cpp \
  txdb.cpp \
  txmempool.cpp \
  ui_interface.cpp \
  validation.cpp \
  validationinterface.cpp \
  versionbits.cpp \
  smartcontracts/soterialib.cpp \
  smartcontracts/smartcontracts.cpp \
  smartcontracts/json/strbuf.c \
  smartcontracts/json/fpconv.c \
  smartcontracts/json/lua_cjson.c \
  lua/lapi.c \
  lua/lauxlib.c \
  lua/lbaselib.c \
  lua/lcode.c \
  lua/lcorolib.c \
  lua/lctype.c \
  lua/ldblib.c \
  lua/ldebug.c \
  lua/ldo.c \
  lua/ldump.c \
  lua/lfunc.c \
  lua/lgc.c \
  lua/linit.c \
  lua/liolib.c \
  lua/llex.c \
  lua/lmathlib.c \
  lua/lmem.c \
  lua/loadlib.c \
  lua/lobject.c \
  lua/lopcodes.c \
  lua/loslib.c \
  lua/lparser.c \
  lua/lstate.c \
  lua/lstring.c \
  lua/lstrlib.c \
  lua/ltable.c \
  lua/ltablib.c \
  lua/ltm.c \
  lua/lundump.c \
  lua/lutf8lib.c \
  lua/lvm.c \
  lua/lzio.c \
  $(SOTERIA_CORE_H)

if ENABLE_ZMQ
libsoteria_zmq_a_CPPFLAGS = $(SOTERIA_INCLUDES) $(ZMQ_CFLAGS)
libsoteria_zmq_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libsoteria_zmq_a_SOURCES = \
  zmq/zmqabstractnotifier.cpp \
  zmq/zmqnotificationinterface.cpp \
  zmq/zmqpublishnotifier.cpp
endif


# wallet: shared between soteriad and soteria-qt, but only linked
# when wallet enabled
libsoteria_wallet_a_CPPFLAGS = $(AM_CPPFLAGS) $(SOTERIA_INCLUDES)
libsoteria_wallet_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libsoteria_wallet_a_SOURCES = \
  wallet/crypter.cpp \
  wallet/db.cpp \
  wallet/feebumper.cpp \
  wallet/fees.cpp \
  wallet/init.cpp \
  wallet/rpcdump.cpp \
  wallet/rpcwallet.cpp \
  wallet/wallet.cpp \
  wallet/walletdb.cpp \
  wallet/bip39.cpp \
  $(SOTERIA_CORE_H)

# crypto primitives library
crypto_libsoteria_crypto_a_CPPFLAGS = $(AM_CPPFLAGS)
crypto_libsoteria_crypto_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
crypto_libsoteria_crypto_a_SOURCES = \
  crypto/aes.cpp \
  crypto/aes.h \
  crypto/chacha20.h \
  crypto/chacha20.cpp \
  crypto/common.h \
  crypto/hmac_sha256.cpp \
  crypto/hmac_sha256.h \
  crypto/hmac_sha512.cpp \
  crypto/hmac_sha512.h \
  crypto/ripemd160.cpp \
  crypto/sha1.cpp \
  crypto/sha1.h \
  crypto/sha256.cpp \
  crypto/sha256.h \
  crypto/sha512.h \
  crypto/sha512.cpp \
  crypto/ethash/include/ethash/ethash.h \
  crypto/ethash/include/ethash/ethash.hpp \
  crypto/ethash/include/ethash/hash_types.h \
  crypto/ethash/include/ethash/hash_types.hpp \
  crypto/ethash/include/ethash/keccak.h \
  crypto/ethash/include/ethash/keccak.hpp \
  crypto/ethash/include/ethash/progpow.hpp \
  crypto/ethash/include/ethash/version.h \
  crypto/ethash/lib/ethash/bit_manipulation.h \
  crypto/ethash/lib/ethash/builtins.h \
  crypto/ethash/lib/ethash/endianness.hpp \
  crypto/ethash/lib/ethash/ethash.cpp \
  crypto/ethash/lib/ethash/ethash-internal.hpp \
  crypto/ethash/lib/ethash/kiss99.hpp \
  crypto/ethash/lib/ethash/managed.cpp \
  crypto/ethash/lib/ethash/primes.c \
  crypto/ethash/lib/ethash/primes.h \
  crypto/ethash/lib/ethash/progpow.cpp \
  crypto/ethash/lib/keccak/keccak.c \
  crypto/ethash/lib/keccak/keccakf1600.c \
  crypto/ethash/lib/keccak/keccakf800.c \
  crypto/ethash/lib/support/attributes.h \
  crypto/ethash/helpers.hpp \
  crypto/ethash/progpow_test_vectors.hpp

if USE_ASM
crypto_libsoteria_crypto_a_SOURCES += crypto/sha256_sse4.cpp
endif

# consensus: shared between all executables that validate any consensus rules.
libsoteria_consensus_a_CPPFLAGS = $(AM_CPPFLAGS) $(SOTERIA_INCLUDES)
libsoteria_consensus_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libsoteria_consensus_a_SOURCES = \
  amount.h \
  founder_payment.h \
  arith_uint256.cpp \
  arith_uint256.h \
  consensus/merkle.cpp \
  consensus/merkle.h \
  consensus/params.h \
  consensus/validation.h \
  hash.cpp \
  hash.h \
  prevector.h \
  primitives/block.cpp \
  primitives/block.h \
  primitives/powcache.cpp \
  primitives/powcache.h \
  primitives/transaction.cpp \
  primitives/transaction.h \
  pubkey.cpp \
  pubkey.h \
  script/soteriaconsensus.cpp \
  script/interpreter.cpp \
  script/interpreter.h \
  script/script.cpp \
  script/script.h \
  script/script_error.cpp \
  script/script_error.h \
  serialize.h \
  span.h \
  support/cleanse.h \
  support/cleanse.cpp \
  tinyformat.h \
  uint256.cpp \
  uint256.h \
  util/strencodings.cpp \
  util/strencodings.h \
  version.h \
  LibBoolEE.cpp \
  LibBoolEE.h \
  $(SOTERIA_CORE_H)

# common: shared between soteriad, and soteria-qt and non-server tools
libsoteria_common_a_CPPFLAGS = $(AM_CPPFLAGS) $(SOTERIA_INCLUDES)
libsoteria_common_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libsoteria_common_a_SOURCES = \
  base58.cpp \
  founder_payment.cpp \
  chainparams.cpp \
  coins.cpp \
  compressor.cpp \
  core_read.cpp \
  core_write.cpp \
  key.cpp \
  keystore.cpp \
  netaddress.cpp \
  netbase.cpp \
  policy/feerate.cpp \
  protocol.cpp \
  scheduler.cpp \
  script/sign.cpp \
  script/standard.cpp \
  warnings.cpp \
  $(SOTERIA_CORE_H)

# util: shared between all executables.
# This library *must* be included to make sure that the glibc
# backward-compatibility objects and their sanity checks are linked.
libsoteria_util_a_CPPFLAGS = $(AM_CPPFLAGS) $(SOTERIA_INCLUDES)
libsoteria_util_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libsoteria_util_a_SOURCES = \
  support/lockedpool.cpp \
  chainparamsbase.cpp \
  clientversion.cpp \
  compat/glibc_sanity.cpp \
  compat/glibcxx_sanity.cpp \
  fs.cpp \
  random.cpp \
  rpc/protocol.cpp \
  support/cleanse.cpp \
  sync.cpp \
  threadinterrupt.cpp \
  util/system.cpp \
  util/moneystr.cpp \
  util/strencodings.cpp \
  util/time.cpp \
  $(SOTERIA_CORE_H)

if GLIBC_BACK_COMPAT
libsoteria_util_a_SOURCES += compat/glibc_compat.cpp
AM_LDFLAGS += $(COMPAT_LDFLAGS)
endif

# cli: shared between soteria-cli and soteria-qt
libsoteria_cli_a_CPPFLAGS = $(AM_CPPFLAGS) $(SOTERIA_INCLUDES)
libsoteria_cli_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libsoteria_cli_a_SOURCES = \
  rpc/client.cpp \
  $(SOTERIA_CORE_H)

nodist_libsoteria_util_a_SOURCES = $(srcdir)/obj/build.h
#

# soteriad binary #
soteriad_SOURCES = soteriad.cpp
soteriad_CPPFLAGS = $(AM_CPPFLAGS) $(SOTERIA_INCLUDES)
soteriad_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
soteriad_LDFLAGS = $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS)

if TARGET_WINDOWS
soteriad_SOURCES += soteriad-res.rc
endif

soteriad_LDADD = \
  $(LIBSOTERIA_SERVER) \
  $(LIBSOTERIA_COMMON) \
  $(LIBUNIVALUE) \
  $(LIBSOTERIA_UTIL) \
  $(LIBSOTERIA_WALLET) \
  $(LIBSOTERIA_ZMQ) \
  $(LIBSOTERIA_CONSENSUS) \
  $(LIBSOTERIA_CRYPTO) \
  $(LIBLEVELDB) \
  $(LIBLEVELDB_SSE42) \
  $(LIBMEMENV) \
  $(LIBSECP256K1)

soteriad_LDADD += $(BOOST_LIBS) $(BDB_LIBS) $(SSL_LIBS) $(CRYPTO_LIBS) $(MINIUPNPC_LIBS) $(EVENT_PTHREADS_LIBS) $(EVENT_LIBS) $(ZMQ_LIBS)

# soteria-cli binary #
soteria_cli_SOURCES = soteria-cli.cpp
soteria_cli_CPPFLAGS = $(AM_CPPFLAGS) $(SOTERIA_INCLUDES) $(EVENT_CFLAGS)
soteria_cli_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
soteria_cli_LDFLAGS = $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS)

if TARGET_WINDOWS
soteria_cli_SOURCES += soteria-cli-res.rc
endif

soteria_cli_LDADD = \
  $(LIBSOTERIA_CLI) \
  $(LIBUNIVALUE) \
  $(LIBSOTERIA_UTIL) \
  $(LIBSOTERIA_CRYPTO)

soteria_cli_LDADD += $(BOOST_LIBS) $(SSL_LIBS) $(CRYPTO_LIBS) $(EVENT_LIBS)
#

# soteria-tx binary #
soteria_tx_SOURCES = soteria-tx.cpp
soteria_tx_CPPFLAGS = $(AM_CPPFLAGS) $(SOTERIA_INCLUDES)
soteria_tx_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
soteria_tx_LDFLAGS = $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS)

if TARGET_WINDOWS
soteria_tx_SOURCES += soteria-tx-res.rc
endif

soteria_tx_LDADD = \
  $(LIBUNIVALUE) \
  $(LIBSOTERIA_COMMON) \
  $(LIBSOTERIA_UTIL) \
  $(LIBSOTERIA_CONSENSUS) \
  $(LIBSOTERIA_CRYPTO) \
  $(LIBSECP256K1)

soteria_tx_LDADD += $(BOOST_LIBS) $(CRYPTO_LIBS)
#

# soteriaconsensus library #
if BUILD_SOTERIA_LIBS
include_HEADERS = script/soteriaconsensus.h
libsoteriaconsensus_la_SOURCES = $(crypto_libsoteria_crypto_a_SOURCES) $(libsoteria_consensus_a_SOURCES)

if GLIBC_BACK_COMPAT
  libsoteriaconsensus_la_SOURCES += compat/glibc_compat.cpp
endif

libsoteriaconsensus_la_LDFLAGS = $(AM_LDFLAGS) -no-undefined $(RELDFLAGS)
libsoteriaconsensus_la_LIBADD = $(LIBSECP256K1) $(BOOST_LIBS)
libsoteriaconsensus_la_CPPFLAGS = $(AM_CPPFLAGS) -I$(builddir)/obj -I$(srcdir)/secp256k1/include -DBUILD_SOTERIA_INTERNAL
libsoteriaconsensus_la_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)

endif
#

CTAES_DIST =  crypto/ctaes/bench.c
CTAES_DIST += crypto/ctaes/ctaes.c
CTAES_DIST += crypto/ctaes/ctaes.h
CTAES_DIST += crypto/ctaes/README.md
CTAES_DIST += crypto/ctaes/test.c

CLEANFILES = $(EXTRA_LIBRARIES)

CLEANFILES += *.gcda *.gcno
CLEANFILES += compat/*.gcda compat/*.gcno
CLEANFILES += consensus/*.gcda consensus/*.gcno
CLEANFILES += crypto/*.gcda crypto/*.gcno
CLEANFILES += policy/*.gcda policy/*.gcno
CLEANFILES += primitives/*.gcda primitives/*.gcno
CLEANFILES += script/*.gcda script/*.gcno
CLEANFILES += support/*.gcda support/*.gcno
CLEANFILES += univalue/*.gcda univalue/*.gcno
CLEANFILES += wallet/*.gcda wallet/*.gcno
CLEANFILES += wallet/test/*.gcda wallet/test/*.gcno
CLEANFILES += zmq/*.gcda zmq/*.gcno
CLEANFILES += obj/build.h

EXTRA_DIST = $(CTAES_DIST)


config/soteria-config.h: config/stamp-h1
	@$(MAKE) -C $(top_builddir) $(subdir)/$(@)
config/stamp-h1: $(top_srcdir)/$(subdir)/config/soteria-config.h.in $(top_builddir)/config.status
	$(AM_V_at)$(MAKE) -C $(top_builddir) $(subdir)/$(@)
$(top_srcdir)/$(subdir)/config/soteria-config.h.in:  $(am__configure_deps)
	$(AM_V_at)$(MAKE) -C $(top_srcdir) $(subdir)/config/soteria-config.h.in

clean-local:
	-$(MAKE) -C secp256k1 clean
	-$(MAKE) -C univalue clean
	-rm -f leveldb/*/*.gcda leveldb/*/*.gcno leveldb/helpers/memenv/*.gcda leveldb/helpers/memenv/*.gcno
	-rm -f config.h
	-rm -rf test/__pycache__

.rc.o:
	@test -f $(WINDRES)
	## FIXME: How to get the appropriate modulename_CPPFLAGS in here?
	$(AM_V_GEN) $(WINDRES) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(CPPFLAGS) -DWINDRES_PREPROC -i $< -o $@

check-symbols: $(bin_PROGRAMS)
if GLIBC_BACK_COMPAT
	@echo "Checking glibc back compat..."
	$(AM_V_at) READELF=$(READELF) CPPFILT=$(CPPFILT) $(top_srcdir)/contrib/devtools/symbol-check.py < $(bin_PROGRAMS)
endif

check-security: $(bin_PROGRAMS)
if HARDEN
	@echo "Checking binary security..."
	$(AM_V_at) READELF=$(READELF) OBJDUMP=$(OBJDUMP) $(top_srcdir)/contrib/devtools/security-check.py < $(bin_PROGRAMS)
endif

%.pb.cc %.pb.h: %.proto
	@test -f $(PROTOC)
	$(AM_V_GEN) $(PROTOC) --cpp_out=$(@D) --proto_path=$(<D) $<

if EMBEDDED_LEVELDB
include Makefile.leveldb.include
endif

if ENABLE_TESTS
include Makefile.test.include
endif

if ENABLE_BENCH
include Makefile.bench.include
endif

if ENABLE_QT
include Makefile.qt.include
endif

if ENABLE_QT_TESTS
include Makefile.qttest.include
endif
