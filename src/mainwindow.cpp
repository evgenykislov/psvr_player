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

MainWindow::MainWindow(VideoPlayer *video_player, PsvrSensors *psvr,
    PsvrControl* psvr_control, QWidget *parent):
    QMainWindow(parent), ui(new Ui::MainWindow), media_duration_(0),
    current_play_position_(0), settings_("psvr_player.conf", QSettings::IniFormat),
    auto_full_screen_(false), horizont_level_(0), psvr_control_(psvr_control)
{
	this->video_player = video_player;
	this->psvr = psvr;

	hmd_window = 0;
	hid_device_infos = 0;

  fov_ = settings_.value("fov", 80).toFloat();

  ui->setupUi(this);

  installEventFilter(&key_filter_);

	setWindowTitle(windowTitle() + " " + QString(PROJECT_VERSION));
  last_directory_ = QDir::currentPath();

	player_position_delay_timer.setInterval(16);
	player_position_delay_timer.setSingleShot(true);
	connect(&player_position_delay_timer, SIGNAL(timeout()), this, SLOT(UIPlayerPositionChangedDelayed()));

  connect(psvr, SIGNAL(SensorUpdate()), this, SLOT(PSVRUpdate()));

	connect(video_player, SIGNAL(PositionChanged(float)), this, SLOT(PlayerPositionChanged(float)));
	connect(video_player, SIGNAL(Playing()), this, SLOT(PlayerPlaying()));
	connect(video_player, SIGNAL(Paused()), this, SLOT(PlayerPaused()));
  connect(video_player, SIGNAL(Stopped()), this, SLOT(PlayerStopped()));
  connect(video_player, SIGNAL(DurationParsed(unsigned int)), this,
      SLOT(PlayerDurationParsed(unsigned int)), Qt::QueuedConnection);

	connect(ui->OpenButton, SIGNAL(clicked()), this, SLOT(OpenVideoFile()));

  connect(ui->StopButton, SIGNAL(clicked()), this, SLOT(UIPlayerStop()));
  connect(ui->PlayerSlider, SIGNAL(sliderMoved(int)), this, SLOT(UIPlayerPositionChanged(int)));

  connect(&key_filter_, SIGNAL(ResetView()), this, SLOT(ResetView()), Qt::QueuedConnection);
  connect(&key_filter_, SIGNAL(CompensateView()), this, SLOT(CompensateView()), Qt::QueuedConnection);
  connect(&key_filter_, SIGNAL(FullScreen()), this, SLOT(UIPlayerFullScreen()), Qt::QueuedConnection);
  connect(&key_filter_, SIGNAL(FullStop()), this, SLOT(UIPlayerStopPlay()), Qt::QueuedConnection);


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

  connect(&update_timer_, SIGNAL(timeout()), this, SLOT(UpdateTimer()), Qt::QueuedConnection);

  update_timer_.setInterval(std::chrono::milliseconds(kUpdateSensorsInterval));
  update_timer_.start();

  ui->FOVDoubleSpinBox->setValue(fov_);

  ShowHelmetState();

  // Скорости поворота шлема (компенсация)
  double xv, yv, zv;
  xv = settings_.value("x_velocity", 0.0).toDouble();
  yv = settings_.value("y_velocity", 0.0).toDouble();
  zv = settings_.value("z_velocity", 0.0).toDouble();
  psvr->SetVelocity(xv, yv, zv);
}

MainWindow::~MainWindow()
{
  psvr_control_->CloseDevice();
  removeEventFilter(&key_filter_);

	delete ui;

	if(hid_device_infos)
		hid_free_enumeration(hid_device_infos);

  if (psvr) {
    double xv, yv, zv;
    psvr->GetVelocity(xv, yv, zv);
    settings_.setValue("x_velocity", xv);
    settings_.setValue("y_velocity", yv);
    settings_.setValue("z_velocity", zv);
  }
}

void MainWindow::SetHMDWindow(HMDWindow* hmdwnd)
{
  if (hmd_window) {
    // Сначала отвяжем уже привязанное окно (если есть)
    hmd_window->removeEventFilter(&key_filter_);
  }

  if (!hmdwnd) {
    return;
  }

  hmd_window = hmdwnd;

  connect(&key_filter_, SIGNAL(Up()), hmd_window, SLOT(OnUp()), Qt::QueuedConnection);
  connect(&key_filter_, SIGNAL(Down()), hmd_window, SLOT(OnDown()), Qt::QueuedConnection);
  connect(&key_filter_, SIGNAL(Left()), hmd_window, SLOT(OnLeft()), Qt::QueuedConnection);
  connect(&key_filter_, SIGNAL(Right()), hmd_window, SLOT(OnRight()), Qt::QueuedConnection);
  connect(&key_filter_, SIGNAL(Select()), hmd_window, SLOT(OnSelect()), Qt::QueuedConnection);

  hmd_window->installEventFilter(&key_filter_);



  UpdateFov();

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

  auto ec = settings_.value("eyes_corr", 0).toInt();
  ui->EyesDispSpinBox->setValue(ec);
  OnEyesCorrChanged(ec);

  auto_full_screen_ = settings_.value("auto_full_screen", true).toBool();
  ui->AutoFullScreenChk->setChecked(auto_full_screen_);

  auto hl = settings_.value("horizont_level", 0).toInt();
  ui->HorizontSpinBox->setValue(hl);
  OnHorizontChanged(hl);
}


void MainWindow::PSVRUpdate()
{
	//ui->DebugLabel->setText(QString::asprintf("%d\t%d\t%d", psvr->GetAccelerationX(), psvr->GetAccelerationY(), psvr->GetAccelerationZ()));
}

void MainWindow::FOVValueChanged(double v) {
  float fv = (float)v;
  if (fov_ == fv) { return; }
  // Update value
  fov_ = fv;
  UpdateFov();
  settings_.setValue("fov", fov_);
  settings_.sync();
}

void MainWindow::ResetView()
{
  psvr->ResetView(false);
}

void MainWindow::CompensateView()
{
  psvr->ResetView(true);
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

  if (hmd_window) {
    hmd_window->showNormal();
  }

  // TODO
//  if (psvr_control_.IsOpened() || psvr_control_.OpenDevice()) {
//    psvr_control_.SetVRMode(false);
//  }
}

void MainWindow::PlayerPositionChanged(float pos)
{
	if(!ui->PlayerSlider->isSliderDown() || player_position_delay_timer.isActive())
		ui->PlayerSlider->setValue((int)((float)ui->PlayerSlider->maximum() * pos));

  current_play_position_ = static_cast<uint64_t>(media_duration_ * pos);
  hmd_window->current_play_position_ = current_play_position_;
  ui->CurPosLabel->setText(FormatPlayTime(current_play_position_));
}


void MainWindow::PlayerDurationParsed(unsigned int dur_ms) {
  media_duration_ = dur_ms;
  hmd_window->media_duration_ = media_duration_;

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


void MainWindow::UIPlayerFullScreen() {
  if(hmd_window) {
    hmd_window->ChangeFullScreen();
    hmd_window->activateWindow();
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
  if (!psvr_control_->IsOpened()) {
    psvr_control_->OpenDevice();
  }
  ShowHelmetState();
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
  if (psvr_control_->IsOpened()) {
    cst = "Control - OK";
  }

  ui->SensorsStateLbl->setText(sst);
  ui->ControlStateLbl->setText(cst);
}

void MainWindow::UpdateFov() {
  if(hmd_window) {
    hmd_window->GetHMDWidget()->SetFOV(fov_);
  }
}


void MainWindow::OnEyesCorrChanged(int value)
{
  if (!hmd_window) { return; }
  hmd_window->SetEyesDistance(value * kEyeCorrFactor);
  settings_.setValue("eyes_corr", value);
  settings_.sync();

  const float kScreenFactor = 32.0f;
  int av = static_cast<int>(68.0f - value * kEyeCorrFactor * kScreenFactor);
  ui->EyesDistanceLabel->setText(QString("about %1 mm").arg(av));
}

void MainWindow::OnAutoFullScreenChanged(int value) {
  auto_full_screen_ = value != 0;
  settings_.setValue("auto_full_screen", auto_full_screen_);
  settings_.sync();
}

void MainWindow::OnHorizontChanged(int value) {
  horizont_level_ = value;
  if (hmd_window) {
    hmd_window->SetHorizontLevel(value * kHorizontCorrFactor);
  }
  settings_.setValue("horizont_level", horizont_level_);
  settings_.sync();
}

