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

#ifndef PSVR_HMDWIDGET_H
#define PSVR_HMDWIDGET_H

#include <atomic>

#include <QOpenGLWidget>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLTexture>
#include <QOpenGLFunctions>
#include <QOpenGLFramebufferObject>

#include "videoplayer.h"
#include "psvr.h"

/*! Класс-виджет для обработки изображения, наложения и т.д.
В нём оборачиваются все функции по работе с OpenGL */
class HMDWidget : public QOpenGLWidget
{
	Q_OBJECT

	public:
		enum VideoProjectionMode
		{
			Monoscopic,
			OverUnder,
			SideBySide
		};

	private:
		VideoPlayer *video_player;
		PsvrSensors *psvr;


		QOpenGLFunctions *gl;

		QOpenGLShaderProgram *sphere_shader;
		QOpenGLShaderProgram *distortion_shader;

		QOpenGLBuffer cube_vbo;
		QOpenGLVertexArrayObject cube_vao;

		QOpenGLTexture *video_tex;
    std::shared_ptr<QOpenGLTexture> info_tex_; //!< Текстура с информацией: прогресс, настроики и т.д.


		/*QOpenGLBuffer screen_vbo;
		QOpenGLVertexArrayObject screen_vao;
		QOpenGLFramebufferObject *fbo;*/

		float fov;
		float barrel_power;

		int video_angle;
		VideoProjectionMode video_projection_mode;
		bool invert_stereo;

		bool rgb_workaround;
    bool cylinder_screen_;

		//void CreateFBO(int width, int height);
		void UpdateTexture();
		void RenderEye(int eye);

	public:
		HMDWidget(VideoPlayer *video_player, PsvrSensors *psvr, QWidget *parent = 0);
		~HMDWidget();

    static const int kInfoWidth = 1920;
    static const int kInfoHeight = 1920;
    using InfoTextureRow = uint32_t[kInfoWidth];

		float GetFOV()											{ return fov; }
		void SetFOV(float fov)									{ this->fov = fov; }

		int GetVideoAngle()										{ return video_angle; }
		void SetVideoAngle(int angle)							{ this->video_angle = angle; }

    void SetEyesDistance(float disp) { eyes_disp_ = disp; }

    void SetCylinderScreen(bool value);

		VideoProjectionMode GetVideoProjectionMode()			{ return video_projection_mode; }
		void SetVideoProjectionMode(VideoProjectionMode mode)	{ this->video_projection_mode = mode; }

		bool GetInvertStereo()									{ return invert_stereo; }
		void SetInvertStereo(bool invert)						{ this->invert_stereo = invert; }

		void SetRGBWorkaround(bool enabled)						{ this->rgb_workaround = enabled; }

    /*! Выдаёт указатель на данные для рисования окна информации.
    Данные представляют собой массив kInfoHeight * kInfoWidth пикселей,
    каждый пиксель 4 байта (RGBA) */
    InfoTextureRow* GetInfoData() { return info_texture_array_; }
    void ForceUpdateInfo() { force_update_info_ = true; }

    void SetHorizontLevel(float horz) { horizont_level_ = horz; }

	protected:
		void initializeGL() Q_DECL_OVERRIDE;
		void resizeGL(int w, int h) Q_DECL_OVERRIDE;
		void paintGL() Q_DECL_OVERRIDE;

 private:
  static const size_t kTriangleFactor = 32;

  std::vector<uint32_t> info_texture_data_; //!< Память под данные выделяются в конструкторе. Единожды
  InfoTextureRow* info_texture_array_; //!< Указывает на данные в info_texture_data_
  std::vector<QVector3D> cube_vertices_;


  // TODO Can make faster
  std::atomic<float> eyes_disp_; //!< Смещение для компенсации меж-глазного расстояния
  std::atomic<float> horizont_level_; //!< Смещение горизонта
  std::atomic_bool force_update_info_; //!< Признак, что нужно принудительно обновить текстуру информации


  void GenerateCubeVertices();
  void AddFaceToVertices(QVector3D p1, QVector3D p2, QVector3D p3, QVector3D p4);
  void AddSquareToVertices(QVector3D p1, QVector3D p2, QVector3D p3, QVector3D p4);
  QVector3D ApproximateVertice(QVector3D p1, QVector3D p2, QVector3D p3, QVector3D p4, float f1, float f2);

};


#endif //PSVR_HMDWIDGET_H
