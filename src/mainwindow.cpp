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

#include <QTimer>
#include <QThreadPool>
#include <QFileDialog>
#include <QMessageBox>
#include <QKeyEvent>

#include "hmdwindow.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "project_version.h"

MainWindow::MainWindow(VideoPlayer *video_player, PSVR *psvr,
    PSVRThread *psvr_thread, QWidget *parent):
    QMainWindow(parent), ui(new Ui::MainWindow), media_duration_(0),
    current_play_position_(0)
{
	this->video_player = video_player;
	this->psvr = psvr;
	this->psvr_thread = psvr_thread;

	hmd_window = 0;
	hid_device_infos = 0;

	ui->setupUi(this);

  installEventFilter(&key_filter_);
  grabKeyboard();

	setWindowTitle(windowTitle() + " " + QString(PROJECT_VERSION));
  last_directory_ = QDir::currentPath();

	player_position_delay_timer.setInterval(16);
	player_position_delay_timer.setSingleShot(true);
	connect(&player_position_delay_timer, SIGNAL(timeout()), this, SLOT(UIPlayerPositionChangedDelayed()));

	connect(psvr_thread, SIGNAL(PSVRUpdate()), this, SLOT(PSVRUpdate()));

	connect(video_player, SIGNAL(PositionChanged(float)), this, SLOT(PlayerPositionChanged(float)));
	connect(video_player, SIGNAL(Playing()), this, SLOT(PlayerPlaying()));
	connect(video_player, SIGNAL(Paused()), this, SLOT(PlayerPaused()));
  connect(video_player, SIGNAL(Stopped()), this, SLOT(PlayerStopped()));
  connect(video_player, SIGNAL(DurationParsed(unsigned int)), this,
      SLOT(PlayerDurationParsed(unsigned int)), Qt::QueuedConnection);

	connect(ui->RefreshHIDDevicesButton, SIGNAL(clicked()), this, SLOT(RefreshHIDDevices()));
	connect(ui->ConnectHIDDeviceButton, SIGNAL(clicked()), this, SLOT(ConnectPSVR()));

	connect(ui->OpenButton, SIGNAL(clicked()), this, SLOT(OpenVideoFile()));

	connect(ui->PlayButton, SIGNAL(clicked()), this, SLOT(UIPlayerPlay()));
  connect(ui->StopButton, SIGNAL(clicked()), this, SLOT(UIPlayerStop()));
	connect(ui->PlayerSlider, SIGNAL(sliderMoved(int)), this, SLOT(UIPlayerPositionChanged(int)));

  connect(&key_filter_, SIGNAL(PauseSignal()), this, SLOT(UIPlayerPlay()), Qt::QueuedConnection);
  connect(&key_filter_, SIGNAL(MakeStep(int)), this, SLOT(UIPlayerMakeStep(int)), Qt::QueuedConnection);
  connect(&key_filter_, SIGNAL(ResetView()), this, SLOT(ResetView()), Qt::QueuedConnection);
  connect(&key_filter_, SIGNAL(FullScreen()), this, SLOT(UIPlayerFullScreen()), Qt::QueuedConnection);

	connect(ui->FOVDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(FOVValueChanged(double)));
	connect(ui->ResetViewButton, SIGNAL(clicked()), this, SLOT(ResetView()));

	connect(ui->Angle360RadioButton, SIGNAL(toggled(bool)), this, SLOT(UpdateVideoAngle()));
	connect(ui->Angle180RadioButton, SIGNAL(toggled(bool)), this, SLOT(UpdateVideoAngle()));
	connect(ui->AngleCustomRadioButton, SIGNAL(toggled(bool)), this, SLOT(UpdateVideoAngle()));
	connect(ui->AngleCustomSpinBox, SIGNAL(valueChanged(double)), this, SLOT(UpdateVideoAngle()));

	connect(ui->StereoMonoscopicRadioButton, SIGNAL(toggled(bool)), this, SLOT(UpdateVideoProjection()));
	connect(ui->StereoOverUnderRadioButton, SIGNAL(toggled(bool)), this, SLOT(UpdateVideoProjection()));
	connect(ui->StereoSBSRadioButton, SIGNAL(toggled(bool)), this, SLOT(UpdateVideoProjection()));
	connect(ui->StereoInvertCheckBox, SIGNAL(toggled(bool)), this, SLOT(UpdateVideoProjection()));

	connect(ui->RGBWorkaroundCheckBox, SIGNAL(toggled(bool)), this, SLOT(SetRGBWorkaround(bool)));

	RefreshHIDDevices();
}

MainWindow::~MainWindow()
{
  releaseKeyboard();
  removeEventFilter(&key_filter_);

	delete ui;

	if(hid_device_infos)
		hid_free_enumeration(hid_device_infos);
}

void MainWindow::SetHMDWindow(HMDWindow *hmd_window)
{
	this->hmd_window = hmd_window;

	if(!hmd_window)
		return;
		
	ui->FOVDoubleSpinBox->setValue(hmd_window->GetHMDWidget()->GetFOV());

	HMDWidget *hmd_widget = hmd_window->GetHMDWidget();

	switch(hmd_widget->GetVideoAngle())
	{
		case 360:
			ui->Angle360RadioButton->setChecked(true);
			ui->AngleCustomSpinBox->setEnabled(false);
			break;
		case 180:
			ui->Angle180RadioButton->setChecked(true);
			ui->AngleCustomSpinBox->setEnabled(false);
			break;
		default:
			ui->AngleCustomRadioButton->setChecked(true);
			ui->AngleCustomSpinBox->setEnabled(true);
			ui->AngleCustomSpinBox->setValue(hmd_widget->GetVideoAngle());
			break;
	}

	switch(hmd_widget->GetVideoProjectionMode())
	{
		case HMDWidget::Monoscopic:
			ui->StereoMonoscopicRadioButton->setChecked(true);
			ui->StereoInvertCheckBox->setEnabled(false);
			break;
		case HMDWidget::OverUnder:
			ui->StereoOverUnderRadioButton->setChecked(true);
			ui->StereoInvertCheckBox->setEnabled(true);
			break;
		case HMDWidget::SideBySide:
			ui->StereoSBSRadioButton->setChecked(true);
			ui->StereoInvertCheckBox->setEnabled(true);
			break;
	}

}

void MainWindow::RefreshHIDDevices()
{
	if(hid_device_infos)
		hid_free_enumeration(hid_device_infos);

	hid_device_infos = PSVR::EnumerateDevices();

	QStringList items;
	for(struct hid_device_info *dev = hid_device_infos; dev; dev=dev->next)
	{
		QString s;		
		QTextStream(&s) << QString::fromWCharArray(dev->manufacturer_string) << " "
						<< QString::fromWCharArray(dev->product_string)
						<< " (" << QString::fromLocal8Bit(dev->path) << ")";
		items.append(s);
	}

	ui->HIDDevicesListWidget->clear();
	ui->HIDDevicesListWidget->addItems(items);
}

void MainWindow::ConnectPSVR()
{
	if(psvr_thread->isRunning())
	{
		psvr_thread->requestInterruption();
		psvr_thread->wait();
		ui->ConnectHIDDeviceButton->setText(tr("Connect"));
		return;
	}

	int index = ui->HIDDevicesListWidget->currentRow();
	if(index < 0)
		return;

	hid_device_info *dev = hid_device_infos;
	for(int i = 0; i < index; i++)
	{
		if(!dev)
			break;
		dev = dev->next;
	}

	if(!dev)
		return;

	if(psvr->Open(dev->path))
	{
    if (!psvr_control_.OpenDevice()) {
      QMessageBox::critical(this, tr("PSVR Player"), tr("Failed to connect to HID control device."));
    }
    else {
      psvr_control_.SetVRMode(true);
    }

		psvr_thread->start();
		ui->ConnectHIDDeviceButton->setText(tr("Disconnect"));
	}
	else
	{
		QMessageBox::critical(this, tr("PSVR Player"), tr("Failed to connect to HID device."));
	}
}

void MainWindow::PSVRUpdate()
{
	//ui->DebugLabel->setText(QString::asprintf("%d\t%d\t%d", psvr->GetAccelerationX(), psvr->GetAccelerationY(), psvr->GetAccelerationZ()));
}

void MainWindow::FOVValueChanged(double v)
{
	if(hmd_window)
		hmd_window->GetHMDWidget()->SetFOV((float)v);
}

void MainWindow::ResetView()
{
	psvr->ResetView();
}

void MainWindow::OpenVideoFile()
{
  QString file;
  QFileDialog dlg(this, tr("Open Video"), last_directory_, tr("All files (*)"));
  dlg.setOptions(QFileDialog::DontUseNativeDialog);
  dlg.setFileMode(QFileDialog::ExistingFile);
  if (dlg.exec()) {
    last_directory_ = dlg.directory().path();
    auto fl = dlg.selectedFiles();
    if (!fl.empty()) {
      file = fl.front();
    }
  }

  if(file.isEmpty()) {
		return;
  }

	if(!video_player->LoadVideo(QDir::toNativeSeparators(file).toLocal8Bit().constData()))
	{
		QMessageBox::critical(this, tr("PSVR Player"), tr("Failed to open video file."));
		return;
	}

	ui->PlayerControlsWidget->setEnabled(true);
}

void MainWindow::UIPlayerPlay()
{
	if(video_player->IsPlaying())
		video_player->Pause();
	else
		video_player->Play();
}

void MainWindow::UIPlayerStop()
{
	video_player->Stop();
}

void MainWindow::UIPlayerPositionChanged(int value)
{
	float pos = (float)value / (float)ui->PlayerSlider->maximum();

	if(player_position_delay_timer.isActive())
	{
		player_position_delayed = pos;
		return;
	}

	video_player->SetPosition(pos);
	player_position_delay_timer.start();
}


void MainWindow::PlayerPlaying()
{
	ui->StopButton->setEnabled(true);
	ui->PlayButton->setText(tr("Pause"));
}

void MainWindow::PlayerPaused()
{
	ui->PlayButton->setText(tr("Play"));
}

void MainWindow::PlayerStopped()
{
	ui->StopButton->setEnabled(false);
	ui->PlayerSlider->setValue(0);
	ui->PlayButton->setText(tr("Play"));
}

void MainWindow::PlayerPositionChanged(float pos)
{
	if(!ui->PlayerSlider->isSliderDown() || player_position_delay_timer.isActive())
		ui->PlayerSlider->setValue((int)((float)ui->PlayerSlider->maximum() * pos));

  current_play_position_ = static_cast<uint64_t>(media_duration_ * pos);
  ui->CurPosLabel->setText(FormatPlayTime(current_play_position_));
}


void MainWindow::PlayerDurationParsed(unsigned int dur_ms) {
  media_duration_ = dur_ms;
  ui->DurationLabel->setText(QString("/ ") + FormatPlayTime(media_duration_));
}


void MainWindow::UIPlayerPositionChangedDelayed()
{
	if(player_position_delayed >= 0.0f)
	{
		video_player->SetPosition(player_position_delayed);
		player_position_delayed = -1.0f;
	}
}


void MainWindow::UIPlayerMakeStep(int move_ms) {
  float pos = 0.0f;

  if (move_ms < 0 && current_play_position_ < static_cast<uint64_t>(-move_ms)) {
    // Goto start
  }
  else if (media_duration_ == 0) {
    // Use default value
  }
  else {
    pos = static_cast<float>(current_play_position_ + move_ms) / media_duration_;
  }

  video_player->SetPosition(pos);
}


void MainWindow::UIPlayerFullScreen() {
  if(hmd_window) {
    hmd_window->SwitchFullScreen();
  }
}


void MainWindow::UpdateVideoAngle()
{
	HMDWidget *hmd_widget = hmd_window->GetHMDWidget();

	if(ui->Angle360RadioButton->isChecked())
	{
		hmd_widget->SetVideoAngle(360);
		ui->AngleCustomSpinBox->setEnabled(false);
	}
	else if(ui->Angle180RadioButton->isChecked())
	{
		hmd_widget->SetVideoAngle(180);
		ui->AngleCustomSpinBox->setEnabled(false);
	}
	else if(ui->AngleCustomRadioButton->isChecked())
	{
		hmd_widget->SetVideoAngle((int)ui->AngleCustomSpinBox->value());
		ui->AngleCustomSpinBox->setEnabled(true);
	}
}

void MainWindow::UpdateVideoProjection()
{
	HMDWidget *hmd_widget = hmd_window->GetHMDWidget();

	if(ui->StereoMonoscopicRadioButton->isChecked())
	{
		hmd_widget->SetVideoProjectionMode(HMDWidget::Monoscopic);
		ui->StereoInvertCheckBox->setEnabled(false);
	}
	else if(ui->StereoOverUnderRadioButton->isChecked())
	{
		hmd_widget->SetVideoProjectionMode(HMDWidget::OverUnder);
		hmd_widget->SetInvertStereo(ui->StereoInvertCheckBox->isChecked());
		ui->StereoInvertCheckBox->setEnabled(true);
	}
	else if(ui->StereoSBSRadioButton->isChecked())
	{
		hmd_widget->SetVideoProjectionMode(HMDWidget::SideBySide);
		hmd_widget->SetInvertStereo(ui->StereoInvertCheckBox->isChecked());
		ui->StereoInvertCheckBox->setEnabled(true);
	}
}

void MainWindow::SetRGBWorkaround(bool enabled)
{
	hmd_window->GetHMDWidget()->SetRGBWorkaround(enabled);
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
	if(event->key() == Qt::Key_R)
	{
		psvr->ResetView();
	}
	else
	{
		QMainWindow::keyPressEvent(event);
	}
}

void MainWindow::closeEvent(QCloseEvent *event)
{
	psvr_thread->requestInterruption();
	psvr_thread->wait();

	if(hmd_window)
	{
		hmd_window->SetMainWindow(0);
		hmd_window->close();
	}

	QMainWindow::closeEvent(event);
}


QString MainWindow::FormatPlayTime(uint64_t value_ms) {
  uint64_t top = value_ms;
  top /= 100; // Removes ms and 10ms
  unsigned int ms100 = top % 10;
  top /= 10;
  unsigned int sec = top % 60;
  top /= 60;
  unsigned int min = top % 60;
  unsigned int hour = top / 60;
  QChar fc = QLatin1Char('0');
  return QString("%1:%2:%3.%4").arg(hour, 2, 10, fc).arg(min, 2, 10, fc).\
      arg(sec, 2, 10, fc).arg(ms100, 1, 10, fc);
}
