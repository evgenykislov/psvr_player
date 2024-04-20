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
#include "mainwindow.h"

class HMDWindow : public QMainWindow
{
	Q_OBJECT

	private:
		HMDWidget *hmd_widget;

		VideoPlayer *video_player;
		PsvrSensors *psvr;

		MainWindow *main_window;

	public:
		HMDWindow(VideoPlayer *video_player, PsvrSensors *psvr, QWidget *parent = 0);
		~HMDWindow();

		HMDWidget *GetHMDWidget()					{ return hmd_widget; }

		void SetMainWindow(MainWindow *main_window)	{ this->main_window = main_window; }

    void SwitchFullScreen(bool makefull);
    void ChangeFullScreen();

    void SetEyesDistance(float disp);

	protected:
		void closeEvent(QCloseEvent *event) Q_DECL_OVERRIDE;

 private:
  HMDWidget::InfoTextureRow* info_data_;

  /*! Загрузить тестовую информацию, если она есть */
  void LoadTestInfo();


};


#endif //PSVR_HMDWINDOW_H
