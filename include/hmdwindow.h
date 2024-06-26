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

#ifndef PSVR_HMDWINDOW_H
#define PSVR_HMDWINDOW_H

#include <QMainWindow>

#include "hmdwidget.h"

#include "info_screen.h"
#include "mainwindow.h"

#include "psvr_control.h"

class HMDWindow : public QMainWindow
{
	Q_OBJECT

	private:
		HMDWidget *hmd_widget;

		VideoPlayer *video_player;
		PsvrSensors *psvr;

		MainWindow *main_window;

	public:
    HMDWindow(VideoPlayer *video_player, PsvrSensors *psvr,
        PsvrControl* psvr_control_, QWidget *parent = 0);
		~HMDWindow();

		HMDWidget *GetHMDWidget()					{ return hmd_widget; }

		void SetMainWindow(MainWindow *main_window)	{ this->main_window = main_window; }

    void SwitchFullScreen(bool makefull);
    void ChangeFullScreen();

    void SetEyesDistance(float disp);
    void SetHorizontLevel(float horz);

    // TODO Set private, fixes update
    uint64_t media_duration_; // in ms
    uint64_t current_play_position_; // in ms

 public slots:
  void OnUp();
  void OnDown();
  void OnLeft();
  void OnRight();
  void OnSelect();


	protected:
		void closeEvent(QCloseEvent *event) Q_DECL_OVERRIDE;

 private:
  static const size_t kScrWidth = 1920;
  static const size_t kScrHeight = 1920;
  static const size_t kInfoScrXPos = 510;
  static const size_t kInfoScrYPos = 510;
  const uint64_t kBeforeEndInterval = 10000; //!< Minimal interval before end of movie after fastforward
  const uint64_t kForwardStep = 3000; //!< Интервал перемотки

  std::vector<uint32_t> test_scr_; //!< Загруженный тестовый экран. Может быть мустой массив, если тестовый экран не загружен
  std::vector<uint32_t> compose_scr_; //!< Память для объединения тестового экрана и меню

  HMDWidget::InfoTextureRow* info_data_;
  InformationScreen info_scr_;
  PsvrControl* psvr_control_;
  bool show_menu_; //!< Признак, что отображается настроечное меню

  /*! Загрузить тестовую информацию, если она есть */
  void LoadTestInfo();

  void ShowMenu();
  void UpdateInformation();
  void HideMenu();

  void ActionInvert(int value);
  void ActionPlay();

  void PlayerMakeStep(int move_ms);

 private slots:
  void PlayerPlaying();

};


#endif //PSVR_HMDWINDOW_H
