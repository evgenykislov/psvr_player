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
#include "tile.h"
#include "hmdwindow.h"

HMDWindow::HMDWindow(VideoPlayer *video_player, PsvrSensors *psvr,
    PsvrControl* psvr_control, QWidget *parent): QMainWindow(parent),
    psvr_control_(psvr_control) {
	this->video_player = video_player;
	this->psvr = psvr;

	main_window = 0;

	setWindowTitle(tr("PSVR HMD"));

	resize(640, 480);

  compose_scr_.resize(kScrWidth * kScrHeight);
  LoadTestInfo();

	hmd_widget = new HMDWidget(video_player, psvr);
  info_data_ = hmd_widget->GetInfoData();
  setCentralWidget(hmd_widget);
  ShowMenu();


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
  std::ifstream f("info_test.data", std::ios_base::binary);
  if (f) {
    test_scr_.resize(kScrWidth * kScrHeight);
    f.read(reinterpret_cast<char*>(test_scr_.data()), kScrWidth * kScrHeight * sizeof(uint32_t));
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
    // Changes menu selections
    info_scr_.DoLeft();
    UpdateInformation();
  } else {
    // Make some rewind
    PlayerMakeStep(-kForwardStep);
  }
}

void HMDWindow::OnRight() {
  if (show_menu_) {
    info_scr_.DoRight();
    UpdateInformation();
  } else {
    // Make some rewind
    PlayerMakeStep(kForwardStep);
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
  if (compose_scr_.size() == test_scr_.size()) {
    memcpy(compose_scr_.data(), test_scr_.data(), compose_scr_.size() * sizeof(uint32_t));
  } else {
    std::memset(compose_scr_.data(), 0, compose_scr_.size() * sizeof(uint32_t));
  }

  auto& scr = info_scr_.GetInfoScr();
  AddTile(compose_scr_, kScrWidth, scr, info_scr_.GetInfoScrWidth(),
      kInfoScrXPos, kInfoScrYPos);

  if (kScrWidth != HMDWidget::kInfoWidth || kScrHeight != HMDWidget::kInfoHeight) {
    assert(false);
    return;
  }

  memcpy(info_data_, compose_scr_.data(), compose_scr_.size() * sizeof(uint32_t));
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
