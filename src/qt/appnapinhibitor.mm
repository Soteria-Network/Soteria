// Copyright (c) 2025-2026 The Soteria Core developer

#include <qt/appnapinhibitor.h>

#ifdef Q_OS_MAC
#import <Foundation/Foundation.h>

CAppNapInhibitor::CAppNapInhibitor()
{
    m_token = [[NSProcessInfo processInfo]
        beginActivityWithOptions:NSActivityUserInitiated
                          reason:@"Soteria Wallet syncing"];
}

CAppNapInhibitor::~CAppNapInhibitor()
{
    if (m_token) {
        [[NSProcessInfo processInfo] endActivity:(id)m_token];
        m_token = nullptr;
    }
}
#endif
