// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2011-2017 The Bitcoin Core developers
// Copyright (c) 2017-2020 The Raven Core developers
// Copyright (c) 2025 The Soteria Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "ui_interface.h"
#include <util/system.h>
#include <string>

CClientUIInterface uiInterface;

bool InitError(const std::string& str)
{
    uiInterface.ThreadSafeMessageBox(str, "", CClientUIInterface::MSG_ERROR);
    return false;
}

void InitWarning(const std::string& str)
{
    uiInterface.ThreadSafeMessageBox(str, "", CClientUIInterface::MSG_WARNING);
}

std::string AmountHighWarn(const std::string& optname)
{
    return strprintf(_("%s is set very high!"), optname);
}

std::string AmountErrMsg(const char* const optname, const std::string& strValue)
{
    return strprintf(_("Invalid amount for -%s=<amount>: '%s'"), optname, strValue);
}
