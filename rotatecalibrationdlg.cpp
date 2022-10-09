#include "rotatecalibrationdlg.h"
#include "ui_rotatecalibrationdlg.h"

RotateCalibrationDlg::RotateCalibrationDlg(PsvrSensors* sensors, QWidget *parent) :
    QDialog(parent), ui(new Ui::RotateCalibrationDlg), sensors_(sensors) {
  ui->setupUi(this);

  connect(&update_timer_, SIGNAL(timeout()), this, SLOT(Update()), Qt::QueuedConnection);
}

RotateCalibrationDlg::~RotateCalibrationDlg()
{
  delete ui;
}

void RotateCalibrationDlg::Update()
{
  if (sensors_->IsCalibrationCompleted()) {
    accept();
    return;
  }

  auto ct = std::chrono::steady_clock::now();
  int value = std::chrono::duration_cast<std::chrono::milliseconds>(ct - start_time_).count();
  ui->bar->setValue(value);
}

void RotateCalibrationDlg::on_laid_btn__clicked()
{
  start_time_ = std::chrono::steady_clock::now();
  sensors_->StartCalibration();
  ui->laid_btn_->setDisabled(true);
  update_timer_.setInterval(300);
  update_timer_.start();
}

void RotateCalibrationDlg::on_cancel_btn__clicked() {
  sensors_->CancelCalibration();
  reject();
}
