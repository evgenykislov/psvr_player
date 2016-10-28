
#include "hmdwidget.h"

#include <QOpenGLContext>

#include <GL/gl.h>
#include <GL/glu.h>

HMDWidget::HMDWidget(QWidget *parent) : QOpenGLWidget(parent)
{
}

HMDWidget::~HMDWidget()
{
	delete test_shader;
}

static const QVector3D cube_vertices[] = {
		// back
		QVector3D( -1.0f,  1.0f, -1.0f), QVector3D(1.0f, 0.0f, 0.0f),
		QVector3D( -1.0f, -1.0f, -1.0f), QVector3D(0.0f, 1.0f, 0.0f),
		QVector3D(  1.0f, -1.0f, -1.0f), QVector3D(0.0f, 0.0f, 1.0f),

		QVector3D(  1.0f, -1.0f, -1.0f), QVector3D(1.0f, 0.0f, 0.0f),
		QVector3D(  1.0f,  1.0f, -1.0f), QVector3D(0.0f, 1.0f, 0.0f),
		QVector3D( -1.0f,  1.0f, -1.0f), QVector3D(0.0f, 0.0f, 1.0f),

		// front
		QVector3D(  1.0f,  1.0f,  1.0f), QVector3D(1.0f, 0.0f, 0.0f),
		QVector3D(  1.0f, -1.0f,  1.0f), QVector3D(0.0f, 1.0f, 0.0f),
		QVector3D( -1.0f, -1.0f,  1.0f), QVector3D(0.0f, 0.0f, 1.0f),

		QVector3D( -1.0f, -1.0f,  1.0f), QVector3D(1.0f, 0.0f, 0.0f),
		QVector3D( -1.0f,  1.0f,  1.0f), QVector3D(0.0f, 1.0f, 0.0f),
		QVector3D(  1.0f,  1.0f,  1.0f), QVector3D(0.0f, 0.0f, 1.0f),

		// left
		QVector3D( -1.0f,  1.0f,  1.0f), QVector3D(1.0f, 0.0f, 0.0f),
		QVector3D( -1.0f, -1.0f,  1.0f), QVector3D(0.0f, 1.0f, 0.0f),
		QVector3D( -1.0f, -1.0f, -1.0f), QVector3D(0.0f, 0.0f, 1.0f),

		QVector3D( -1.0f, -1.0f, -1.0f), QVector3D(1.0f, 0.0f, 0.0f),
		QVector3D( -1.0f,  1.0f, -1.0f), QVector3D(0.0f, 1.0f, 0.0f),
		QVector3D( -1.0f,  1.0f,  1.0f), QVector3D(0.0f, 0.0f, 1.0f),

		// right
		QVector3D(  1.0f,  1.0f, -1.0f), QVector3D(1.0f, 0.0f, 0.0f),
		QVector3D(  1.0f, -1.0f, -1.0f), QVector3D(0.0f, 1.0f, 0.0f),
		QVector3D(  1.0f, -1.0f,  1.0f), QVector3D(0.0f, 0.0f, 1.0f),

		QVector3D(  1.0f, -1.0f,  1.0f), QVector3D(1.0f, 0.0f, 0.0f),
		QVector3D(  1.0f,  1.0f,  1.0f), QVector3D(0.0f, 1.0f, 0.0f),
		QVector3D(  1.0f,  1.0f, -1.0f), QVector3D(0.0f, 0.0f, 1.0f),

		// top
		QVector3D( -1.0f,  1.0f, -1.0f), QVector3D(1.0f, 0.0f, 0.0f),
		QVector3D(  1.0f,  1.0f, -1.0f), QVector3D(0.0f, 1.0f, 0.0f),
		QVector3D(  1.0f,  1.0f,  1.0f), QVector3D(0.0f, 0.0f, 1.0f),

		QVector3D(  1.0f,  1.0f,  1.0f), QVector3D(1.0f, 0.0f, 0.0f),
		QVector3D( -1.0f,  1.0f,  1.0f), QVector3D(0.0f, 1.0f, 0.0f),
		QVector3D( -1.0f,  1.0f, -1.0f), QVector3D(0.0f, 0.0f, 1.0f),

		// bottom
		QVector3D(  1.0f, -1.0f, -1.0f), QVector3D(1.0f, 0.0f, 0.0f),
		QVector3D( -1.0f, -1.0f, -1.0f), QVector3D(0.0f, 1.0f, 0.0f),
		QVector3D( -1.0f, -1.0f,  1.0f), QVector3D(0.0f, 0.0f, 1.0f),

		QVector3D( -1.0f, -1.0f,  1.0f), QVector3D(1.0f, 0.0f, 0.0f),
		QVector3D(  1.0f, -1.0f,  1.0f), QVector3D(0.0f, 1.0f, 0.0f),
		QVector3D(  1.0f, -1.0f, -1.0f), QVector3D(0.0f, 0.0f, 1.0f)
	};

void HMDWidget::initializeGL()
{
	test_shader = new QOpenGLShaderProgram(this);
	test_shader->addShaderFromSourceFile(QOpenGLShader::Vertex, "./shader/test.vert");
	test_shader->addShaderFromSourceFile(QOpenGLShader::Fragment, "./shader/test.frag");
	test_shader->link();

	test_shader->bind();
	test_shader->bindAttributeLocation("vertex_attr", 0);
	test_shader->bindAttributeLocation("color_attr", 1);
	//modelview_projection_uni = test_shader->uniformLocation("modelview_projection_uni");

	vbo.create();
	vbo.bind();
	vbo.setUsagePattern(QOpenGLBuffer::StaticDraw);
	vbo.allocate(cube_vertices, sizeof(cube_vertices));

	vao.create();
	vao.bind();
	test_shader->enableAttributeArray(0);
	test_shader->enableAttributeArray(1);
	test_shader->setAttributeBuffer(0, GL_FLOAT, 0, 3, sizeof(QVector3D) * 2);
	test_shader->setAttributeBuffer(1, GL_FLOAT, sizeof(QVector3D), 3, sizeof(QVector3D) * 2);

	test_shader->release();
	vbo.release();
	vao.release();
}


void HMDWidget::resizeGL(int w, int h)
{
}

void HMDWidget::paintGL()
{
	int w = width();
	int h = height();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glViewport(0, 0, w, h);
	RenderEye(0);

	//glViewport(w/2, 0, w/2, h);
	//RenderEye(1);
}

void HMDWidget::RenderEye(int eye)
{
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);

	test_shader->bind();

	QMatrix4x4 modelview_matrix;
	modelview_matrix.lookAt(QVector3D(2.0f, 2.0f, 2.0f), QVector3D(0.0f, 0.0f, 0.0f), QVector3D(0.0f, 1.0f, 0.0f));

	QMatrix4x4 projection_matrix;
	projection_matrix.perspective(80.0f, (float)width() / (float)height(), 0.1f, 100.0f);
	test_shader->setUniformValue("modelview_projection_uni", projection_matrix * modelview_matrix);

	vao.bind();
	glDrawArrays(GL_TRIANGLES, 0, 36);
	vao.release();
	test_shader->release();
}