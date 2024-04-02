/*
 * Created by Florian Märkl <info@florianmaerkl.de>
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

#include <cstring>

#include <QBoxLayout>
#include <QKeyEvent>

#include "psvr.h"
#include "hmdwindow.h"

HMDWindow::HMDWindow(VideoPlayer *video_player, PsvrSensors *psvr, QWidget *parent) : QMainWindow(parent)
{
	this->video_player = video_player;
	this->psvr = psvr;

	main_window = 0;

	setWindowTitle(tr("PSVR HMD"));

	resize(640, 480);

	hmd_widget = new HMDWidget(video_player, psvr);
	setCentralWidget(hmd_widget);
}

HMDWindow::~HMDWindow()
{
	delete hmd_widget;
}

void HMDWindow::SwitchFullScreen(bool makefull) {
  if (makefull) {
    showFullScreen();
    setCursor(Qt::BlankCursor);
    ShowCross(true);
//    hmd_widget->SetEyesDistance(-0.01f);
  }
  else {
    showNormal();
    unsetCursor();
    ShowCross(false);
  }
}

void HMDWindow::ChangeFullScreen() {
  SwitchFullScreen(!isFullScreen());
}

void HMDWindow::keyPressEvent(QKeyEvent *event)
{
	switch(event->key())
	{
		case Qt::Key_R:
			psvr->ResetView();
			break;
		case Qt::Key_F11:
      ChangeFullScreen();
			break;
		default:
			QMainWindow::keyPressEvent(event);
			break;

	}
}

void HMDWindow::closeEvent(QCloseEvent *event)
{
	if(main_window)
	{
		main_window->SetHMDWindow(0);
		main_window->close();
	}

  QMainWindow::closeEvent(event);
}

void HMDWindow::ShowCross(bool on) {
  return;
  if (!on) {
    video_player->SetScreen(VideoDataInfoPtr(), VideoDataInfoPtr());
  }

  auto or_mask = video_player->GetAvailableData();
  auto and_mask = video_player->GetAvailableData();
  // TODO SOME Modifications
  std::memset(and_mask->GetData(), 0x00, and_mask->GetDataRawSize());

  auto om = or_mask->GetData();
  auto am = and_mask->GetData();
  auto ms = or_mask->GetDataRawSize();
  std::memset(om, 0, ms);
  for (size_t i = 2880; i < ms; i += 5760) {
    om[i] = 0xff;
    om[i + 1] = 0xff;
    om[i + 2] = 0xff;
    am[i] = 0xff;
    am[i + 1] = 0xff;
    am[i + 2] = 0xff;
  }

  size_t sm = 1920 * 270 * 3;
  for (size_t i = 0; i < 1920 * 3; i += 3) {
    om[i + sm] = 0xff;
    om[i + 1 + sm] = 0xff;
    om[i + 2 + sm] = 0xff;
    am[i + sm] = 0xff;
    am[i + 1 + sm] = 0xff;
    am[i + 2 + sm] = 0xff;
  }

  sm = 1920 * 810 * 3;
  for (size_t i = 0; i < 1920 * 3; i += 3) {
    om[i + sm] = 0xff;
    om[i + 1 + sm] = 0xff;
    om[i + 2 + sm] = 0xff;
    am[i + sm] = 0xff;
    am[i + 1 + sm] = 0xff;
    am[i + 2 + sm] = 0xff;
  }

  video_player->SetScreen(or_mask, and_mask);
}
