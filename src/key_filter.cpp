/*
 * Created by Evgeny Kislov <dev@evgenykislov.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "key_filter.h"

#include <functional>

#include <QKeyEvent>


struct KeyEvent {
  int key;
  bool ctrl;
  bool shift;
  bool alt;
  std::function<void(KeyFilter*)> processor;
};

KeyEvent g_keys[] = {
  {Qt::Key_Space, false, false, false, [](KeyFilter* kf){ emit kf->Pause(); }},
  {Qt::Key_Space, true,  false, false, [](KeyFilter* kf){ emit kf->Stop(); }},
  // Fast forward
  {Qt::Key_Left,  false, false, false, [](KeyFilter* kf){ emit kf->MakeStep(-10000); }},
  {Qt::Key_Right, false, false, false, [](KeyFilter* kf){ emit kf->MakeStep(10000); }},
  {Qt::Key_Left,  true,  false, false, [](KeyFilter* kf){ emit kf->MakeStep(-60000); }},
  {Qt::Key_Right, true,  false, false, [](KeyFilter* kf){ emit kf->MakeStep(60000); }},
  {Qt::Key_Left,  false, true,  false, [](KeyFilter* kf){ emit kf->MakeStep(-600000); }},
  {Qt::Key_Right, false, true,  false, [](KeyFilter* kf){ emit kf->MakeStep(600000); }},
  // Reset view
  {Qt::Key_R,     false, false, false, [](KeyFilter* kf){ emit kf->ResetView(); }},
  {Qt::Key_Up,    true,  false, false, [](KeyFilter* kf){ emit kf->ResetView(); }},
  {Qt::Key_Down,  true,  false, false, [](KeyFilter* kf){ emit kf->ResetView(); }},
  // Full screen
  {Qt::Key_F11,   false, false, false, [](KeyFilter* kf){ emit kf->FullScreen(); }}
};

bool KeyFilter::eventFilter(QObject*, QEvent* event) {
  if (event->type() == QEvent::KeyPress) {
    QKeyEvent* ke = static_cast<QKeyEvent*>(event);
    auto key = ke->key();
    bool ctrl = (ke->modifiers() & Qt::ControlModifier) != 0;
    bool shift = (ke->modifiers() & Qt::ShiftModifier) != 0;
    bool alt = (ke->modifiers() & Qt::AltModifier) != 0;

    for (auto it = std::begin(g_keys); it != std::end(g_keys); ++it) {
      if (it->key == key && it->ctrl == ctrl && it->shift == shift && it->alt == alt) {
        it->processor(this);
        return true;
      }
    }
  }

  return false;
}
