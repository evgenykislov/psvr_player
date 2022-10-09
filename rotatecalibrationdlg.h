#ifndef ROTATECALIBRATIONDLG_H
#define ROTATECALIBRATIONDLG_H

#include <QDialog>
#include <QTimer>

#include "include/psvr.h"

namespace Ui {
  class RotateCalibrationDlg;
}

class RotateCalibrationDlg : public QDialog
{
  Q_OBJECT

public:
  explicit RotateCalibrationDlg(PsvrSensors* sensors, QWidget *parent = nullptr);
  ~RotateCalibrationDlg();

private slots:
  void Update();

private slots:
  void on_laid_btn__clicked();

  void on_cancel_btn__clicked();

private:
  Ui::RotateCalibrationDlg *ui;
  PsvrSensors* sensors_;
  QTimer update_timer_;
  std::chrono::steady_clock::time_point start_time_;
};

#endif // ROTATECALIBRATIONDLG_H
