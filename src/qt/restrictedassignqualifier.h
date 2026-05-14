// Copyright (c) 2011-2016 The Bitcoin Core developers
// Copyright (c) 2017-2019 The Raven Core developers
// Copyright (c) 2025-present The Soteria Core developers

#ifndef SOTERIA_QT_ASSIGNQUALIFIER_H
#define SOTERIA_QT_ASSIGNQUALIFIER_H

#include "amount.h"

#include <QMenu>
#include <QWidget>
#include <memory>

class ClientModel;
class PlatformStyle;
class WalletModel;
class QStringListModel;
class QSortFilterProxyModel;
class QCompleter;
class AssetFilterProxy;


namespace Ui
{
class AssignQualifier;
}

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

/** Overview ("home") page widget */
class AssignQualifier : public QWidget
{
    Q_OBJECT

public:
    explicit AssignQualifier(const PlatformStyle* _platformStyle, QWidget* parent = 0);
    ~AssignQualifier();

    void setClientModel(ClientModel* clientModel);
    void setWalletModel(WalletModel* walletModel);
    void showOutOfSyncWarning(bool fShow);
    Ui::AssignQualifier* getUI();
    bool eventFilter(QObject* object, QEvent* event);

    void enableSubmitButton();
    void showWarning(QString string, bool failure = true);
    void hideWarning();

    AssetFilterProxy* assetFilterProxy;
    QCompleter* completer;

private:
    Ui::AssignQualifier* ui;
    ClientModel* clientModel;
    WalletModel* walletModel;
    const PlatformStyle* platformStyle;

public Q_SLOTS:
    void clear();

private Q_SLOTS:
    void check();
    void dataChanged();
    void changeAddressChanged(int);
};

#endif // SOTERIA_QT_ASSIGNQUALIFIER_H
