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

#ifndef PSVR_PSVR_H
#define PSVR_PSVR_H

#include <mutex>
#include <thread>

#include <hidapi/hidapi.h>
#include <QMatrix4x4>

#define PSVR_BUFFER_SIZE 64

class PsvrSensors: public QObject
{
  Q_OBJECT

	private:
    hid_device* device_;

		unsigned char buffer[PSVR_BUFFER_SIZE];

	public:
    PsvrSensors();
    ~PsvrSensors();

		static hid_device_info *EnumerateDevices();

    bool OpenDevice();
    void CloseDevice();

    bool IsOpen()					{ return device_ != 0; }

		bool Read(int timeout);

		void ResetView();

    /*! Выдаёт матрицу разворота */
    void GetModelViewMatrix(QMatrix4x4& matrix);

    /*! Выставляет сохранённые значения скорости шлема */
    void SetVelocity(double xvelocity, double yvelocity, double zvelocity);

    void GetVelocity(double& xvelocity, double& yvelocity, double& zvelocity);

 signals:
  void SensorUpdate();

 private:
  const int kReadIntervalMcs = 500;
  const int kReadTimeoutMs = 1; //!< Timeout for reading sensors from hid device. Using "-1" can cause hangup.
  const unsigned short kPsvrVendorID = 0x054c;
  const unsigned short kPsvrProductID = 0x09af;
  const char kPsvrSensorInterface[4] = ":04";

  // Compensation
  const double kCompensationRough = 0.3;
  std::chrono::steady_clock::time_point last_reading_; //!< time of last read/rotate operation. Or default value if not valid. Changed in PsvrSensors::Read only.
  std::chrono::steady_clock::time_point last_reset_view_;

  // Current angle speed (degrees per milliseconds) for axis compensation
  double x_velo_;
  double y_velo_;
  double z_velo_;

  // Текущие углы поворота шлема относительно пользователя
  // Т.е. если польователь повернул голову направо, то y_angle будет положительный.
  // Подробнее про систему координат написано в docs/Координаты.txt
  double x_angle_;
  double y_angle_;
  double z_angle_;


  int horizont_level_;

  std::mutex angle_lock_;

  std::thread read_thr_;
  std::atomic_bool run_reading_;

  std::string GetSensorDevice();

};

#endif //PSVR_PSVR_H
