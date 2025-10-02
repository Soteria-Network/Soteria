// Copyright (c) 2022 The Avian Core developers
// Copyright (c) 2022 Shafil Alam
// Copyright (c) 2025 The Soteria Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SOTERIA_QT_SMARTCONTRACTS_H
#define SOTERIA_QT_SMARTCONTRACTS_H

#include <QDialog>
#include <QThread>

class PlatformStyle;

namespace Ui {
    class Smartcontracts;
}

class Smartcontracts: public QDialog
{
    Q_OBJECT

public:
    explicit Smartcontracts(const PlatformStyle *platformStyle, QWidget *parent = 0);
    ~Smartcontracts();

private Q_SLOTS:
    void wrapped_clicked();

private:
    Ui::Smartcontracts*ui;
};

#endif // SOTERIA_QT_SMARTCONTRACTS_H
