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
#include <fstream>

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
  info_data_ = hmd_widget->GetInfoData();
  setCentralWidget(hmd_widget);

  LoadTestInfo();
}

HMDWindow::~HMDWindow()
{
	delete hmd_widget;
}

void HMDWindow::SwitchFullScreen(bool makefull) {
  if (makefull) {
    showFullScreen();
    setCursor(Qt::BlankCursor);
//    hmd_widget->SetEyesDistance(-0.01f);
  }
  else {
    showNormal();
    unsetCursor();
  }
}

void HMDWindow::ChangeFullScreen() {
  SwitchFullScreen(!isFullScreen());
}

void HMDWindow::SetEyesDistance(float disp) {
  hmd_widget->SetEyesDistance(disp);
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

void HMDWindow::LoadTestInfo() {
  if (!info_data_) { return; }
  std::ifstream f("info_test.data", std::ios_base::binary);
  if (f) {
    f.read(reinterpret_cast<char*>(info_data_),
        HMDWidget::kInfoHeight * sizeof(HMDWidget::InfoTextureRow));
  }
}

