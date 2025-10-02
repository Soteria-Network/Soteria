// Copyright (c) 2022 The Avian Core developers
// Copyright (c) 2022 Shafil Alam
// Copyright (c) 2025 The Soteria Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SOTERIA_SMARTCONTRACTS_SOTERIALIB_H
#define SOTERIA_SMARTCONTRACTS_SOTERIALIB_H

#include "lua/lua.hpp"
#include <string>

/* Run RPC commands */
bool RPCParse(std::string& strResult, const std::string& strCommand, const bool fExecute, std::string* const pstrFilteredOut);

/* Lua */
void register_soterialib(lua_State* L);

#endif
