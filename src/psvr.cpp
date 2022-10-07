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

#include <stdio.h>
#include <hidapi/hidapi.h>
#include <cstdint>
#include <cstring>

#include "psvr.h"



#define MAX_STR			255

int16_t read_int16(unsigned char *buffer, int offset);

#define PSVR_VENDOR_ID	0x054c
#define PSVR_PRODUCT_ID	0x09af

#define ACCELERATION_COEF 0.00003125f

PSVR::PSVR(): x_angle_summ(0.0), y_angle_summ(0.0), z_angle_summ(0.0),
    dx_angle(0.0), dy_angle(0.0), dz_angle(0.0) {
	psvr_device = 0;
	memset(buffer, 0, sizeof(buffer));
	modelview_matrix.setToIdentity();
}

PSVR::~PSVR()
{
}

hid_device_info *PSVR::EnumerateDevices()
{
	//return hid_enumerate(0x0, 0x0);
	return hid_enumerate(PSVR_VENDOR_ID, PSVR_PRODUCT_ID);
}

bool PSVR::Open(const char *path)
{
	if(path)
		psvr_device = hid_open_path(path);
	else
		psvr_device = hid_open(PSVR_VENDOR_ID, PSVR_PRODUCT_ID, 0);

	if(!psvr_device)
	{
		fprintf(stderr, "Failed to open PSVR HID device.\n");
		return false;
	}

	/*int r;
	wchar_t wstr[MAX_STR];

	r = hid_get_manufacturer_string(psvr_device, wstr, MAX_STR);
	if(r > 0)
		printf("Manufacturer: %ls\n", wstr);

	r = hid_get_product_string(psvr_device, wstr, MAX_STR);
	if(r > 0)
		printf("Product: %ls\n", wstr);

	r = hid_get_serial_number_string(psvr_device, wstr, MAX_STR);
	if(r > 0)
		printf("SN: %ls\n", wstr);*/

	//hid_set_nonblocking(psvr_device, 1);

	return true;
}


void PSVR::Close()
{
	if(!psvr_device)
		return;

	hid_close(psvr_device);
	psvr_device = 0;
}

bool PSVR::Read(int timeout)
{
	if(!psvr_device)
		return false;

	int size = hid_read_timeout(psvr_device, buffer, PSVR_BUFFER_SIZE, timeout);
  auto ct = std::chrono::steady_clock::now();
	
  if(size != 64) {
    // printf("read failed \"%S\"\n", hid_error(psvr_device));
    return false;
  }

  if (last_reading_ == decltype(last_reading_)()) {
    last_reading_ = ct;
    ResetView();
    return true;
  }

  double ims = std::chrono::duration_cast<std::chrono::microseconds>
      (ct - last_reading_).count() * 0.001;
  last_reading_ = ct;

  x_acc = read_int16(buffer, 20) + read_int16(buffer, 36);
  y_acc = read_int16(buffer, 22) + read_int16(buffer, 38);
  z_acc = read_int16(buffer, 24) + read_int16(buffer, 40);

  std::unique_lock<std::mutex> locker(angle_lock_);
  double x_angle = (-x_acc * ACCELERATION_COEF - dx_angle) * ims;
  double y_angle = (-y_acc * ACCELERATION_COEF - dy_angle) * ims;
  double z_angle = (z_acc * ACCELERATION_COEF - dz_angle) * ims;
  x_angle_summ += x_angle;
  y_angle_summ += y_angle;
  z_angle_summ += z_angle;
  locker.unlock();

  modelview_matrix.rotate(y_angle, QVector3D(1.0, 0.0, 0.0) * modelview_matrix);
  modelview_matrix.rotate(x_angle, QVector3D(0.0, 1.0, 0.0) * modelview_matrix);
  modelview_matrix.rotate(z_angle, QVector3D(0.0, 0.0, 1.0) * modelview_matrix);

  return true;
}

void PSVR::ResetView()
{
  modelview_matrix.setToIdentity();

  auto ct = std::chrono::steady_clock::now();
  if (last_reset_ == decltype(last_reset_)()) {
    last_reset_ = ct;
    return;
  }

  double ims = std::chrono::duration_cast<std::chrono::microseconds>
      (ct - last_reset_).count() * 0.001;
  last_reset_ = ct;

  std::unique_lock<std::mutex> locker(angle_lock_);
  printf("xan: %.1f, yan: %.1f, zan: %.1f\n", x_angle_summ, y_angle_summ, z_angle_summ);
  dx_angle += x_angle_summ / ims * kCompensationSmooth;
  dy_angle += y_angle_summ / ims * kCompensationSmooth;
  dz_angle += z_angle_summ / ims * kCompensationSmooth;
  x_angle_summ = 0.0;
  y_angle_summ = 0.0;
  z_angle_summ = 0.0;
  locker.unlock();
}

int16_t read_int16(unsigned char *buffer, int offset)
{
	int16_t v;
	v = buffer[offset];
	v |= buffer[offset+1] << 8;
	return v;
}
