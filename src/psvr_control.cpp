/*
 * Created by Evgeny Kislov <dev@evgenykislov.com>
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

#include "psvr_control.h"

#include <cassert>
#include <stdexcept>

#include <hidapi/hidapi.h>



PsvrControl::PsvrControl(): device_(nullptr) {

}

std::string PsvrControl::GetControlDevice() {
  auto devs = hid_enumerate(kPsvrVendorID, kPsvrProductID);
  if (!devs) {
    return std::string();
  }

  std::string res;
  for (auto dev = devs; dev; dev = dev->next) {
    try {
      std::string p = dev->path;
      if (p.substr(p.length() - 3) == kPsvrControlInterface) {
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

void PsvrControl::ReadCurrentSettings() {
  // TODO I don't know how to do this

//  buffer_[0] = 0x81;
//  buffer_[1] = 0x00;
//  buffer_[2] = 0xaa;
//  buffer_[3] = 0x08;
//  buffer_[4] = ;
//  buffer_[5] = ;
//  buffer_[6] = ;
//  buffer_[7] = ;
//  buffer_[8] = ;
//  buffer_[1] = ;
//  buffer_[1] = ;


//  auto res = hid_write((hid_device*)device_,buffer_, kMaxBufferSize);
//  int k = 0;
}

void PsvrControl::RestoreCurrentSettings() {
  // TODO I don't know how to do this
}


PsvrControl::~PsvrControl() {

}

bool PsvrControl::OpenDevice() {
  CloseDevice();
  assert(!device_);

  auto dev_name = GetControlDevice();
  if (dev_name.empty()) {
    return false;
  }

  device_ = hid_open_path(dev_name.c_str());
  if (!device_) {
    return false;
  }

  ReadCurrentSettings();

  return true;
}

void PsvrControl::CloseDevice() {
  if (device_) {
    RestoreCurrentSettings();
    hid_close((hid_device*)device_);
    device_ = nullptr;
  }
}

bool PsvrControl::SetVRMode(bool vrmode)
{
  buffer_[0] = 0x23;
  buffer_[1] = 0x00;
  buffer_[2] = 0xaa;
  buffer_[3] = 0x04;
  buffer_[4] = vrmode ? 0x01 : 0x00;
  buffer_[5] = 0x00;
  buffer_[6] = 0x00;
  buffer_[7] = 0x00;

  return hid_write((hid_device*)device_, buffer_, 8) != -1;
}
