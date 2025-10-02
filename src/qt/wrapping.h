// Copyright (c) 2011-2016 The Bitcoin Core developers
// Copyright (c) 2017-2019 The Raven Core developers
// Copyright (c) 2025 The Soteria Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SOTERIA_WRAPPING_H
#define SOTERIA_WRAPPING_H

#include "walletmodel.h"
#include "guiutil.h"

#include <QDialog>
#include <QThread>

class PlatformStyle;

namespace Ui {
    class WrapPage;
}

class WrapPage: public QDialog
{
    Q_OBJECT

public:
    explicit WrapPage(const PlatformStyle *platformStyle, QWidget *parent = 0);
    ~WrapPage();

    void setModel(WalletModel *model);

private Q_SLOTS:
    void wrapped_clicked();
    void setBalance(const CAmount& balance, const CAmount& unconfirmedBalance, const CAmount& immatureBalance,
                    const CAmount& watchOnlyBalance, const CAmount& watchUnconfBalance, const CAmount& watchImmatureBalance);

private:
    Ui::WrapPage *ui;
    WalletModel *model;
};

#endif // SOTERIA_WRAPPING_H
