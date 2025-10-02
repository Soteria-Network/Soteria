// Copyright (c) 2022 Shafil Alam
// Copyright (c) 2025 The Soteria Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SOTERIA_NAME_SYSTEM_H
#define SOTERIA_NAME_SYSTEM_H

#include <string>
#include <array>

#include "univalue.h"


/** Struct to store SNS IP data */
struct ANS_IP {
    std::string string{""};
    std::string hex{""}; 
};

/** Soteria Name System */
class CSoteriaNameSystem {
public:
    /** Static prefix for SNS IDs */
    static const std::string prefix;
    /** Static domain */
    static const std::string domain;

    /** SNS types */
    enum Type {
        // Soteria address
        ADDR = 0x0,
        // Raw IPv4 (127.0.0.1)
        IPv4 = 0x1
    };

    /** Convert SNS type into string with description */
    static std::pair<std::string, std::string> enum_to_string(Type type) {
        switch(type) {
            case ADDR:
                return std::make_pair("Soteria address", "Enter a Soteria address");
            case IPv4:
                return std::make_pair("IPv4 [DNS A record]", "Enter IPv4 address");
            default:
                return std::make_pair("Invalid", "Invalid");
        }
    }

    CSoteriaNameSystem(Type type, std::string rawData);
    CSoteriaNameSystem(std::string ansID);

    /** Get SNS ID as string */
    std::string to_string();
    
    /** SNS ID encode/decode */
    std::string EncodeHex();
    static std::string DecodeHex(std::string hex);

    /** Get JSON object about this SNS ID */
    UniValue to_object();

    /** SNS return types */
    Type type() { return m_type; };
    std::string addr() { return m_addr; };
    std::string ipv4() { return m_ipv4.string; };

    /** Check if valid IPv4 address */
    static bool CheckIPv4(std::string rawip, bool isHex);

    /** Check if valid SNS ID */
    static bool IsValidID(std::string ansID);

    /** Check SNS raw data based on type */
    static bool CheckTypeData(Type type, std::string typeData);
    /** Convert raw data into SNS ID type data */
    static std::string FormatTypeData(Type type, std::string typeData, std::string& error);

private:
    /** SNS type */
    Type m_type;
    /** Soteria address */
    std::string m_addr;
    /** IPv4 string+hex */
    ANS_IP m_ipv4;
};

/** Public array of SNS types  */
constexpr std::array<CSoteriaNameSystem::Type, 2> ANSTypes { 
    CSoteriaNameSystem::ADDR, 
    CSoteriaNameSystem::IPv4
};

#endif // SOTERIA_NAME_SYSTEM_H
