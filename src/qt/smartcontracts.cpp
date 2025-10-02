// Copyright (c) 2022 The Avian Core developers
// Copyright (c) 2022 Shafil Alam
// Copyright (c) 2025 The Soteria Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include "config/soteria-config.h"
#endif

#include "smartcontracts.h"
#include "ui_smartcontracts.h"
#include "smartcontracts/smartcontracts.h"

#include "guiutil.h"
#include "platformstyle.h"
#include "guiconstants.h"
#include "validation.h"
#include <util/system.h>

#include <vector>
#include <string>

#include <QMessageBox>
#include <QSignalMapper>
#include <QStringList>

#include <QPainter>
#include <QPainterPath>
#include <QGraphicsDropShadowEffect>

Smartcontracts::Smartcontracts(const PlatformStyle *platformStyle, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Smartcontracts)
{
    ui->setupUi(this);

    // Set data dir label
    ui->labelDatadir->setText(tr("List of smartcontracts in: ") + QString::fromStdString((GetDataDir() / "smartcontracts" ).string()));

    // Set warning
    if (!AreSmartContractsDeployed()) {
        ui->labelAlerts->setText(tr("Warning: Soteria Smart Plans are not deployed."));
    }

    if (!gArgs.IsArgSet("-smartcontracts") && AreSmartContractsDeployed()) {
        ui->labelAlerts->setText(tr("Warning: Soteria Smart Plans are deployed but is disabled."));
    }

    if (gArgs.IsArgSet("-smartcontracts") && AreSmartContractsDeployed()) {
        ui->labelAlerts->setText(tr("Warning: Soteria Smart Plans are ACTIVE! Please exercise extreme caution."));
    }

    // List all smart plans
    auto plans = CSoteriaSmartContracts::GetPlans();
    ui->listWidget->addItem(QString::fromStdString(std::string("There are ") + std::to_string(plans.size()) + std::string(" smartcontracts.")));
    for(const std::string& plan : plans) {
        ui->listWidget->addItem(QString::fromStdString(plan));
    }

    /** Connect signals */
    // connect(ui->wrapButton, SIGNAL(clicked()), this, SLOT(wrapped_clicked()));
}

void Smartcontracts::wrapped_clicked()
{
    /** Show warning */
}

Smartcontracts::~Smartcontracts()
{
    delete ui;
}
