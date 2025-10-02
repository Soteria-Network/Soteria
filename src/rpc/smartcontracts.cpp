// Copyright (c) 2022 The Avian Core developers
// Copyright (c) 2022 Shafil Alam
// Copyright (c) 2025 The Soteria Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "rpc/smartcontracts.h"
#include "smartcontracts/smartcontracts.h"
#include "rpc/server.h"
#include "validation.h"
#include "fs.h"
#include <util/system.h>
#include <stdexcept>
#include <stdint.h>
#include <univalue.h>
#include <string>
#include <vector>
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

UniValue call_smartcontract(const JSONRPCRequest& request)
{
    if (!AreSmartContractsDeployed())
        throw std::runtime_error(
            "Coming soon: Soteria smart contract function will be available in a future release.\n");

    if (request.fHelp || request.params.size() < 2)
        throw std::runtime_error(
            "call_smartcontract\n"
            "\nCall an Soteria smart contract function.\n"
            "\nArguments:\n"
            "1. smartcontract_name    (string, required) Lua file.\n"
            "2. function           (string, required) Lua function.\n"
            "3. args               (string, not needed) Lua args.\n"
            "\nResult:\n"
            "1.    (string) Result from called function\n"
            "\nExamples:\n" +
            HelpExampleCli("call_smartcontract", "\"social\" \"getLikes\"") + HelpExampleRpc("call_smartcontract", "\"social\" \"getLikes\""));

    LOCK(cs_main);
    // Build args vector and filename
    if (gArgs.IsArgSet("-smartcontracts")) {
        std::vector<std::string> args = {};
        std::string file = request.params[0].get_str() + ".lua";

        for (size_t i = 0; i < request.params.size(); i++) {
            args.push_back(request.params[i].get_str());
        }

        // Remove first two entries (smartcontract_name & function)
        args.erase(args.begin());
        args.erase(args.begin());
        
        // Ensure the directory exists ——
        fs::path smartDir = GetDataDir(false) / "smartcontracts";        
        if (!fs::exists(smartDir)) {
            fs::create_directories(smartDir);
        }   
        // Build the full path to <datadir>    
        fs::path path = smartDir / file; // Would use std::string overload so no need to use const char* 

        // Run the smart contract
        SmartContractResult result = CSoteriaSmartContracts::RunFile(path.string().c_str(), request.params[1].get_str().c_str(), args);
        // Check if the file actually exists
        if (fs::exists(path)) {
            if (result.is_error) {
                throw JSONRPCError(RPC_MISC_ERROR, result.result);
            } else {
                return result.result;
            }
        } else {
            throw JSONRPCError(RPC_MISC_ERROR, "Smart plan does not exist.");
        }
    } else {
        throw JSONRPCError(RPC_MISC_ERROR, "Smart Plans are experimental and prone to bugs. Please take precautions when using this feature. To enable, launch Soteria with the -smartcontracts flag.");
    }
}

UniValue list_smartcontracts(const JSONRPCRequest& request)
{
    if (!AreSmartContractsDeployed())
        throw std::runtime_error(
            "Coming soon: Soteria smart contract function will be available in a future release.\n");

    if (request.fHelp)
        throw std::runtime_error(
            "list_smartcontracts\n"
            "\nList soteria smart contracts.\n"
            "\nResult:\n"
            "[ smart contract name ]     (array) list of soteria smart contracts\n"
            "\nExamples:\n" +
            HelpExampleCli("list_smartcontracts", "") + HelpExampleRpc("list_smartcontracts", ""));

    LOCK(cs_main);

    if (gArgs.IsArgSet("-smartcontracts")) {
        UniValue plans(UniValue::VARR);
        for (const std::string& plan : CSoteriaSmartContracts::GetPlans()) {
            plans.push_back(plan);
        }
        return plans;
    } else {
        throw JSONRPCError(RPC_MISC_ERROR, "Smart Plans are experimental and prone to bugs. Please take precautions when using this feature. To enable, launch Soteria with the -smartcontracts flag.");
    }
}

static const CRPCCommand commands[] =
    { //  category              name                      actor (function)         argNames
      //  --------------------- ------------------------  -----------------------  ----------
        {"smartcontracts",         "callsmartcontract",        &call_smartcontract,        {"smartcontract_name", "function", "args"}},
        {"smartcontracts",         "listsmartcontracts",       &list_smartcontracts,       {}}
    };

void RegisterSmartContractRPCCommands(CRPCTable& t)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        t.appendCommand(commands[vcidx].name, &commands[vcidx]);
}
