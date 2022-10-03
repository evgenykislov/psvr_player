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

#ifndef PSVR_VIDEOPLAYER_H
#define PSVR_VIDEOPLAYER_H

#include <memory>
#include <mutex>

#include <QObject>
#include <QMutex>

#include <vlc/vlc.h>

/*! Information about pixel data */
class VideoDataInfo {
 public:
  VideoDataInfo(unsigned int width, unsigned int height) {
    width_ = width;
    height_ = height;
    data_.resize(width_ * height_ * 3);
  }

  unsigned char* GetData() { return data_.data(); }
  unsigned int GetWidth() { return width_; }
  unsigned int GetHeight() { return height_; }

 private:
  VideoDataInfo() = delete;
  VideoDataInfo(const VideoDataInfo&) = delete;
  VideoDataInfo& operator=(const VideoDataInfo&) = delete;

  unsigned int width_;
  unsigned int height_;
  std::vector<unsigned char> data_;
};

using VideoDataInfoPtr = std::shared_ptr<VideoDataInfo>;


class VideoPlayer : public QObject
{
	Q_OBJECT

	private:
    const static int kParseTimeout = 0;

		libvlc_instance_t *libvlc;
		libvlc_media_t *media;
		libvlc_media_player_t *media_player;
		libvlc_event_manager_t *event_manager;

    VideoDataInfoPtr current_data_; // Current data for playing
    std::mutex current_data_lock_; // Lock for current_data_ only
    VideoDataInfoPtr vlc_locked_data_; // Data, locked by vlc player. Set new value in vlc callback only

		void UnloadVideo();
    void OnParsed();

	public:
		VideoPlayer(QObject *parent = 0);
		~VideoPlayer();

		bool LoadVideo(const char *path);

		void Play();
		void Pause();
		void Stop();
		void SetPosition(float pos);

		bool IsPlaying();


		void *VLC_Lock(void **p_pixels);
		void VLC_Unlock(void *id, void *const *p_pixels);
		void VLC_Display(void *id);

		unsigned int VLC_Setup(char *chroma, unsigned int *width, unsigned int *height, unsigned int *pitches, unsigned int *lines);
		void VLC_Cleanup();

		void VLC_Event(const struct libvlc_event_t *event);

    void SetCurrentData(VideoDataInfoPtr d);
    VideoDataInfoPtr GetCurrentData();


	signals:
		void DisplayVideoFrame();

		void Playing();
		void Paused();
		void Stopped();
		void PositionChanged(float pos);
    void DurationParsed(unsigned int dur_ms);
};

#endif //PSVR_VIDEOPLAYER_H
