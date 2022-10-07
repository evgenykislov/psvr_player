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

#include <hidapi/hidapi.h>
#include <QMatrix4x4>

#define PSVR_BUFFER_SIZE 64

class PSVR
{
	private:
		hid_device *psvr_device;

		unsigned char buffer[PSVR_BUFFER_SIZE];
		short x_acc, y_acc, z_acc;

		QMatrix4x4 modelview_matrix;

	public:
		PSVR();
		~PSVR();

		static hid_device_info *EnumerateDevices();

		bool Open(const char *path = 0);
		void Close();

		bool IsOpen()					{ return psvr_device != 0; }

		bool Read(int timeout);

		void ResetView();

		short GetAccelerationX(void)	{ return x_acc; }
		short GetAccelerationY(void)	{ return y_acc; }
		short GetAccelerationZ(void)	{ return z_acc; }

		QMatrix4x4 GetModelViewMatrix(void)	{ return modelview_matrix; }

 private:
  // Compensation
  const double kCompensationSmooth = 0.3;
  std::chrono::steady_clock::time_point last_reading_; //!< time of last read/rotate operation. Or default value if not valid
  std::chrono::steady_clock::time_point last_reset_; //!< Time of last reset operation
  double x_angle_summ;
  double y_angle_summ;
  double z_angle_summ;

  // Current angle speed (per milliseconds) for axis (compensation)
  double dx_angle;
  double dy_angle;
  double dz_angle;

  std::mutex angle_lock_;
};

#endif //PSVR_PSVR_H
