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
#include "../rotatecalibrationdlg.h"

#include "project_version.h"

MainWindow::MainWindow(VideoPlayer *video_player, PsvrSensors *psvr, QWidget *parent):
    QMainWindow(parent), ui(new Ui::MainWindow), media_duration_(0),
    current_play_position_(0), play_sc_(nullptr), play_mode_(false)
{
	this->video_player = video_player;
	this->psvr = psvr;

	hmd_window = 0;
	hid_device_infos = 0;

	ui->setupUi(this);

//  grabKeyboard();

	setWindowTitle(windowTitle() + " " + QString(PROJECT_VERSION));
  last_directory_ = QDir::currentPath();

	player_position_delay_timer.setInterval(16);
	player_position_delay_timer.setSingleShot(true);
	connect(&player_position_delay_timer, SIGNAL(timeout()), this, SLOT(UIPlayerPositionChangedDelayed()));

  connect(psvr, SIGNAL(SensorUpdate()), this, SLOT(PSVRUpdate()));

	connect(video_player, SIGNAL(PositionChanged(float)), this, SLOT(PlayerPositionChanged(float)));
  connect(video_player, SIGNAL(Playing()), this, SLOT(PlayerPlaying()), Qt::QueuedConnection);
	connect(video_player, SIGNAL(Paused()), this, SLOT(PlayerPaused()));
  connect(video_player, SIGNAL(Stopped()), this, SLOT(PlayerStopped()), Qt::QueuedConnection);
  connect(video_player, SIGNAL(DurationParsed(unsigned int)), this,
      SLOT(PlayerDurationParsed(unsigned int)), Qt::QueuedConnection);

	connect(ui->OpenButton, SIGNAL(clicked()), this, SLOT(OpenVideoFile()));

	connect(ui->PlayButton, SIGNAL(clicked()), this, SLOT(UIPlayerPlay()));
  connect(ui->StopButton, SIGNAL(clicked()), this, SLOT(UIPlayerStop()));
	connect(ui->PlayerSlider, SIGNAL(sliderMoved(int)), this, SLOT(UIPlayerPositionChanged(int)));

  connect(&key_filter_, SIGNAL(Pause()), this, SLOT(UIPlayerPlay()), Qt::QueuedConnection);
  connect(&key_filter_, SIGNAL(MakeStep(int)), this, SLOT(UIPlayerMakeStep(int)), Qt::QueuedConnection);
  connect(&key_filter_, SIGNAL(ResetView()), this, SLOT(ResetView()), Qt::QueuedConnection);
  connect(&key_filter_, SIGNAL(FullScreen()), this, SLOT(UIPlayerFullScreen()), Qt::QueuedConnection);
  connect(&key_filter_, SIGNAL(Stop()), this, SLOT(UIPlayerStopPlay()), Qt::QueuedConnection);
  connect(&key_filter_, SIGNAL(ChangeEyesDistance(int)), this, SLOT(EyesDistanceChange(int)), Qt::QueuedConnection);

	connect(ui->FOVDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(FOVValueChanged(double)));
	connect(ui->ResetViewButton, SIGNAL(clicked()), this, SLOT(ResetView()));

	connect(ui->Angle360RadioButton, SIGNAL(toggled(bool)), this, SLOT(UpdateVideoAngle()));
	connect(ui->Angle180RadioButton, SIGNAL(toggled(bool)), this, SLOT(UpdateVideoAngle()));
	connect(ui->AngleCustomRadioButton, SIGNAL(toggled(bool)), this, SLOT(UpdateVideoAngle()));
	connect(ui->AngleCustomSpinBox, SIGNAL(valueChanged(double)), this, SLOT(UpdateVideoAngle()));
  connect(ui->CylinderRBut, SIGNAL(toggled(bool)), this, SLOT(UpdateVideoAngle()));


	connect(ui->StereoMonoscopicRadioButton, SIGNAL(toggled(bool)), this, SLOT(UpdateVideoProjection()));
	connect(ui->StereoOverUnderRadioButton, SIGNAL(toggled(bool)), this, SLOT(UpdateVideoProjection()));
	connect(ui->StereoSBSRadioButton, SIGNAL(toggled(bool)), this, SLOT(UpdateVideoProjection()));
	connect(ui->StereoInvertCheckBox, SIGNAL(toggled(bool)), this, SLOT(UpdateVideoProjection()));

	connect(ui->RGBWorkaroundCheckBox, SIGNAL(toggled(bool)), this, SLOT(SetRGBWorkaround(bool)));

  connect(ui->eyes_distance_edt, SIGNAL(textEdited(QString)), this, SLOT(on_eyes_distance_edt_textChanged(QString)));

  connect(&update_timer_, SIGNAL(timeout()), this, SLOT(UpdateTimer()), Qt::QueuedConnection);

  update_timer_.setInterval(std::chrono::milliseconds(kUpdateSensorsInterval));
  update_timer_.start();

  try {
    play_sc_ = new QShortcut(this);
    play_sc_->setKey(Qt::CTRL + Qt::Key_Space);
    connect(play_sc_, SIGNAL(activated()), this, SLOT(UIPlayerStopPlay()), Qt::QueuedConnection);
  }
  catch (std::bad_alloc&) {
  }

  ShowHelmetState();
}

MainWindow::~MainWindow()
{
	delete ui;

	if(hid_device_infos)
		hid_free_enumeration(hid_device_infos);
}

void MainWindow::SetHMDWindow(HMDWindow *hmd_window)
{
  if (!hmd_window) {
    this->hmd_window->disconnect(this);
    this->hmd_window =nullptr;
    return;
  }

	this->hmd_window = hmd_window;
		
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

  connect(hmd_window, SIGNAL(Play()), this, SLOT(UIPlayerStopPlay()), Qt::QueuedConnection);

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

void MainWindow::EyesDistanceChange(int incr)
{
  bool ok;
  auto val = ui->eyes_distance_edt->text().toInt(&ok);
  if (!ok) {
    val = 0;
  }

  val += incr;
  ui->eyes_distance_edt->setText(QString("%1").arg(val));
  UpdateEyesDistance(val);
}

void MainWindow::ResetView()
{
	psvr->ResetView();
}

void MainWindow::OpenVideoFile()
{
  QString file;
  QFileDialog dlg(this, tr("Open movie"), last_directory_, tr("All files (*)"));
  dlg.setOptions(QFileDialog::DontUseNativeDialog);
  dlg.setFileMode(QFileDialog::ExistingFile);
  if (!last_file_.isEmpty()) {
    dlg.selectFile(last_file_);
  }
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

  last_file_ = file;
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


void MainWindow::UIPlayerStopPlay()
{
  if(video_player->IsPlaying()) {
    video_player->Stop();
  }
  else {
    video_player->Play();
  }
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
  if (!psvr_control_.IsOpened() && !psvr_control_.OpenDevice()) {
    QMessageBox::critical(this, tr("PSVR Player"), tr("Failed to connect to HID control device."));
  }
  else {
    psvr_control_.SetVRMode(true);
  }

  if (ui->AutoFullScreenChk->isChecked()) {
    if (hmd_window) {
      hmd_window->SwitchFullScreen(true);
    }
  }

	ui->StopButton->setEnabled(true);
	ui->PlayButton->setText(tr("Pause"));

  if (!play_mode_) {
    installEventFilter(&key_filter_);
    grabKeyboard();
    play_mode_ = true;
  }
}

void MainWindow::PlayerPaused()
{
	ui->PlayButton->setText(tr("Play"));
}

void MainWindow::PlayerStopped()
{
  if (play_mode_) {
    play_mode_ = false;
    releaseKeyboard();
    removeEventFilter(&key_filter_);
  }

	ui->StopButton->setEnabled(false);
	ui->PlayerSlider->setValue(0);
	ui->PlayButton->setText(tr("Play"));

  if (ui->AutoFullScreenChk->isChecked()) {
    if (hmd_window) {
      hmd_window->showNormal();
    }
  }

  if (psvr_control_.IsOpened() || psvr_control_.OpenDevice()) {
    psvr_control_.SetVRMode(false);
  }
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
  else if (media_duration_ <= kBeforeEndInterval) {
    // Use default value
  }
  else if (current_play_position_ + move_ms > (media_duration_ - kBeforeEndInterval)) {
    // Stay to before-end position
    pos = static_cast<float>(media_duration_ - kBeforeEndInterval) / media_duration_;
  }
  else {
    pos = static_cast<float>(current_play_position_ + move_ms) / media_duration_;
  }

  video_player->SetPosition(pos);
}


void MainWindow::UIPlayerFullScreen() {
  if(hmd_window) {
    hmd_window->ChangeFullScreen();
  }
}


void MainWindow::UpdateVideoAngle()
{
	HMDWidget *hmd_widget = hmd_window->GetHMDWidget();
  if (!hmd_widget) {
    assert(false);
    return;
  }

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

  hmd_widget->SetCylinderScreen(ui->CylinderRBut->isChecked());
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

void MainWindow::UpdateTimer() {
  if (!psvr->IsOpen()) {
    psvr->OpenDevice();
  }
  if (!psvr_control_.IsOpened()) {
    psvr_control_.OpenDevice();
  }
  ShowHelmetState();
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
  auto key = event->key();
  switch (key) {
    case Qt::Key_R:
      psvr->ResetView();
      break;
    default:
      QMainWindow::keyPressEvent(event);
  }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
  psvr->CloseDevice();

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

void MainWindow::ShowHelmetState()
{
  QString sst = "Sensors - Failed";
  QString cst = "Control - Failed";
  if (psvr->IsOpen()) {
    sst = "Sensors - OK";
  }
  if (psvr_control_.IsOpened()) {
    cst = "Control - OK";
  }

  ui->SensorsStateLbl->setText(sst);
  ui->ControlStateLbl->setText(cst);
}

void MainWindow::UpdateEyesDistance(int distance)
{
  hmd_window->GetHMDWidget()->SetEyesDistance(distance);
}

void MainWindow::on_CalibrationBtn_clicked() {
  RotateCalibrationDlg dlg(psvr, this);
  if (dlg.exec() == QDialog::Accepted) {

  }
}

void MainWindow::on_eyes_distance_edt_textChanged(const QString &arg1)
{
  bool ok;
  auto val = arg1.toInt(&ok);
  if (ok) {
    UpdateEyesDistance(val);
  }
}
