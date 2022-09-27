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

#ifndef PSVR_CONTROL_16092022_H
#define PSVR_CONTROL_16092022_H


#include <string>


/*! Class for control PSVR mode and settings
 *  Class isn't thread-safe.
*/
class PsvrControl {
 public:
  PsvrControl();
  virtual ~PsvrControl();

  bool OpenDevice();
  void CloseDevice();

  bool IsOpened();

  bool SetVRMode(bool vrmode);


 private:
  PsvrControl(const PsvrControl&) = delete;
  PsvrControl(PsvrControl&&) = delete;
  PsvrControl& operator=(const PsvrControl&) = delete;
  PsvrControl& operator=(PsvrControl&&) = delete;

  const unsigned short kPsvrVendorID = 0x054c;
  const unsigned short kPsvrProductID = 0x09af;
  const char kPsvrControlInterface[4] = ":05";
  static const size_t kMaxBufferSize = 128;

  unsigned char buffer_[kMaxBufferSize];
  void* device_; //!< Opened control device with hid_device* type. Or nullptr

  std::string GetControlDevice();
  void ReadCurrentSettings();
  void RestoreCurrentSettings();




};

#endif // PSVR_CONTROL_H
