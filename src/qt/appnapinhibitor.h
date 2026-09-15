// Copyright (c) 2025-2026 The Soteria Core developer

#ifndef SOTERIA_QT_APPNAPINHIBITOR_H
#define SOTERIA_QT_APPNAPINHIBITOR_H

#include <QtCore/qglobal.h>

#ifdef Q_OS_MAC
class CAppNapInhibitor
{
public:
    CAppNapInhibitor();
    ~CAppNapInhibitor();
private:
    void* m_token = nullptr;
};
#else
class CAppNapInhibitor
{
public:
    CAppNapInhibitor() {}
    ~CAppNapInhibitor() {}
};
#endif

#endif
