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
 along with TrenchBroom.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "ui/GamesPreferencePane.h"

#include <QLabel>
#include <QAction>
#include <QBoxLayout>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QFormLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QStackedWidget>
#include <QToolButton>
#include <QWidget>

#include "PreferenceManager.h"
#include "fs/DiskIO.h"
#include "fs/PathInfo.h"
#include "mdl/GameConfig.h"
#include "mdl/GameManager.h"
#include "ui/AppController.h"
#include "ui/BitmapButton.h"
#include "ui/BorderLine.h"
#include "ui/EmptyWidget.h"
#include "ui/FileDialogDefaultDir.h"
#include "ui/FileLogger.h"
#include "ui/FormWithSectionsLayout.h"
#include "ui/GameEngineDialog.h"
#include "ui/GameListBox.h"
#include "ui/ImageUtils.h"
#include "ui/MapDocument.h"
#include "ui/MiniToolBarLayout.h"
#include "ui/QPathUtils.h"
#include "ui/SystemPaths.h"
#include "ui/ViewConstants.h"

namespace tb::ui
{

GamesPreferencePane::GamesPreferencePane(
  AppController& appController, MapDocument* document, QWidget* parent)
  : PreferencePane{parent}
  , m_appController{appController}
  , m_document{document}
{
  createGui();
  updateControls();
  m_gameListBox->setFocus();
}

void GamesPreferencePane::createGui()
{
  m_gameListBox = new GameListBox{m_appController};
  m_gameListBox->selectGame(0);
  m_gameListBox->setMaximumWidth(220);
  m_gameListBox->setMinimumHeight(300);

  m_defaultPage = createEmptyWidget(tr("Select a game."));

  m_stackedWidget = new QStackedWidget{};
  m_stackedWidget->addWidget(m_defaultPage);

  m_largeIconLabel = new QLabel{};
  m_largeIconLabel->setFixedSize(128, 128);
  m_largeIconLabel->setAlignment(Qt::AlignCenter);

  auto* glbContainer = new QWidget{};
  glbContainer->setFixedWidth(200);
  auto* glbLayout = new QVBoxLayout{glbContainer};
  glbLayout->addSpacing(20);
  glbLayout->addWidget(m_largeIconLabel, 0, Qt::AlignHCenter);
  glbLayout->addStretch();

  auto* stwLayout = new QVBoxLayout{};
  stwLayout->setContentsMargins(
    LayoutConstants::DialogOuterMargin,
    LayoutConstants::DialogOuterMargin,
    LayoutConstants::DialogOuterMargin,
    LayoutConstants::DialogOuterMargin);
  stwLayout->setSpacing(LayoutConstants::WideVMargin);
  stwLayout->addWidget(m_stackedWidget, 1, Qt::AlignTop);

  auto* layout = new QHBoxLayout{};
  layout->setContentsMargins(QMargins{});
  layout->setSpacing(0);
  setLayout(layout);

  layout->addWidget(glbContainer);
  layout->addWidget(new BorderLine(BorderLine::Direction::Vertical));
  layout->addSpacing(LayoutConstants::MediumVMargin);
  layout->addLayout(stwLayout, 1);

  setMinimumWidth(600);

  connect(
    m_gameListBox, &GameListBox::currentGameChanged, this, [&]() { updateControls(); });
}

void GamesPreferencePane::showUserConfigDirClicked()
{
  const auto path = SystemPaths::userGamesDirectory().lexically_normal();

  fs::Disk::createDirectory(path) | kdl::transform([&](auto) {
    const auto url = QUrl::fromLocalFile(pathAsQPath(path));
    QDesktopServices::openUrl(url);
  }) | kdl::transform_error([&](auto e) {
    if (m_document)
    {
      m_document->logger().error() << e.msg;
    }
    else
    {
      FileLogger::instance().error() << e.msg;
    }
  });
}

bool GamesPreferencePane::canResetToDefaults()
{
  return false;
}

void GamesPreferencePane::doResetToDefaults() {}

void GamesPreferencePane::updateControls()
{
  m_gameListBox->updateGameInfos();

  if (const auto* selectedGameInfo = m_gameListBox->selectedGameInfo())
  {
    // [AMANA GAMES] Update the large icon label
    auto gamePath = PreferenceManager::instance().getPendingValue(selectedGameInfo->gamePathPreference);
    auto iconPath = std::filesystem::path{pathAsQString(gamePath).toStdString()} / "gameicon.png";
    if (fs::Disk::pathInfo(iconPath) != fs::PathInfo::File) {
        iconPath = selectedGameInfo->gameConfig.findConfigFile("gameicon.png");
    }
    if (fs::Disk::pathInfo(iconPath) != fs::PathInfo::File) {
        iconPath = std::filesystem::path{"DefaultGameIcon.svg"};
    }
    m_largeIconLabel->setPixmap(loadPixmap(iconPath).scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation));

    if (m_currentGamePage && &m_currentGamePage->gameInfo() == selectedGameInfo)
    {
      // refresh the current page
      m_currentGamePage->updateControls();
    }
    else
    {
      // build a new current page
      delete m_currentGamePage;
      m_currentGamePage =
        new GamePreferencePane{m_appController, m_document, *selectedGameInfo};

      m_stackedWidget->addWidget(m_currentGamePage);
      m_stackedWidget->setCurrentWidget(m_currentGamePage);

      connect(
        m_currentGamePage,
        &GamePreferencePane::requestUpdate,
        this,
        &GamesPreferencePane::updateControls);
    }
  }
  else
  {
    m_stackedWidget->setCurrentWidget(m_defaultPage);
    m_largeIconLabel->setPixmap(QPixmap{});
  }
}

bool GamesPreferencePane::validate()
{
  return true;
}

// GamePreferencePane

GamePreferencePane::GamePreferencePane(
  AppController& appController,
  MapDocument* document,
  const mdl::GameInfo& gameInfo,
  QWidget* parent)
  : QWidget{parent}
  , m_appController{appController}
  , m_document{document}
  , m_gameInfo{gameInfo}
{
  createGui();
}

void GamePreferencePane::createGui()
{
  m_gamePathText = new QLineEdit{};
  m_gamePathText->setPlaceholderText(tr("Click on the button to change..."));
  connect(m_gamePathText, &QLineEdit::editingFinished, this, [this]() {
    updateGamePath(m_gamePathText->text());
  });

  auto* validDirectoryIcon = new QAction{m_gamePathText};
  m_gamePathText->addAction(validDirectoryIcon, QLineEdit::TrailingPosition);
  connect(
    m_gamePathText,
    &QLineEdit::textChanged,
    this,
    [validDirectoryIcon](const QString& text) {
      if (text.isEmpty() || QDir{text}.exists())
      {
        validDirectoryIcon->setToolTip("");
        validDirectoryIcon->setIcon(QIcon{});
      }
      else
      {
        validDirectoryIcon->setToolTip(tr("Directory not found"));
        validDirectoryIcon->setIcon(loadSVGIcon("IssueBrowser.svg"));
      }
    });

  auto* chooseGamePathButton = new QPushButton{tr("...")};
  connect(
    chooseGamePathButton,
    &QPushButton::clicked,
    this,
    &GamePreferencePane::chooseGamePathClicked);

  auto* configureEnginesButton = new QPushButton{tr("Configure engines...")};
  connect(
    configureEnginesButton,
    &QPushButton::clicked,
    this,
    &GamePreferencePane::configureEnginesClicked);

  auto* gamePathLayout = new QHBoxLayout{};
  gamePathLayout->setContentsMargins(QMargins{});
  gamePathLayout->setSpacing(LayoutConstants::MediumHMargin);
  gamePathLayout->addWidget(m_gamePathText, 1);
  gamePathLayout->addWidget(chooseGamePathButton);

  auto* layout = new FormWithSectionsLayout{};
  layout->setContentsMargins(0, LayoutConstants::MediumVMargin, 0, 0);
  layout->setVerticalSpacing(2);
  layout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

  layout->addSection(QString::fromStdString(m_gameInfo.gameConfig.name));
  layout->addRow(tr("Game Path"), gamePathLayout);
  // [AMANA GAMES] Engines are not manageable in this editor.
  // layout->addRow("", configureEnginesButton);



  setLayout(layout);
}

void GamePreferencePane::chooseGamePathClicked()
{
  const auto pathStr = QFileDialog::getExistingDirectory(
    this, tr("Game Path"), fileDialogDefaultDirectory(FileDialogDir::GamePath));
  if (!pathStr.isEmpty())
  {
    updateGamePath(pathStr);
  }
}

void GamePreferencePane::updateGamePath(const QString& str)
{
  updateFileDialogDefaultDirectoryWithDirectory(FileDialogDir::GamePath, str);

  auto& prefs = PreferenceManager::instance();
  prefs.set(m_gameInfo.gamePathPreference, pathFromQString(str));
  emit requestUpdate();
}

void GamePreferencePane::configureEnginesClicked()
{
  auto& logger = m_document ? m_document->logger() : FileLogger::instance();
  auto dialog = GameEngineDialog{m_appController, m_gameInfo, logger, this};
  dialog.exec();
}

const mdl::GameInfo& GamePreferencePane::gameInfo() const
{
  return m_gameInfo;
}

void GamePreferencePane::updateControls()
{
  auto& prefs = PreferenceManager::instance();

  // Refresh tool paths from preferences
  for (const auto& [tool, toolPathEditor] : m_toolPathEditors)
  {
    const auto& toolPath = prefs.getPendingValue(tool->pathPreference);
    toolPathEditor->setText(pathAsQString(toolPath));
  }

  // Refresh game path
  const auto gamePath = prefs.getPendingValue(m_gameInfo.gamePathPreference);
  m_gamePathText->setText(pathAsQString(gamePath));
}

} // namespace tb::ui
