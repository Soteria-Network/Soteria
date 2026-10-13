// Copyright (c) 2011-2016 The Bitcoin Core developers
// Copyright (c) 2017-2019 The Raven Core developers
// Copyright (c) 2025-2026 The Soteria Core developer

#include "assettablemodel.h"
#include "assetrecord.h"
#include <string>
#include "guiconstants.h"
#include "guiutil.h"
#include "walletmodel.h"
#include "wallet/wallet.h"
#include <vector>
#include <set>
#include "core_io.h"
#include <map>
#include "amount.h"
#include "assets/assets.h"
#include "validation.h"
#include "platformstyle.h"

#include <QDebug>
#include <QStringList>



class AssetTablePriv {
public:
    AssetTablePriv(AssetTableModel *_parent) :
            parent(_parent)
    {
    }

    AssetTableModel *parent;

    QList<AssetRecord> cachedBalances;

    // loads all current balances into cache
#ifdef ENABLE_WALLET
    void refreshWallet() {
        qDebug() << "AssetTablePriv::refreshWallet";
        auto currentActiveAssetCache = GetCurrentAssetCache();
        if (currentActiveAssetCache) {
            {
                LOCK(cs_main);
                std::map<std::string, CAmount> balances;
                std::map<std::string, std::vector<COutput> > outputs;
                if (!GetAllMyAssetBalances(outputs, balances)) {
                    qWarning("AssetTablePriv::refreshWallet: Error retrieving asset balances");
                    return;
                }
                QList<AssetRecord> refreshedBalances;                
                std::set<std::string> setAssetsToSkip;
                auto bal = balances.begin();
                for (; bal != balances.end(); bal++) {
                    // retrieve units for asset
                    uint8_t units = OWNER_UNITS;
                    bool fIsAdministrator = true;
                    std::string ipfsHash = "";
                    std::string ansID = "";

                    if (setAssetsToSkip.count(bal->first))
                        continue;

                    if (!IsAssetNameAnOwner(bal->first)) {
                        // Asset is not an administrator asset
                        CNewAsset assetData;
                        if (!currentActiveAssetCache->GetAssetMetaDataIfExists(bal->first, assetData)) {
                            qWarning("AssetTablePriv::refreshWallet: Error retrieving asset data");
                            return;
                        }
                        units = assetData.units;
                        ipfsHash = assetData.strIPFSHash;
                        ansID = assetData.strANSID;
                        // If we have the administrator asset, add it to the skip listå
                        const std::string owner_name = GetAssetOwnerName(bal->first);
                        if (balances.count(owner_name)) {
                            setAssetsToSkip.insert(owner_name);
                        } else {
                            fIsAdministrator = false;
                        }
                    } else {
                        // Asset is an administrator asset, if we own assets that is administrators, skip this balance
                        std::string name = bal->first;
                        name.pop_back();
                        if (balances.count(name) || balances.count("$" + name)) {
                            setAssetsToSkip.insert(bal->first);
                            continue;
                        }
                    }
                    int64_t assetTime = 0;
                    const auto outputsIt = outputs.find(bal->first);
                    if (outputsIt != outputs.end()) {
                        for (const auto& txout : outputsIt->second) {
                            if (txout.tx && (assetTime == 0 || txout.tx->GetTxTime() < assetTime))
                                assetTime = txout.tx->GetTxTime();
                        }
                    }
                    refreshedBalances.append(AssetRecord(bal->first, bal->second, units, fIsAdministrator, EncodeAssetData(ipfsHash), EncodeANS(ansID), assetTime));
                }
                cachedBalances = refreshedBalances;
            }
        }
    }
#endif


    int size() {
        return cachedBalances.size();
    }

    AssetRecord *index(int idx) {
        if (idx >= 0 && idx < cachedBalances.size()) {
            return &cachedBalances[idx];
        }
        return 0;
    }

};

AssetTableModel::AssetTableModel(WalletModel *parent) :
        QAbstractTableModel(parent),
        walletModel(parent),
        priv(new AssetTablePriv(this))
{
    columns << tr("Name") << tr("Quantity");
#ifdef ENABLE_WALLET
    priv->refreshWallet();
#endif
};

AssetTableModel::~AssetTableModel()
{
    delete priv;
};

void AssetTableModel::checkBalanceChanged() {
    qDebug() << "AssetTableModel::CheckBalanceChanged";
    // TODO: optimize by 1) updating cache incrementally; and 2) emitting more specific dataChanged signals
    Q_EMIT layoutAboutToBeChanged();
#ifdef ENABLE_WALLET
    priv->refreshWallet();
#endif
    Q_EMIT dataChanged(index(0, 0, QModelIndex()), index(priv->size(), columns.length()-1, QModelIndex()));
    Q_EMIT layoutChanged();
}

int AssetTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return priv->size();
}

int AssetTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 2;
}

QVariant AssetTableModel::data(const QModelIndex &index, int role) const
{
    Q_UNUSED(role);
    if(!index.isValid())
        return QVariant();
    AssetRecord *rec = static_cast<AssetRecord*>(index.internalPointer());

    switch (role)
    {
        case AmountRole:
            return (unsigned long long) rec->quantity;
        case AssetNameRole:
            return QString::fromStdString(rec->name);
        case FormattedAmountRole:
            return QString::fromStdString(rec->formattedQuantity());
        case AdministratorRole:
            return rec->fIsAdministrator;
        case AssetIPFSHashRole:
            return QString::fromStdString(rec->ipfshash);
        case AssetDateRole:
            return QVariant::fromValue<qint64>(rec->nTime);
        case AssetIPFSHashDecorationRole:
        {
            if (index.column() == Quantity)
                return QVariant();

            if (rec->ipfshash.size() == 0)
                return QVariant();

            QPixmap pixmap;

            if (darkModeEnabled)
                pixmap = QPixmap::fromImage(QImage(":/icons/external_link_dark"));
            else
                pixmap = QPixmap::fromImage(QImage(":/icons/external_link"));

            return pixmap;
        }
        case AssetANSRole:
            return QString::fromStdString(rec->ansID);
        case AssetANSDecorationRole:
        {
            if (index.column() == Quantity)
                return QVariant();

            if (rec->ansID.size() == 0)
                return QVariant();

            QPixmap pixmap;

            if (darkModeEnabled)
                pixmap = QPixmap::fromImage(QImage(":/icons/export"));
            else
                pixmap = QPixmap::fromImage(QImage(":/icons/export"));

            return pixmap;
        }
        case Qt::DecorationRole:
        {
            if (index.column() == Quantity)
                return QVariant();

            if (!rec->fIsAdministrator)
                QVariant();

            QPixmap pixmap;

            pixmap = QPixmap::fromImage(QImage(":/icons/asset_administrator"));

            return pixmap;
        }
        case Qt::DisplayRole: {
            if (index.column() == Name)
                return QString::fromStdString(rec->name);
            else if (index.column() == Quantity)
                return QString::fromStdString(rec->formattedQuantity());
        }
        case Qt::ToolTipRole:
            return formatTooltip(rec);
        case Qt::TextAlignmentRole:
        {
            if (index.column() == Quantity) {
                return Qt::AlignRight + Qt::AlignVCenter;
            }
        }
        default:
            return QVariant();
    }
}

QVariant AssetTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole)
    {
        if (orientation == Qt::Horizontal) {
            if (section < columns.size())
                return columns.at(section);
        } else {
            return section;
        }
    } else if (role == Qt::SizeHintRole) {
        if (orientation == Qt::Vertical)
            return QSize(30, 50);
    } else if (role == Qt::TextAlignmentRole) {
        if (orientation == Qt::Vertical)
            return Qt::AlignLeft + Qt::AlignVCenter;

        return Qt::AlignHCenter + Qt::AlignVCenter;
    }

    return QVariant();
}

QModelIndex AssetTableModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    AssetRecord *data = priv->index(row);
    if(data)
    {
        QModelIndex idx = createIndex(row, column, priv->index(row));
        return idx;
    }

    return QModelIndex();
}

QString AssetTableModel::formatTooltip(const AssetRecord *rec) const
{
    QString tooltip = formatAssetName(rec) + QString("\n") + formatAssetQuantity(rec) + QString("\n") + formatAssetData(rec);
    return tooltip;
}

QString AssetTableModel::formatAssetName(const AssetRecord *wtx) const
{
    return QString::fromStdString(wtx->name);
}

QString AssetTableModel::formatAssetQuantity(const AssetRecord *wtx) const
{
    return QString::fromStdString(wtx->formattedQuantity());
}

QString AssetTableModel::formatAssetData(const AssetRecord *wtx) const
{
    return QString::fromStdString(wtx->ipfshash);
}
