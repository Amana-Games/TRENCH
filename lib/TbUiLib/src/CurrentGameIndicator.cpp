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

#include "ui/CurrentGameIndicator.h"

#include <QPixmap>
#include <QString>

#include "mdl/GameInfo.h"
#include "ui/ImageUtils.h"

#include <filesystem>
#include "PreferenceManager.h"

namespace tb::ui
{

CurrentGameIndicator::CurrentGameIndicator(const mdl::GameInfo& gameInfo, QWidget* parent)
  : DialogHeader{parent}
{
  const auto gamePath = pref(gameInfo.gamePathPreference);
  auto iconPath = gamePath / "gameicon.png";
  if (!std::filesystem::exists(iconPath)) {
      iconPath = gameInfo.gameConfig.findConfigFile(gameInfo.gameConfig.icon);
      if (iconPath.empty())
      {
        iconPath = std::filesystem::path{"DefaultGameIcon.svg"};
      }
  }

  // [AMANA GAMES] Scale icon to a consistent size.
  const auto gameIcon = loadPixmap(iconPath).scaled(48, 48, Qt::KeepAspectRatio, Qt::SmoothTransformation);
  set(QString::fromStdString(gameInfo.gameConfig.name), gameIcon);
}

} // namespace tb::ui
