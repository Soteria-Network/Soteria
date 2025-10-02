// Copyright (c) 2022 Shafil Alam
// Copyright (c) 2025 The Soteria Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "sns.h"

#include <iostream>
#include <string>
#include <sstream>

#include "string.h"

#include "univalue.h"
#include <util/system.h>
#include "util/strencodings.h"
#include "base58.h"
#include "script/standard.h"

#include "boost/asio.hpp"

using namespace boost::asio::ip;

/** Static prefix from SNS IDs */
const std::string CSoteriaNameSystem::prefix = "SNS";

/** Static domain */
const std::string CSoteriaNameSystem::domain = ".SOTER";

static std::string IPv4ToHex(std::string strIPv4)
{
    boost::system::error_code error;
    auto ip = address_v4::from_string(strIPv4, error);
    if (error) return "0";

    std::stringstream ss;
    ss << std::hex << ip.to_ulong();
    return ss.str();
}

static std::string HexToIPv4(std::string hexIPv4)
{
    if(!IsHexNumber(hexIPv4)) return "0.0.0.0";

    unsigned int hex = std::stoul(hexIPv4, 0, 16);
    auto ip = address_v4(hex);
    return ip.to_string();
}

bool CSoteriaNameSystem::CheckIPv4(std::string rawipv4, bool isHex) {
     if (isHex) rawipv4 = HexToIPv4(rawipv4);

    boost::system::error_code error;
    address_v4::from_string(rawipv4, error);

    if (error) return false;
    else return true;
}

// TODO: Add error result?
bool CSoteriaNameSystem::CheckTypeData(Type type, std::string typeData)
{
    switch (type) {
        case ADDR: {
            CTxDestination destination = DecodeDestination(typeData);
            if (!IsValidDestination(destination)) return false;
            break;
        }
        case IPv4: {
            if (!CheckIPv4(typeData, true)) return false;
            break;
        }
        default: {
            /** Unknown type */ 
            return false;
            break;
        }
    }
    return true;
}

std::string CSoteriaNameSystem::FormatTypeData(Type type, std::string typeData, std::string& error)
{
    // By default we just echo back the input
    std::string returnStr = typeData;

    // Check and set type data
    switch (type) {
        case ADDR: {
            CTxDestination destination = DecodeDestination(typeData);
            if (!IsValidDestination(destination)) {
                error = (typeData != "") 
                ? std::string("Invalid Soteria address: ") + typeData 
                : std::string("Empty Soteria address.");
                return "";
                   }
             break;
        }
        case IPv4: {
            if (!CheckIPv4(typeData, false)) {
                error = (typeData != "") 
                ? std::string("Invalid IPv4 address: ") + typeData 
                : std::string("Empty IPv4 addresss.");
                return "";
            }
            returnStr = IPv4ToHex(typeData);
            break;
        }
        default: {
            // Catches any Type enum value you didn’t explicitly handle above.
            error = "Unknown SNS type " +
                    std::to_string(static_cast<int>(type));
                    return "";
                }
          }
    return returnStr;
}

bool CSoteriaNameSystem::IsValidID(std::string ansID) {
    // Check for min length
    if(ansID.length() <= prefix.size() + 1) return false;

    // Check for prefix
    bool hasPrefix = (ansID.substr(0, CSoteriaNameSystem::prefix.length()) == CSoteriaNameSystem::prefix) && (ansID.size() <= 64);
    if (!hasPrefix) return false;

    // Must be valid hex number
    std::string hexStr = ansID.substr(prefix.length(), 1);
    if (!IsHexNumber(hexStr)) return false;

    // Check type
    int hexInt = stoi(hexStr, 0, 16);
    Type type = static_cast<Type>(hexInt);
    std::string rawData = ansID.substr(prefix.length() + 1);

    if (!CheckTypeData(type, rawData)) return false;

    return true;
}

CSoteriaNameSystem::CSoteriaNameSystem(Type type, std::string rawData) :
    m_addr(""),
    m_ipv4()
{
    this->m_type = type;

    if (!CheckTypeData(this->m_type, rawData)) return;

    switch(this->m_type) {
        case ADDR: {
            this->m_addr = rawData;
            break;
        }
        case IPv4: {
            this->m_ipv4.string = HexToIPv4(rawData);
            this->m_ipv4.hex = rawData;
            break;
        }
    }
}

CSoteriaNameSystem::CSoteriaNameSystem(std::string ansID) :
    m_addr(""),
    m_ipv4()
{
    // Check if valid ID
    if(!IsValidID(ansID)) return;

    // Get type
    Type type = static_cast<Type>(stoi(ansID.substr(prefix.length(), 1), 0, 16));
    this->m_type = type;

    // Set info based on data
    std::string data = ansID.substr(prefix.length() + 1); // prefix + type

    switch(this->m_type) {
        case ADDR: {
            this->m_addr = data;
            break;
        }
        case IPv4: {
            this->m_ipv4.string = HexToIPv4(data);
            this->m_ipv4.hex = data;
            break;
        }    
    }
}

std::string CSoteriaNameSystem::EncodeHex()
{
    std::string strHex;

    // Add type as hex
    std::stringstream ss;
    ss << std::hex << this->m_type;
    strHex += ss.str();

    // Encode data
    switch(this->m_type) {
        case ADDR: {
            // Convert Base58 to hex
            std::vector<unsigned char> b;
            DecodeBase58(this->m_addr, b);
            strHex += std::string(b.begin(), b.end());
            break;
        }
        case IPv4: {
            strHex += this->m_ipv4.hex;
            break;
        }
    }

    std::vector<unsigned char> vec = ParseHex(strHex);
    return std::string(vec.begin(), vec.end());
}

std::string CSoteriaNameSystem::DecodeHex(std::string hex)
{
    /** TODO: Fix hex checking. */
    // // Check if hex
    // if(!IsHexNumber(hex)) return ""; // Return invalid SNS ID

    // Decode hex str
    hex = HexStr(hex);

    // Get type
    Type type = static_cast<Type>(stoi(hex.substr(0, 1)));

    // Decode data
    std::string encodedData = hex.substr(1);
    std::string decodedData = "";

    switch(type) {
        case ADDR: {
            // Convert hex to Base58
            std::vector<char> charData(encodedData.begin(), encodedData.end());
            std::vector<unsigned char> unsignedCharData;
            for (char c : charData)
                unsignedCharData.push_back(static_cast<unsigned char>(c));
            decodedData = EncodeBase58(unsignedCharData);
            break;
        }
        case IPv4: {
            decodedData = encodedData; // SNS already expects IPv4 as hex.
            break;
        }
    }
    
    CSoteriaNameSystem ansID(type, decodedData);
    return ansID.to_string();
}

std::string CSoteriaNameSystem::to_string() {
    std::string id = "";

    // 1. Add prefix
    id += prefix;

    // 2. Add type
    std::stringstream ss;
    ss << std::hex << this->m_type;
    id += ss.str();

    // 3. Add data
    switch(this->m_type) {
        case ADDR: {
            id += this->m_addr;
            break;
        }
        case IPv4: {
            id += this->m_ipv4.hex;
            break;
        }
    }

    return id;
}

UniValue CSoteriaNameSystem::to_object()
{
    UniValue ansInfo(UniValue::VOBJ);

    ansInfo.pushKV("ans_id", this->to_string());
    ansInfo.pushKV("ans_type_hex", this->m_type);
    ansInfo.pushKV("ans_encoded_hex", this->EncodeHex());

    switch(this->m_type) {
        case ADDR: {
            ansInfo.pushKV("ans_addr", this->m_addr);
            break;
        }
        case IPv4: {
            ansInfo.pushKV("ans_ip_hex", this->m_ipv4.hex);
            ansInfo.pushKV("ans_ip", this->m_ipv4.string);
            break;
        }
    }

    return ansInfo;
}
