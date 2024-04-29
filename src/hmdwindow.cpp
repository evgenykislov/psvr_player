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
#include "psvr_control.h"
#include "hmdwindow.h"

HMDWindow::HMDWindow(VideoPlayer *video_player, PsvrSensors *psvr,
    PsvrControl* psvr_control, QWidget *parent): QMainWindow(parent),
    psvr_control_(psvr_control) {
	this->video_player = video_player;
	this->psvr = psvr;

	main_window = 0;

	setWindowTitle(tr("PSVR HMD"));

	resize(640, 480);

	hmd_widget = new HMDWidget(video_player, psvr);
  info_data_ = hmd_widget->GetInfoData();
  setCentralWidget(hmd_widget);
  ShowMenu();

//  LoadTestInfo();

  connect(video_player, SIGNAL(Playing()), this, SLOT(PlayerPlaying()));
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

void HMDWindow::SetHorizontLevel(float horz) {
  hmd_widget->SetHorizontLevel(horz);
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


void HMDWindow::OnUp() {
  if (show_menu_) {
    info_scr_.DoUp();
    UpdateInformation();
  }
}

void HMDWindow::OnDown() {
  if (show_menu_) {
    info_scr_.DoDown();
    UpdateInformation();
  }
}

void HMDWindow::OnLeft() {
  if (show_menu_) {
    info_scr_.DoLeft();
    UpdateInformation();
  }
}

void HMDWindow::OnRight() {
  if (show_menu_) {
    info_scr_.DoRight();
    UpdateInformation();
  }
}

void HMDWindow::OnSelect() {
  if (video_player->IsPlaying()) {
    video_player->Pause();
    ShowMenu();
  } else {
    int value;

    auto action = info_scr_.DoSelectAndGetAction(value);
    switch (action) {
      case InformationScreen::kNoAction: break;
      case InformationScreen::kInvertAction: ActionInvert(value); break;
      case InformationScreen::kPlayAction: ActionPlay(); break;
      case InformationScreen::kRePositionAction: PlayerMakeStep(value * 1000); break;
      default:
        assert(false);
    }
    UpdateInformation();
  }
}


void HMDWindow::PlayerMakeStep(int move_ms) {


  if (move_ms < 0 && current_play_position_ < static_cast<uint64_t>(-move_ms)) {
    // Goto start
    current_play_position_ = 0;
  }
  else if (media_duration_ <= kBeforeEndInterval) {
    // Use default value
    current_play_position_ = 0;
  }
  else if (current_play_position_ + move_ms > (media_duration_ - kBeforeEndInterval)) {
    // Stay to before-end position
    current_play_position_ = media_duration_ - kBeforeEndInterval;
  }
  else {
    current_play_position_ += move_ms;
  }

  video_player->SetPosition(static_cast<float>(current_play_position_) /
      media_duration_);
}

void HMDWindow::ShowMenu()
{
  show_menu_ = true;
  info_scr_.ShowMenu(true);
  UpdateInformation();
}

void HMDWindow::UpdateInformation() {
  auto scr = info_scr_.GetInfoScr();
  auto wd = info_scr_.GetInfoScrWidth();
  auto ht = info_scr_.GetInfoScrHeight();
  if ((kInfoScrXPos + wd) > HMDWidget::kInfoWidth) {
    assert(false);
    return;
  }
  if ((kInfoScrYPos + ht) > HMDWidget::kInfoHeight) {
    assert(false);
    return;
  }

  for (size_t y = 0; y < ht; ++y) {
    for (size_t x = 0; x < wd; ++x) {
      info_data_[kInfoScrYPos + y][kInfoScrXPos + x] = scr[y * wd + x];
    }
  }

  hmd_widget->ForceUpdateInfo();
}



void HMDWindow::HideMenu()
{
  show_menu_ = false;
  info_scr_.ShowMenu(false);
  UpdateInformation();
}

void HMDWindow::ActionInvert(int value) {
  hmd_widget->SetInvertStereo(value != 0);
}

void HMDWindow::ActionPlay() {
  HideMenu();
  video_player->Play();
}

void HMDWindow::PlayerPlaying() {
  if (!psvr_control_->IsOpened() && !psvr_control_->OpenDevice()) {
    info_scr_.SetNoVrWarning(true);
  }
  else {
    info_scr_.SetNoVrWarning(false);
    psvr_control_->SetVRMode(true);
  }
  HideMenu();

  // TODO
//  hmd_window->SwitchFullScreen(true);
//  hmd_window->activateWindow();
}
