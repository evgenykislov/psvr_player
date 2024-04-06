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

#ifndef PSVR_MAINWINDOW_H
#define PSVR_MAINWINDOW_H

#include <QMainWindow>
#include <QSettings>
#include <QTimer>

#include "key_filter.h"
#include "videoplayer.h"
#include "psvr.h"
#include "psvr_control.h"

namespace Ui
{
	class MainWindow;
}

class HMDWindow;

class MainWindow : public QMainWindow
{
	Q_OBJECT

	private:
		Ui::MainWindow *ui;

		struct hid_device_info *hid_device_infos;

		VideoPlayer *video_player;
    PsvrSensors *psvr;

		HMDWindow *hmd_window;

		float player_position_delayed;
		QTimer player_position_delay_timer;

	public:
    MainWindow(VideoPlayer *video_player, PsvrSensors *psvr, QWidget *parent = 0);
		~MainWindow();

		void SetHMDWindow(HMDWindow *hmd_window);

	protected slots:
		void PSVRUpdate();
		void FOVValueChanged(double v);

		void ResetView();

		void OpenVideoFile();

		void UIPlayerPlay();
		void UIPlayerStop();
    void UIPlayerStopPlay();
		void UIPlayerPositionChanged(int value);

		void UIPlayerPositionChangedDelayed();

    void UIPlayerMakeStep(int move_ms);
    void UIPlayerFullScreen();

		void PlayerPlaying();
		void PlayerPaused();
		void PlayerStopped();
		void PlayerPositionChanged(float pos);
    void PlayerDurationParsed(unsigned int dur_ms);

		void UpdateVideoAngle();
		void UpdateVideoProjection();

		void SetRGBWorkaround(bool enabled);

    void UpdateTimer();

	protected:
		void keyPressEvent(QKeyEvent *event) Q_DECL_OVERRIDE;
		void closeEvent(QCloseEvent *event) Q_DECL_OVERRIDE;

 private slots:
  void on_CalibrationBtn_clicked();
  void OnEyesCorrChanged(int);

 private:
  const int kUpdateSensorsInterval = 2000;
  const uint64_t kBeforeEndInterval = 10000; //!< Minimal interval before end of movie after fastforward
  const float kEyeCorrFactor = -0.015f;

  PsvrControl psvr_control_;

  KeyFilter key_filter_;
  uint64_t media_duration_; // in ms
  uint64_t current_play_position_; // in ms
  QString last_directory_;
  QString last_file_;
  QSettings settings_;

  QString FormatPlayTime(uint64_t value_ms);
  void ShowHelmetState();


  QTimer update_timer_;
};


#endif //PSVR_MAINWINDOW_H
