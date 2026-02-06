/*
 Copyright (C) 2010 Kristian Duske

 This file is part of TrenchBroom.

 TrenchBroom is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 TrenchBroom is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with TrenchBroom. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ui/AppInfoPanel.h"

#include <QApplication>
#include <QClipboard>
#include <QLabel>
#include <QString>
#include <QStringBuilder>
#include <QVBoxLayout>

#include "ui/AppController.h"
#include "ui/BorderLine.h"
#include "ui/ClickableLabel.h"
#include "ui/GetVersion.h"
#include "ui/ImageUtils.h"
#include "ui/QPathUtils.h"
#include "ui/QStyleUtils.h"
#include "ui/ViewConstants.h"
#include "PreferenceManager.h"
#include "mdl/GameInfo.h"
#include "mdl/GameManager.h"
#include "update/Updater.h"

namespace tb::ui
{

AppInfoPanel::AppInfoPanel(AppController& appController, QWidget* parent)
  : QWidget{parent}
{
  auto appIconImage = loadPixmap("AppIcon.png");
  if (appIconImage.width() > 256 || appIconImage.height() > 256)
  {
    appIconImage = appIconImage.scaled(
      256, 256, Qt::KeepAspectRatio, Qt::SmoothTransformation);
  }
  auto* appIcon = new QLabel{};
  appIcon->setPixmap(appIconImage);

  auto* appName = new QLabel{tr("TRENCH")};
  setHeaderStyle(appName);

  auto* appLine = new BorderLine{};
  auto* appClaim = new QLabel{tr("")};

  auto version = new ClickableLabel{tr("Version %1").arg(getBuildVersion())};
  auto build = new ClickableLabel{tr("Build %1").arg(getBuildIdStr())};
  auto qtVersion =
    new ClickableLabel{tr("Qt %1").arg(QString::fromLocal8Bit(qVersion()))};

  setInfoStyle(version);
  setInfoStyle(build);
  setInfoStyle(qtVersion);
  build->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

  // TRENCH specific info
  auto* greeting = new QLabel{tr("AMANA GAMES")};
  setHeaderStyle(greeting);
  greeting->setAlignment(Qt::AlignCenter);

  auto* credits = new QLabel{tr("TRENCH - by AMANA GAMES")};
  setInfoStyle(credits); // Using info style for a more discreet look
  auto creditsFont = credits->font();
  creditsFont.setBold(true);
  creditsFont.setPointSize(creditsFont.pointSize() - 1);
  credits->setFont(creditsFont);
  credits->setAlignment(Qt::AlignCenter);

  auto* gamePathLabel = new QLabel{};
  auto& gameManager = appController.gameManager();
  const auto* gInfo = gameManager.gameInfo("AINIMONIA");
  if (gInfo)
  {
    auto& prefs = PreferenceManager::instance();
    auto gPath = prefs.get(gInfo->gamePathPreference);
    QString pathStr = pathAsQString(gPath);
    if (pathStr.isEmpty()) pathStr = tr("Not configured");
    gamePathLabel->setText(tr("Game Path: %1").arg(pathStr));
  }
  setInfoStyle(gamePathLabel);
  gamePathLabel->setAlignment(Qt::AlignCenter);

  const auto tooltip = tr("Click to copy to clipboard");
  version->setToolTip(tooltip);
  build->setToolTip(tooltip);
  qtVersion->setToolTip(tooltip);

  connect(version, &ClickableLabel::clicked, this, &AppInfoPanel::versionInfoClicked);
  connect(build, &ClickableLabel::clicked, this, &AppInfoPanel::versionInfoClicked);
  connect(qtVersion, &ClickableLabel::clicked, this, &AppInfoPanel::versionInfoClicked);

  auto* versionLayout = new QHBoxLayout{};
  versionLayout->setContentsMargins(0, 0, 0, 0);
  versionLayout->setSpacing(LayoutConstants::MediumHMargin);
  versionLayout->addWidget(version);

  auto* versionWidget = new QWidget{};
  versionWidget->setLayout(versionLayout);

  auto* layout = new QVBoxLayout{};
  layout->setContentsMargins(20, 20, 20, 20);
  layout->setSpacing(8);
  layout->addStretch();
  layout->addWidget(greeting, 0, Qt::AlignHCenter);
  layout->addSpacing(10);
  layout->addWidget(appIcon, 0, Qt::AlignHCenter);
  layout->addWidget(appName, 0, Qt::AlignHCenter);
  layout->addWidget(appLine);
  layout->addWidget(appClaim, 0, Qt::AlignHCenter);
  layout->addWidget(gamePathLabel, 0, Qt::AlignHCenter);
  layout->addSpacing(10);
  layout->addWidget(versionWidget, 0, Qt::AlignHCenter);
  layout->addWidget(build, 0, Qt::AlignHCenter);
  layout->addWidget(qtVersion, 0, Qt::AlignHCenter);
  layout->addStretch();
  layout->addWidget(credits, 0, Qt::AlignHCenter);
  layout->addStretch();

  setLayout(layout);
}

void AppInfoPanel::versionInfoClicked()
{
  const auto str =
    tr("TRENCH %1 Build %2").arg(getBuildVersion()).arg(getBuildIdStr());

  QApplication::clipboard()->setText(str);
}

} // namespace tb::ui
