// Copyright (c) 2022 The Avian Core developers
// Copyright (c) 2022 The Avian Core developers
// Copyright (c) 2022 Shafil Alam
// Copyright (c) 2025 The Soteria Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SOTERIA_SMARTCONTRACTS_H
#define SOTERIA_SMARTCONTRACTS_H

#include <string>
#include <vector>

/* Soteria SmartcontractsResult */
class SmartContractResult
{
public:
    const char* result;
    bool is_error = false;
};

/* Soteria Smartcontracts*/
class CSoteriaSmartContracts
{
public:
    static SmartContractResult RunFile(const char* file, const char* func, std::vector<std::string> args={});
    static std::vector<std::string> GetPlans();
};

#endif // SOTERIA_SMARTCONTRACTS_H
