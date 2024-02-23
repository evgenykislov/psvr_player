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

#include "hmdwidget.h"

HMDWidget::HMDWidget(VideoPlayer *video_player, PsvrSensors *psvr, QWidget *parent):
    QOpenGLWidget(parent), sphere_fbo(nullptr), screen_shader(nullptr),
    cylinder_screen_(false), play_position_(0.0f), view_distance_(0)
{
  this->video_player = video_player;

  connect(this->video_player, SIGNAL(PositionChanged(float)), this, SLOT(PlayerPositionChanged(float)));

	this->psvr = psvr;

	gl = 0;

	sphere_shader = 0;
	video_tex = 0;

	fov = 80.0f;

	video_angle = 360;
	video_projection_mode = Monoscopic;
	invert_stereo = false;

	rgb_workaround = false;

  GenerateCubeVertices();
}

HMDWidget::~HMDWidget()
{
	delete video_tex;
  delete sphere_fbo;
}

void HMDWidget::SetCylinderScreen(bool value) {
  cylinder_screen_ = value;
}


static const QVector3D cube_vertices[] = {
		// back
		QVector3D(-1.0f,  1.0f, -1.0f),
		QVector3D(-1.0f, -1.0f, -1.0f),
		QVector3D( 1.0f, -1.0f, -1.0f),

		QVector3D( 1.0f, -1.0f, -1.0f),
		QVector3D( 1.0f,  1.0f, -1.0f),
		QVector3D(-1.0f,  1.0f, -1.0f),

		// front
		QVector3D( 1.0f,  1.0f,  1.0f),
		QVector3D( 1.0f, -1.0f,  1.0f),
		QVector3D(-1.0f, -1.0f,  1.0f),

		QVector3D(-1.0f, -1.0f,  1.0f),
		QVector3D(-1.0f,  1.0f,  1.0f),
		QVector3D( 1.0f,  1.0f,  1.0f),

		// left
		QVector3D(-1.0f,  1.0f,  1.0f),
		QVector3D(-1.0f, -1.0f,  1.0f),
		QVector3D(-1.0f, -1.0f, -1.0f),

		QVector3D(-1.0f, -1.0f, -1.0f),
		QVector3D(-1.0f,  1.0f, -1.0f),
		QVector3D(-1.0f,  1.0f,  1.0f),

		// right
		QVector3D( 1.0f,  1.0f, -1.0f),
		QVector3D( 1.0f, -1.0f, -1.0f),
		QVector3D( 1.0f, -1.0f,  1.0f),

		QVector3D( 1.0f, -1.0f,  1.0f),
		QVector3D( 1.0f,  1.0f,  1.0f),
		QVector3D( 1.0f,  1.0f, -1.0f),

		// top
		QVector3D(-1.0f,  1.0f, -1.0f),
		QVector3D( 1.0f,  1.0f, -1.0f),
		QVector3D( 1.0f,  1.0f,  1.0f),

		QVector3D( 1.0f,  1.0f,  1.0f),
		QVector3D(-1.0f,  1.0f,  1.0f),
		QVector3D(-1.0f,  1.0f, -1.0f),

		// bottom
		QVector3D( 1.0f, -1.0f, -1.0f),
		QVector3D(-1.0f, -1.0f, -1.0f),
		QVector3D(-1.0f, -1.0f,  1.0f),

		QVector3D(-1.0f, -1.0f,  1.0f),
		QVector3D( 1.0f, -1.0f,  1.0f),
		QVector3D( 1.0f, -1.0f, -1.0f)
	};

static const QVector2D screen_vertices[] = {
		QVector2D(-1.0f,  1.0f),
		QVector2D(-1.0f, -1.0f),
		QVector2D( 1.0f,  1.0f),
		QVector2D( 1.0f, -1.0f)
	};

void HMDWidget::initializeGL()
{
	gl = context()->functions();

	sphere_shader = new QOpenGLShaderProgram(this);
	sphere_shader->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shader/sphere.vert");
	sphere_shader->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shader/sphere.frag");
	sphere_shader->link();

	sphere_shader->bind();
	sphere_shader->bindAttributeLocation("vertex_attr", 0);

	cube_vbo.create();
	cube_vbo.bind();
	cube_vbo.setUsagePattern(QOpenGLBuffer::StaticDraw);
  cube_vbo.allocate(cube_vertices_.data(), cube_vertices_.size() * sizeof(QVector3D));

	cube_vao.create();
	cube_vao.bind();
	sphere_shader->enableAttributeArray(0);
	sphere_shader->setAttributeBuffer(0, GL_FLOAT, 0, 3);
	sphere_shader->release();
	cube_vbo.release();
	cube_vao.release();

	video_tex = new QOpenGLTexture(QOpenGLTexture::Target2D);
	video_tex->create();
	video_tex->setFormat(QOpenGLTexture::RGB8_UNorm);
	video_tex->setSize(1, 1);
	video_tex->setMinMagFilters(QOpenGLTexture::Linear, QOpenGLTexture::Linear);
  video_tex->allocateStorage(rgb_workaround ? QOpenGLTexture::BGR : QOpenGLTexture::RGB, QOpenGLTexture::PixelType::UInt8);
	video_tex->bind();
	unsigned char data[3] = { 0, 0, 0};
  video_tex->setData(rgb_workaround ? QOpenGLTexture::BGR : QOpenGLTexture::RGB, QOpenGLTexture::PixelType::UInt8, (const void *)data);


  screen_shader = new QOpenGLShaderProgram(this);
  screen_shader->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shader/screen.vert");
  screen_shader->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shader/screen.frag");
  screen_shader->link();
  screen_shader->bind();
  screen_shader->bindAttributeLocation("vertex_attr", 0);
	screen_vbo.create();
	screen_vbo.bind();
	screen_vbo.setUsagePattern(QOpenGLBuffer::StaticDraw);
	screen_vbo.allocate(screen_vertices, sizeof(screen_vertices));
	screen_vao.create();
	screen_vao.bind();

  screen_shader->enableAttributeArray(0);
  screen_shader->setAttributeBuffer(0, GL_FLOAT, 0, 2);
  screen_shader->release();

  screen_vbo.release();
  screen_vao.release();

  CreateSphereFbo(width() / 2, height());
}


void HMDWidget::resizeGL(int w, int h)
{
  CreateSphereFbo(w / 2, h);
	update();
}

void HMDWidget::paintGL()
{
	UpdateTexture();

	gl->glClear(GL_COLOR_BUFFER_BIT);
	gl->glEnable(GL_CULL_FACE);
	gl->glDisable(GL_DEPTH_TEST);

	/*gl->glViewport(0, 0, w, h);
	RenderEye(0, w, h);*/

	RenderEye(0);
	RenderEye(1);

  update();
}

void HMDWidget::GenerateCubeVertices() {
  QVector3D ltn(-1.0f,  1.0f, -1.0f);
  QVector3D lbn(-1.0f, -1.0f, -1.0f);
  QVector3D rbn( 1.0f, -1.0f, -1.0f);
  QVector3D rtn( 1.0f,  1.0f, -1.0f);
  QVector3D ltf(-1.0f,  1.0f,  1.0f);
  QVector3D lbf(-1.0f, -1.0f,  1.0f);
  QVector3D rbf( 1.0f, -1.0f,  1.0f);
  QVector3D rtf( 1.0f,  1.0f,  1.0f);

  AddFaceToVertices(ltn, lbn, rbn, rtn); // Back
  AddFaceToVertices(rtf, rbf, lbf, ltf); // Front
  AddFaceToVertices(ltf, lbf, lbn, ltn); // Left
  AddFaceToVertices(rtn, rbn, rbf, rtf); // Right
  AddFaceToVertices(ltn, rtn, rtf, ltf); // Top
  AddFaceToVertices(rbn, lbn, lbf, rbf); // Bottom

// Check with original cube_vertices.
//  assert(cube_vertices_.size() == 36);
//  float max_dif = 0.0f;
//  size_t max_index = 0;
//  for (size_t i = 0; i < 36; ++i) {
//    auto dif = (cube_vertices_[i] - cube_vertices[i]).length();
//    if (dif > max_dif) {
//      max_dif = dif;
//      max_index = i;
//    }
//  }
}

void HMDWidget::AddFaceToVertices(QVector3D p1, QVector3D p2, QVector3D p3, QVector3D p4)
{
  for (size_t i = 0; i < kTriangleFactor; ++i) {
    for (size_t j = 0; j < kTriangleFactor; ++j) {
      QVector3D s1 = ApproximateVertice(p1, p2, p3, p4,
        static_cast<double>(i) / kTriangleFactor,
        static_cast<double>(j) / kTriangleFactor);
      QVector3D s2 = ApproximateVertice(p1, p2, p3, p4,
        static_cast<double>(i + 1) / kTriangleFactor,
        static_cast<double>(j) / kTriangleFactor);
      QVector3D s3 = ApproximateVertice(p1, p2, p3, p4,
        static_cast<double>(i + 1) / kTriangleFactor,
        static_cast<double>(j + 1) / kTriangleFactor);
      QVector3D s4 = ApproximateVertice(p1, p2, p3, p4,
        static_cast<double>(i) / kTriangleFactor,
        static_cast<double>(j + 1) / kTriangleFactor);
      AddSquareToVertices(s1, s2, s3, s4);
    }
  }
}

void HMDWidget::AddSquareToVertices(QVector3D p1, QVector3D p2, QVector3D p3, QVector3D p4)
{
  cube_vertices_.push_back(p1);
  cube_vertices_.push_back(p2);
  cube_vertices_.push_back(p3);
  cube_vertices_.push_back(p3);
  cube_vertices_.push_back(p4);
  cube_vertices_.push_back(p1);
}

QVector3D HMDWidget::ApproximateVertice(QVector3D p1, QVector3D p2, QVector3D p3, QVector3D p4, float f1, float f2)
{
  QVector3D l = (p2 - p1) * f1 + p1;
  QVector3D r = (p3 - p4) * f1 + p4;
  return (r - l) * f2 + l;
}


void HMDWidget::CreateSphereFbo(int width, int height)
{
  delete sphere_fbo;
  sphere_fbo = new QOpenGLFramebufferObject(width, height); // TODO new
}

void HMDWidget::UpdateTexture()
{
  auto video_data = video_player->GetCurrentData();
  if (!video_data) {
		return;
  }

  if(video_tex->width() != video_data->GetWidth() || video_tex->height() != video_data->GetHeight())
	{
		if(video_tex->isStorageAllocated())
		{
			video_tex->destroy();
			video_tex->create();
		}
		video_tex->setFormat(QOpenGLTexture::RGB8_UNorm);
    video_tex->setSize(video_data->GetWidth(), video_data->GetHeight());
		video_tex->setMinMagFilters(QOpenGLTexture::Linear, QOpenGLTexture::Linear);
    video_tex->allocateStorage(rgb_workaround ? QOpenGLTexture::BGR : QOpenGLTexture::RGB, QOpenGLTexture::PixelType::UInt8);
	}

	video_tex->bind();
  video_tex->setData(rgb_workaround ? QOpenGLTexture::BGR : QOpenGLTexture::RGB, QOpenGLTexture::PixelType::UInt8, video_data->GetData());
}

void HMDWidget::RenderEye(int eye)
{
	int w = width();
	int h = height();

  // Use sphere frame buffer for paiting
  sphere_fbo->bind();

  gl->glViewport(0, 0, w/2, h);
  gl->glClear(GL_COLOR_BUFFER_BIT);

//	gl->glViewport(eye == 1 ? w/2 : 0, 0, w/2, h);



	sphere_shader->bind();

	QMatrix4x4 modelview_matrix;
  psvr->GetModelViewMatrix(modelview_matrix);

	QMatrix4x4 projection_matrix;
	projection_matrix.perspective(fov, ((float)(w/2)) / (float)h, 0.1f, 100.0f);
	sphere_shader->setUniformValue("modelview_projection_uni", projection_matrix * modelview_matrix);

	sphere_shader->setUniformValue("tex_uni", 0);
  sphere_shader->setUniformValue("cylinder_type", cylinder_screen_);
	video_tex->bind(0);

  int eye_inv = invert_stereo ? eye : 1 - eye;

	switch(video_projection_mode)
	{
		case Monoscopic:
			sphere_shader->setUniformValue("min_max_uv_uni", 0.0f, 0.0f, 1.0f, 1.0f);
			break;
		case OverUnder:
			if(eye_inv == 1)
				sphere_shader->setUniformValue("min_max_uv_uni", 0.0f, 0.5f, 1.0f, 1.0f);
			else
				sphere_shader->setUniformValue("min_max_uv_uni", 0.0f, 0.0f, 1.0f, 0.5f);
			break;
		case SideBySide:
			if(eye_inv == 1)
				sphere_shader->setUniformValue("min_max_uv_uni", 0.0f, 0.0f, 0.5f, 1.0f);
			else
				sphere_shader->setUniformValue("min_max_uv_uni", 0.5f, 0.0f, 1.0f, 1.0f);
			break;
	}

	sphere_shader->setUniformValue("projection_angle_factor_uni", 360.0f / (float)video_angle);

	cube_vao.bind();
  gl->glDrawArrays(GL_TRIANGLES, 0, cube_vertices_.size());
	cube_vao.release();
	sphere_shader->release();



  QOpenGLFramebufferObject::bindDefault();

  int leftpoint = int(0 - w/2 * double(view_distance_) / 1000.0);
  if (eye == 1) {
    leftpoint = int(w/2 * (1.0 + double(view_distance_) / 1000.0));
  }
  gl->glViewport(leftpoint, 0, w/2, h);

  screen_shader->bind();
  screen_shader->setUniformValue("tex_uni", 0);
  screen_shader->setUniformValue("barrel_power_uni", barrel_power);
  screen_shader->setUniformValue("show_progress", true);
  screen_shader->setUniformValue("progress_value", play_position_);


	gl->glActiveTexture(GL_TEXTURE0);
  gl->glBindTexture(GL_TEXTURE_2D, sphere_fbo->texture());

	screen_vao.bind();
	gl->glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	screen_vao.release();

  screen_shader->release();
}


void HMDWidget::PlayerPositionChanged(float pos)
{
  play_position_ = pos;
}
