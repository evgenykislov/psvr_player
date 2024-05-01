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

PsvrSensors::PsvrSensors(): x_velo_(0.0), y_velo_(0.0), z_velo_(0.0),
    x_angle_(0.0), y_angle_(0.0), z_angle_(0.0), run_reading_(false) {
  device_ = 0;
	memset(buffer, 0, sizeof(buffer));
  ResetView(false);
}

PsvrSensors::~PsvrSensors()
{
}

hid_device_info *PsvrSensors::EnumerateDevices()
{
	//return hid_enumerate(0x0, 0x0);
	return hid_enumerate(PSVR_VENDOR_ID, PSVR_PRODUCT_ID);
}

bool PsvrSensors::OpenDevice() {
  CloseDevice();
  assert(!device_);

  auto dev_name = GetSensorDevice();
  if (dev_name.empty()) {
    return false;
  }

  device_ = hid_open_path(dev_name.c_str());
  if (!device_) {
    return false;
  }

  run_reading_ = true;
  std::thread rthr([this](){
    while (run_reading_) {
      if (Read(kReadTimeoutMs)) {
        emit SensorUpdate();
      }

      std::this_thread::sleep_for(std::chrono::microseconds(kReadIntervalMcs));

    }
  });

  std::swap(read_thr_, rthr);

  return true;
}


void PsvrSensors::CloseDevice()
{
  if (read_thr_.joinable()) {
    run_reading_ = false;
    read_thr_.join();
  }

  if(!device_)
		return;

  hid_close(device_);
  device_ = 0;
}

bool PsvrSensors::Read(int timeout)
{
  if(!device_)
		return false;

  int size = hid_read_timeout(device_, buffer, PSVR_BUFFER_SIZE, timeout);
  auto ct = std::chrono::steady_clock::now();
	
  if(size != 64) {
    // Device is switched off
    // printf("read failed \"%S\"\n", hid_error(psvr_device));
    return false;
  }

  if (last_reading_ == decltype(last_reading_)()) {
    // last_reading_ isn't valid. reset view and store current value
    last_reading_ = ct;
    ResetView(false);
    return true;
  }

  double ims = std::chrono::duration_cast<std::chrono::microseconds>
      (ct - last_reading_).count() * 0.001;
  last_reading_ = ct;

  // printf("Read sensors in interval %.1f\n", ims);

  int16_t y_acc = read_int16(buffer, 20) + read_int16(buffer, 36);
  int16_t x_acc = read_int16(buffer, 22) + read_int16(buffer, 38);
  int16_t z_acc = read_int16(buffer, 24) + read_int16(buffer, 40);

  std::unique_lock<std::mutex> alocker(angle_lock_);
  double delta_x_angle = (-x_acc * ACCELERATION_COEF - x_velo_) * ims;
  double delta_y_angle = (-y_acc * ACCELERATION_COEF - y_velo_) * ims;
  double delta_z_angle = (-z_acc * ACCELERATION_COEF - z_velo_) * ims;
  x_angle_ += delta_x_angle;
  y_angle_ += delta_y_angle;
  z_angle_ += delta_z_angle;

  alocker.unlock();
  return true;
}

void PsvrSensors::ResetView(bool apply_compensation)
{
  auto ct = std::chrono::steady_clock::now();
  std::lock_guard<std::mutex> lk(angle_lock_);
  if (last_reset_view_ == decltype(last_reading_)()) {
    last_reset_view_ = ct;
  } else {
    // Calculates compensation
    double ims = std::chrono::duration_cast<std::chrono::microseconds>
        (ct - last_reset_view_).count() * 0.001;
    if (apply_compensation && (ims > kMinimumCompensationIntervalMs)) {
      double extra_x_velo = x_angle_ / ims;
      double extra_y_velo = y_angle_ / ims;
      double extra_z_velo = z_angle_ / ims;
      x_velo_ = x_velo_ + extra_x_velo * kCompensationRough;
      y_velo_ = y_velo_ + extra_y_velo * kCompensationRough;
      z_velo_ = z_velo_ + extra_z_velo * kCompensationRough;
    }
    last_reset_view_ = ct;
  }

  // Reset view
  x_angle_ = 0.0;
  y_angle_ = 0.0;
  z_angle_ = 0.0;
}

void PsvrSensors::GetModelViewMatrix(QMatrix4x4& matrix) {
  double x, y, z;
  std::unique_lock<std::mutex> lk(angle_lock_);
  x = x_angle_;
  y = y_angle_;
  z = z_angle_;
  lk.unlock();

  QMatrix4x4 m;
  m.setToIdentity();
  m.rotate(x, -1.0f, 0.0f, 0.0f);
  m.rotate(y, 0.0f, -1.0f, 0.0f);
  m.rotate(z, 0.0f, 0.0f, +1.0f);

  matrix = m;
}

void PsvrSensors::SetVelocity(double xvelocity, double yvelocity, double zvelocity) {
  std::lock_guard<std::mutex> lk(angle_lock_);
  x_velo_ = xvelocity;
  y_velo_ = yvelocity;
  z_velo_ = zvelocity;
}

void PsvrSensors::GetVelocity(double& xvelocity, double& yvelocity, double& zvelocity) {
  std::lock_guard<std::mutex> lk(angle_lock_);
  xvelocity = x_velo_;
  yvelocity = y_velo_;
  zvelocity = z_velo_;
}


std::string PsvrSensors::GetSensorDevice() {
  auto devs = hid_enumerate(kPsvrVendorID, kPsvrProductID);
  if (!devs) {
    return std::string();
  }

  std::string res;
  for (auto dev = devs; dev; dev = dev->next) {
    try {
      std::string p = dev->path;
      if (p.substr(p.length() - 3) == kPsvrSensorInterface) {
        res = p;
        break;
      }
    }
    catch (std::bad_alloc&) {}
    catch (std::out_of_range& ) {}
  }

  hid_free_enumeration(devs);

  return res;
}

int16_t read_int16(unsigned char *buffer, int offset)
{
	int16_t v;
	v = buffer[offset];
	v |= buffer[offset+1] << 8;
	return v;
}
