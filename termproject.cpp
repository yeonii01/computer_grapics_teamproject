#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <Windows.h>
#include <mmsystem.h>
#include <time.h>
#include <random>
#include <gl/glew.h>
#include <gl/freeglut.h>
#include <gl/freeglut_ext.h>
#include <stdlib.h>
#include <gl/glm/glm.hpp>
#include <gl/glm/ext.hpp>
#include <gl/glm/gtc/matrix_transform.hpp>
#pragma comment(lib,"winmm.lib")

#define WINDOWX 800
#define WINDOWY 800
#define pie 3.141592

using namespace std;

random_device rd;
default_random_engine dre(rd());
uniform_real_distribution<float>uid(0, 1);
uniform_real_distribution<float>snowpos(-1, 1);
uniform_real_distribution<float>speed(0.001, 0.1);

void make_vertexShaders();
void make_fragmentShaders();
GLuint make_shaderProgram();
GLvoid InitBuffer();
GLchar* filetobuf(const char* file);
void InitShader();
void ReadObj(FILE* objFile);
void keyboard(unsigned char, int, int);
void Mouse(int button, int state, int x, int y);
void Motion(int x, int y);
void TimerFunction(int value);
GLvoid drawScene(GLvoid);
GLvoid Reshape(int w, int h);
void Playmusic();

GLuint shaderID;
GLint width, height;

GLuint vertexShader;
GLuint fragmentShader;

GLuint VAO, VBO[3];

class Line {
public:
	GLfloat p[6];
	GLfloat color[6];

	void Bind() {
		glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(p), p, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(color), color, GL_STATIC_DRAW);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
		glEnableVertexAttribArray(1);

	}
	void Draw() {
		glBindVertexArray(VAO);
		glDrawArrays(GL_LINE_STRIP, 0, 2);
	}
};

class Plane {
public:
	GLfloat p[9];
	GLfloat n[9];
	GLfloat color[9];

	void Bind() {

		glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(p), p, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(color), color, GL_STATIC_DRAW);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
		glEnableVertexAttribArray(1);


		glm::vec3 normal = glm::cross(glm::vec3(p[3] - p[0], p[4] - p[1], p[5] - p[2]), glm::vec3(p[6] - p[0], p[7] - p[1], p[8] - p[2]));
		for (int i = 0; i < 3; ++i) {
			n[(i * 3) + 0] = normal.x;
			n[(i * 3) + 1] = normal.y;
			n[(i * 3) + 2] = normal.z;
		}

		glBindBuffer(GL_ARRAY_BUFFER, VBO[2]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(n), n, GL_STATIC_DRAW);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
		glEnableVertexAttribArray(2);

	}
	void Draw() {
		glBindVertexArray(VAO);
		glDrawArrays(GL_TRIANGLES, 0, 3);
	}
};

class XZ {
public:
	float x = 0.0f;
	float y = 0.0f;
	float z = 0.0f;
	float speed = 0.0f;
};

BOOL start = TRUE;
float BackGround[] = { 0.878431, 1, 1 };
glm::vec4* vertex;
glm::vec4* face;
int faceNum = 0;
FILE* FL;
void vectoplane(Plane* p);
void planecolorset(Plane* p, int a);

//���������� ����ϴ� ������ ���
GLUquadricObj* whead, * wbody; //����� ����
GLUquadricObj* snow[100]; //��
XZ snowposxz[100];

Plane* screen;		// ����ȭ��
Plane* Road[400];	//��
Plane* Tree[100];	//tree

float light = 1.0; //�� ��Ź
float turnY = 0; //
float zsize = 0.7; //

float rxsize = 0.07f; //���� ��/2�� ��
XZ saveroad[400]; //�� ��Ͽ�

int jumpcount = 0;
const int snowsize = 30;				// �� ����
float snowx = 0.0f, snowz = 0.0f;		// ���� ����� ��ġ y�� �̵�X ���߿� ���� �߰��Ҷ� �߰�
//float lightx = -0.5f, lightz = 0.0f;		// ���� ��ġ
bool leftright = false;
bool Sscreen = true;			// ����ȭ�� ���鶧
bool music = false;				// �뷡 ���۵Ǹ鼭 �÷��� ����(���� ����ȭ�� �̿ϼ����� �̰ɷ� ��ü)
bool jump = false;

void make_vertexShaders()
{
	GLchar* vertexShaderSource;

	vertexShaderSource = filetobuf("vertex.glsl");

	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);

	GLint result;
	GLchar errorLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &result);
	if (!result)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, errorLog);
		cerr << "ERROR: vertex shader ������ ����\n" << errorLog << endl;
		exit(-1);
	}
}

void make_fragmentShaders()
{
	GLchar* fragmentShaderSource = filetobuf("23fragment.glsl");

	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);

	GLint result;
	GLchar errorLog[512];
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &result);
	if (!result)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, errorLog);
		cerr << "ERROR: fragment shader ������ ����\n" << errorLog << endl;
		exit(-1);
	}
}

GLuint make_shaderProgram()
{
	GLint result;
	GLchar errorLog[512];
	GLuint ShaderProgramID;
	ShaderProgramID = glCreateProgram(); //--- ���̴� ���α׷� �����
	glAttachShader(ShaderProgramID, vertexShader); //--- ���̴� ���α׷��� ���ؽ� ���̴� ���̱�
	glAttachShader(ShaderProgramID, fragmentShader); //--- ���̴� ���α׷��� �����׸�Ʈ ���̴� ���̱�
	glLinkProgram(ShaderProgramID); //--- ���̴� ���α׷� ��ũ�ϱ�

	glDeleteShader(vertexShader); //--- ���̴� ��ü�� ���̴� ���α׷��� ��ũ��������, ���̴� ��ü ��ü�� ���� ����
	glDeleteShader(fragmentShader);

	glGetProgramiv(ShaderProgramID, GL_LINK_STATUS, &result); // ---���̴��� �� ����Ǿ����� üũ�ϱ�
	if (!result) {
		glGetProgramInfoLog(ShaderProgramID, 512, NULL, errorLog);
		cerr << "ERROR: shader program ���� ����\n" << errorLog << endl;
		exit(-1);
	}
	glUseProgram(ShaderProgramID); //--- ������� ���̴� ���α׷� ����ϱ�
	//--- ���� ���� ���̴����α׷� ���� �� �ְ�, �� �� �Ѱ��� ���α׷��� ����Ϸ���
	//--- glUseProgram �Լ��� ȣ���Ͽ� ��� �� Ư�� ���α׷��� �����Ѵ�.
	//--- ����ϱ� ������ ȣ���� �� �ִ�.
	return ShaderProgramID;
}

void InitShader()
{
	make_vertexShaders(); //--- ���ؽ� ���̴� �����
	make_fragmentShaders(); //--- �����׸�Ʈ ���̴� �����
	shaderID = make_shaderProgram(); //--- ���̴� ���α׷� �����
}

GLvoid InitBuffer() {
	//--- VAO ��ü ���� �� ���ε�
	glGenVertexArrays(1, &VAO);
	//--- vertex data ������ ���� VBO ���� �� ���ε�.
	glGenBuffers(3, VBO);
}

void main(int argc, char** argv) //--- ������ ����ϰ� �ݹ��Լ� ���� { //--- ������ �����ϱ�
{
	srand((unsigned int)time(NULL));
	glutInit(&argc, argv); // glut �ʱ�ȭ
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH); // ���÷��� ��� ����
	glutInitWindowPosition(0, 0); // �������� ��ġ ����
	glutInitWindowSize(WINDOWX, WINDOWY); // �������� ũ�� ����
	glutCreateWindow("Example6");// ������ ����	(������ �̸�)
	//--- GLEW �ʱ�ȭ�ϱ�
	glewExperimental = GL_TRUE;
	glewInit();

	InitShader();
	InitBuffer();

	glutKeyboardFunc(keyboard);
	glutTimerFunc(5, TimerFunction, 1);
	glutDisplayFunc(drawScene); //--- ��� �ݹ� �Լ�
	glutReshapeFunc(Reshape);
	glutMainLoop();
}

GLvoid drawScene() //--- �ݹ� �Լ�: �׸��� �ݹ� �Լ� 
{
	if (start) {
		start = FALSE;
		//if (Sscreen) {
		//	//Sscreen = false;
		//	FL = fopen("cube.obj", "rt");
		//	ReadObj(FL);
		//	fclose(FL);
		//	screen = (Plane*)malloc(sizeof(Plane) * faceNum);
		//	vectoplane(screen);
		//	planecolorset(screen, 0);
		//}
		for (int i = 0; i < 10; ++i) {
			FL = fopen("cube.obj", "rt");
			ReadObj(FL);
			fclose(FL);
			Road[i] = (Plane*)malloc(sizeof(Plane) * faceNum);
			vectoplane(Road[i]);
			planecolorset(Road[i], 0);
		}
		for (int i = 0; i < 50; ++i) {
			FL = fopen("cube.obj", "rt");
			ReadObj(FL);
			fclose(FL);
			Tree[i] = (Plane*)malloc(sizeof(Plane) * faceNum);
			vectoplane(Tree[i]);
			planecolorset(Tree[i], 0);
		}
		for (int i = 50; i < 100; ++i) {
			FL = fopen("Pyramid.obj", "rt");
			ReadObj(FL);
			fclose(FL);
			Tree[i] = (Plane*)malloc(sizeof(Plane) * faceNum);
			vectoplane(Tree[i]);
			planecolorset(Tree[i], 1);
		}
		whead = gluNewQuadric();
		wbody = gluNewQuadric();
		for (int i = 0; i < 100; ++i) {
			snow[i] = gluNewQuadric();
			snowposxz[i].x = snowx + snowpos(dre);
			snowposxz[i].z = snowz + snowpos(dre);
			snowposxz[i].speed = speed(dre);
		}
	} // �ʱ�ȭ�� ������

	glEnable(GL_DEPTH_TEST);

	InitShader();

	glClearColor(BackGround[0], BackGround[1], BackGround[2], 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 	//���

	glUseProgram(shaderID);
	glBindVertexArray(VAO);// ���̴� , ���� �迭 ���

	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glm::mat4 TR = glm::mat4(1.0f);

	unsigned int vColorLocation = glGetUniformLocation(shaderID, "outColor");//���󺯰�
	unsigned int modelLocation = glGetUniformLocation(shaderID, "model");
	unsigned int viewLocation = glGetUniformLocation(shaderID, "view");
	unsigned int projLocation = glGetUniformLocation(shaderID, "projection");
	//if (Sscreen) {
	//	glm::mat4 Vw = glm::mat4(1.0f);
	//	glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f,2.0f);
	//	glm::vec3 cameraDirection = glm::vec3(0.0f, 0.0f, -2.0f);
	//	glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
	//	Vw = glm::lookAt(cameraPos, cameraDirection, cameraUp);
	//	glUniformMatrix4fv(viewLocation, 1, GL_FALSE, &Vw[0][0]);
	//}
	//else {
	//	glm::mat4 Vw = glm::mat4(1.0f);
	//	glm::vec3 cameraPos = glm::vec3(snowx + 1.5f, 1.5f, snowz + 1.5f);
	//	glm::vec3 cameraDirection = glm::vec3(snowx - 1.5f, -1.5f, snowz - 1.5f);
	//	glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
	//	Vw = glm::lookAt(cameraPos, cameraDirection, cameraUp);
	//	glUniformMatrix4fv(viewLocation, 1, GL_FALSE, &Vw[0][0]);
	//}

	/*glm::mat4 Vw = glm::mat4(1.0f);
	glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 2.0f);
	glm::vec3 cameraDirection = glm::vec3(0.0f, 0.0f, -2.0f);
	glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
	Vw = glm::lookAt(cameraPos, cameraDirection, cameraUp);
	glUniformMatrix4fv(viewLocation, 1, GL_FALSE, &Vw[0][0]);*/

	glm::mat4 Vw = glm::mat4(1.0f);
	glm::vec3 cameraPos = glm::vec3(snowx + 1.5f, 1.5f, snowz + 1.0f);
	glm::vec3 cameraDirection = glm::vec3(snowx - 1.5f, -1.5f, snowz - 1.0f);
	glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
	Vw = glm::lookAt(cameraPos, cameraDirection, cameraUp);
	glUniformMatrix4fv(viewLocation, 1, GL_FALSE, &Vw[0][0]);

	glm::mat4 Pj = glm::mat4(1.0f);
	Pj = glm::perspective(glm::radians(45.0f), (float)WINDOWX / (float)WINDOWY, 0.1f, 100.0f);
	//Pj = glm::translate(Pj, glm::vec3(0.0, 0.0, 0.0));
	glUniformMatrix4fv(projLocation, 1, GL_FALSE, &Pj[0][0]);

	TR = glm::mat4(1.0f);
	modelLocation = glGetUniformLocation(shaderID, "model");
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));

	unsigned int lightPosLocation = glGetUniformLocation(shaderID, "lightPos"); //--- lightPos �� ����: (0.0, 0.0, 5.0);
	glUniform3f(lightPosLocation, snowx, 5.0f, snowz);
	unsigned int lightColorLocation = glGetUniformLocation(shaderID, "lightColor"); //--- lightColor �� ����: (1.0, 1.0, 1.0) ���
	glUniform3f(lightColorLocation, light, light, light);

	
	for (int i = 0; i < snowsize; ++i)
	{
		TR = glm::mat4(1.0f);
		TR = glm::translate(TR, glm::vec3(snowposxz[i].x, snowposxz[i].y, snowposxz[i].z));
		TR = glm::scale(TR, glm::vec3(0.008f, 0.008f, 0.008f));
		glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
		glUniform3f(vColorLocation, 1.0f, 1.0f, 1.0f);
		gluQuadricDrawStyle(wbody, GLU_FILL);
		gluSphere(snow[i], 1.0, 50, 50); //�� ������
	}

	//TR = glm::mat4(1.0f);
	//TR = glm::scale(TR, glm::vec3(1.75f, 1.75f, 0.01f));
	//glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
	//glUniform3f(vColorLocation, 0.73f, 0.5f, 0.3f);
	//for (int i = 0; i < 12; ++i) {
	//	screen[i].Bind();
	//	screen[i].Draw();
	//}

	//treeȮ�ο�
	TR = glm::mat4(1.0f);
	TR = glm::translate(TR, glm::vec3(-0.21f, 0.08f, 0.0f));
	TR = glm::scale(TR, glm::vec3(0.07f, 0.17f, 0.07f));
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
	glUniform3f(vColorLocation, 0.73f, 0.56f, 0.56f);
	for (int i = 0; i < 12; ++i) {
		Tree[0][i].Bind();
		Tree[0][i].Draw();
	}//���� �ص�
	TR = glm::mat4(1.0f);
	TR = glm::translate(TR, glm::vec3(-0.21f, 0.17f, 0.0f));
	TR = glm::scale(TR, glm::vec3(0.15f, 0.125f, 0.15f));
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
	glUniform3f(vColorLocation, 0.4f, 0.8f, 0.66f);
	for (int i = 0; i < 6; ++i) {
		Tree[50][i].Bind();
		Tree[50][i].Draw();
	}//���� �Ӹ� ��
	TR = glm::mat4(1.0f);
	TR = glm::translate(TR, glm::vec3(-0.21f, 0.25f, 0.0f));
	TR = glm::scale(TR, glm::vec3(0.15f, 0.125f, 0.15f));
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
	glUniform3f(vColorLocation, 0.4f, 0.8f, 0.66f);
	for (int i = 0; i < 6; ++i) {
		Tree[50][i].Bind();
		Tree[50][i].Draw();
	}//���� �Ӹ� ��

	TR = glm::mat4(1.0f);
	TR = glm::translate(TR, glm::vec3(snowx, 0.035f, snowz));
	TR = glm::scale(TR, glm::vec3(0.05f, 0.05f, 0.05f));
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
	glUniform3f(vColorLocation, 0.5f, 0.5f, 0.9f);
	gluQuadricDrawStyle(wbody, GLU_FILL);
	gluSphere(wbody, 0.7, 50, 50); //����� ����
	TR = glm::mat4(1.0f);
	TR = glm::translate(TR, glm::vec3(snowx, 0.08f, snowz));
	TR = glm::scale(TR, glm::vec3(0.05f, 0.05f, 0.05f));
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
	glUniform3f(vColorLocation, 0.5f, 0.5f, 0.9f);
	gluQuadricDrawStyle(wbody, GLU_FILL);
	gluSphere(whead, 0.5, 50, 50); //����� �Ӹ�


	//������ 0���� 90�� �ݺ� - ��������̹Ƿ�
	//�ڵ� �� ����!!!! saveroad �ڵ�κ� x�� z 0���϶��� 90���϶� �ڵ尡 �ٸ�!!
	//saveroad - x 
	// ���簡 saveroad[1]�̶�� ���� ��
	//1. saveroad[0] + 1��° �κ��� z�� scale�ݰ�	   + 0��° �κ��� x�� scale�� �ݰ�  - 90��
	//2. saveroad[0] + 0��° �κ��� z�� scale�� �ݰ� - 1��° �κ��� x�� scale �ݰ� - 0��

	//saveroaed - z
	//1. saveroad[0] + 0��° �κ��� z�� scale �ݰ� + 1��° �κ��� x�� scale �ݰ� - 90��
	//2. saveroad[0] + 0��° �κ��� x�� scale �ݰ� + 1��° �κ��� z�� scale �ݰ� - 0��

	//ī�޶� ��ġ ��ȯ�ϰ� �������� 
	//glm::vec3 cameraPos = glm::vec3(1.5f, 1.5f, 1.5f);   ----- �̺κ��� z�� �ڵ� ũ�� �����ϸ鼭 Ȯ��
	//glm::vec3 cameraDirection = glm::vec3(-1.5f, -1.5f, -1.5f);  --- �̺κ��� �� �տ����� �ݴ��� ��ȣ�� z�� �� �������ֱ�(���� ���� �������� ����)
	
	//light��ġ �ٲٰ� ������ 
	//snowx���� snowz�� �ٲٸ鼭 �� ��� Ȯ���غ���

	saveroad[0].x = 0.0f; //scale���� x�κ��� �ݰ� ����صα�
	saveroad[0].z = 0.0f; //scale���� z�κ��� �ݰ� ����صα�
	TR = glm::mat4(1.0f);
	TR = glm::scale(TR, glm::vec3(rxsize * 2, 0.01f, 1.0f));
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
	glUniform3f(vColorLocation, 0.7f, 0.7f, 0.7f);
	for (int i = 0; i < 12; ++i) {
		Road[0][i].Bind();
		Road[0][i].Draw();
	} //��1
	TR = glm::mat4(1.0f);
	saveroad[1].x = saveroad[0].x + 0.15f + rxsize;
	saveroad[1].z = saveroad[0].z + 0.5f - rxsize;
	TR = glm::translate(TR, glm::vec3(-saveroad[1].x, 0.0f, -saveroad[1].z));
	TR = glm::rotate(TR, (float)glm::radians(90.0), glm::vec3(0.0f, 1.0f, 0.0f));
	TR = glm::scale(TR, glm::vec3(rxsize * 2, 0.01f, 0.3f));
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
	glUniform3f(vColorLocation, 0.7f, 0.7f, 0.7f);
	for (int i = 0; i < 12; ++i) {
		Road[1][i].Bind();
		Road[1][i].Draw();
	} //��2
	TR = glm::mat4(1.0f);
	saveroad[2].x = saveroad[1].x + 0.15f - rxsize;
	saveroad[2].z = saveroad[1].z + 0.15f + rxsize;
	TR = glm::translate(TR, glm::vec3(-saveroad[2].x, 0.0f, -saveroad[2].z));
	TR = glm::rotate(TR, (float)glm::radians(0.0), glm::vec3(0.0f, 1.0f, 0.0f));
	TR = glm::scale(TR, glm::vec3(0.14f, 0.01f, 0.3f));
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
	glUniform3f(vColorLocation, 0.7f, 0.7f, 0.7f);
	for (int i = 0; i < 12; ++i) {
		Road[2][i].Bind();
		Road[2][i].Draw();
	} //��3
	TR = glm::mat4(1.0f);
	saveroad[3].x = saveroad[2].z + 0.25f + rxsize;
	saveroad[3].z = saveroad[2].z + 0.15f - rxsize;
	TR = glm::translate(TR, glm::vec3(-saveroad[3].x + 0.5, 0.0f, -saveroad[3].z));
	TR = glm::rotate(TR, (float)glm::radians(90.0), glm::vec3(0.0f, 1.0f, 0.0f));
	TR = glm::scale(TR, glm::vec3(0.14f, 0.01f, 0.2f));
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
	glUniform3f(vColorLocation, 0.7f, 0.7f, 0.7f);
	for (int i = 0; i < 12; ++i) {
		Road[3][i].Bind();
		Road[3][i].Draw();
	} //��4
	TR = glm::mat4(1.0f);
	saveroad[4].x = saveroad[3].x + 0.1f - rxsize;
	saveroad[4].z = saveroad[3].z + 0.07f + rxsize;
	TR = glm::translate(TR, glm::vec3(-saveroad[4].x + 0.5, 0.0f, -saveroad[4].z));
	TR = glm::rotate(TR, (float)glm::radians(0.0), glm::vec3(0.0f, 1.0f, 0.0f));
	TR = glm::scale(TR, glm::vec3(0.14f, 0.01f, 0.14f));
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
	glUniform3f(vColorLocation, 0.7f, 0.7f, 0.7f);
	for (int i = 0; i < 12; ++i) {
		Road[4][i].Bind();
		Road[4][i].Draw();
	} //��5
	TR = glm::mat4(1.0f);
	saveroad[5].x = saveroad[4].x - 0.44f + rxsize;
	saveroad[5].z = saveroad[4].z + 0.07f - rxsize;
	TR = glm::translate(TR, glm::vec3(-saveroad[5].x, 0.0f, -saveroad[5].z));
	TR = glm::rotate(TR, (float)glm::radians(90.0), glm::vec3(0.0f, 1.0f, 0.0f));
	TR = glm::scale(TR, glm::vec3(0.14f, 0.01f, 0.15f));
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
	glUniform3f(vColorLocation, 0.7f, 0.7f, 0.7f);
	for (int i = 0; i < 12; ++i) {
		Road[5][i].Bind();
		Road[5][i].Draw();
	} //��6
	TR = glm::mat4(1.0f);
	saveroad[6].x = saveroad[5].x + 0.075f - rxsize;
	saveroad[6].z = saveroad[5].z + 0.07f + rxsize;
	TR = glm::translate(TR, glm::vec3(-saveroad[6].x, 0.0f, -saveroad[6].z));
	TR = glm::rotate(TR, (float)glm::radians(0.0), glm::vec3(0.0f, 1.0f, 0.0f));
	TR = glm::scale(TR, glm::vec3(0.14f, 0.01f, 0.15f));
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
	glUniform3f(vColorLocation, 0.7f, 0.7f, 0.7f);
	for (int i = 0; i < 12; ++i) {
		Road[6][i].Bind();
		Road[6][i].Draw();
	} // ��7
	TR = glm::mat4(1.0f);
	saveroad[7].x = saveroad[6].x + 0.12f + rxsize;
	saveroad[7].z = saveroad[6].z + 0.076f - rxsize;
	TR = glm::translate(TR, glm::vec3(-saveroad[7].x, 0.0f, -saveroad[7].z));
	TR = glm::rotate(TR, (float)glm::radians(90.0), glm::vec3(0.0f, 1.0f, 0.0f));
	TR = glm::scale(TR, glm::vec3(0.14f, 0.01f, 0.29f));
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
	glUniform3f(vColorLocation, 0.7f, 0.7f, 0.7f);
	for (int i = 0; i < 12; ++i) {
		Road[7][i].Bind();
		Road[7][i].Draw();
	} //��8
	TR = glm::mat4(1.0f);
	saveroad[8].x = saveroad[7].x + 0.145f - rxsize;
	saveroad[8].z = saveroad[7].z + 0.08f + rxsize;
	TR = glm::translate(TR, glm::vec3(-saveroad[8].x, 0.0f, -saveroad[8].z));
	TR = glm::rotate(TR, (float)glm::radians(0.0), glm::vec3(0.0f, 1.0f, 0.0f));
	TR = glm::scale(TR, glm::vec3(0.14f, 0.01f, 0.31f));
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
	glUniform3f(vColorLocation, 0.7f, 0.7f, 0.7f);
	for (int i = 0; i < 12; ++i) {
		Road[8][i].Bind();
		Road[8][i].Draw();
	} //��9


	glutSwapBuffers();
	glutPostRedisplay();
}

GLvoid Reshape(int w, int h) //--- �ݹ� �Լ�: �ٽ� �׸��� �ݹ� �Լ�
{
	glViewport(0, 0, w, h);
}

void keyboard(unsigned char key, int x, int y) {
	switch (key) {
	case 'a':
		leftright = !leftright;
		break;
	case 'p': // �÷���
		music = true;
		Playmusic();
		break;
	case 'q': // ����
		glutLeaveMainLoop();
		break;
	}
	glutPostRedisplay();
}

void TimerFunction(int value) {
	for (int i = 0; i < snowsize; ++i) {
		snowposxz[i].y -= snowposxz[i].speed;
		if (snowposxz[i].y <= -0.5f)
			snowposxz[i].y = 1.5f;
	}

	if (music)
	{
		if (!leftright) {
			snowz -= 0.01; // z ���̵�
			for (int i = 0; i < snowsize; ++i)
				snowposxz[i].z -= 0.01; // �� ��ġ �̵�
		}
		else {
			snowx -= 0.01; // x���̵�
			for (int i = 0; i < snowsize; ++i)
				snowposxz[i].x -= 0.01;
		}

	}
	
	glutPostRedisplay();
	glutTimerFunc(5, TimerFunction, 1);
}

void Playmusic() {

	cout << "----playing----" << endl;
	PlaySound(L"music.wav", 0, SND_FILENAME | SND_ASYNC);
}

GLchar* filetobuf(const char* file) {
	FILE* fptr;
	long length;
	char* buf;
	fptr = fopen(file, "rb");
	if (!fptr)
		return NULL;
	fseek(fptr, 0, SEEK_END);
	length = ftell(fptr);
	buf = (char*)malloc(length + 1);
	fseek(fptr, 0, SEEK_SET);
	fread(buf, length, 1, fptr);
	fclose(fptr);
	buf[length] = 0;
	return buf;
}

void ReadObj(FILE* objFile)
{
	faceNum = 0;
	//--- 1. ��ü ���ؽ� ���� �� �ﰢ�� ���� ����
	char count[100];
	char bind[100];
	int vertexNum = 0;
	while (!feof(objFile)) {
		fscanf(objFile, "%s", count);
		if (count[0] == 'v' && count[1] == '\0')
			vertexNum += 1;
		else if (count[0] == 'f' && count[1] == '\0')
			faceNum += 1;
		memset(count, '\0', sizeof(count));
	}
	int vertIndex = 0;
	int faceIndex = 0;
	vertex = (glm::vec4*)malloc(sizeof(glm::vec4) * vertexNum);
	face = (glm::vec4*)malloc(sizeof(glm::vec4) * faceNum);

	fseek(objFile, 0, 0);
	while (!feof(objFile)) {
		fscanf(objFile, "%s", bind);
		if (bind[0] == 'v' && bind[1] == '\0') {
			fscanf(objFile, "%f %f %f",
				&vertex[vertIndex].x, &vertex[vertIndex].y, &vertex[vertIndex].z);
			vertIndex++;
		}
		else if (bind[0] == 'f' && bind[1] == '\0') {
			fscanf(objFile, "%f %f %f",
				&face[faceIndex].x, &face[faceIndex].y, &face[faceIndex].z);
			int x = face[faceIndex].x - 1, y = face[faceIndex].y - 1, z = face[faceIndex].z - 1;
			faceIndex++;
		}
	}
}

void vectoplane(Plane* p) {
	for (int i = 0; i < faceNum; ++i) {
		int x = face[i].x - 1, y = face[i].y - 1, z = face[i].z - 1;
		p[i].p[0] = vertex[x].x;
		p[i].p[1] = vertex[x].y;
		p[i].p[2] = vertex[x].z;

		p[i].p[3] = vertex[y].x;
		p[i].p[4] = vertex[y].y;
		p[i].p[5] = vertex[y].z;

		p[i].p[6] = vertex[z].x;
		p[i].p[7] = vertex[z].y;
		p[i].p[8] = vertex[z].z;
	}
}

void planecolorset(Plane* p, int a) {
	/*if (a == 0) {
		for (int i = 0; i < faceNum; i += 2) {
			float R = uid(dre);
			float G = uid(dre);
			float B = uid(dre);
			for (int j = 0; j < 3; ++j) {
				p[i].color[j * 3] = R;
				p[i].color[j * 3 + 1] = G;
				p[i].color[j * 3 + 2] = B;

				p[i + 1].color[j * 3] = R;
				p[i + 1].color[j * 3 + 1] = G;
				p[i + 1].color[j * 3 + 2] = B;
			}
		}
	}
	else if (a == 1) {
		float R = uid(dre);
		float G = uid(dre);
		float B = uid(dre);
		for (int i = 0; i < 2; ++i) {
			for (int j = 0; j < 3; ++j) {
				p[i].color[j * 3] = R;
				p[i].color[j * 3 + 1] = G;
				p[i].color[j * 3 + 2] = B;
			}
		}
		for (int i = 2; i < faceNum; ++i) {
			float R = uid(dre);
			float G = uid(dre);
			float B = uid(dre);
			for (int j = 0; j < 3; ++j) {
				p[i].color[j * 3] = R;
				p[i].color[j * 3 + 1] = G;
				p[i].color[j * 3 + 2] = B;
			}
		}
	}*/
}

//TR = glm::mat4(1.0f);
//TR = glm::translate(TR, glm::vec3(-0.125f, 0.0125f, 0.0));
//TR = glm::scale(TR, glm::vec3(0.025f, 0.025f, 1.0f));
//glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
//glUniform3f(vColorLocation, 0.7f, 0.7f, 0.7f);
//for (int i = 0; i < 12; ++i) {
//	Road[2][i].Bind();
//	Road[2][i].Draw();
//} //���׵θ���
//TR = glm::mat4(1.0f);
//TR = glm::translate(TR, glm::vec3(0.125f, 0.0125f, -0.125f));
//TR = glm::scale(TR, glm::vec3(0.025f, 0.025f, 1.25f));
//glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
//glUniform3f(vColorLocation, 0.7f, 0.7f, 0.7f);
//for (int i = 0; i < 12; ++i) {
//	Road[3][i].Bind();
//	Road[3][i].Draw();
//} //���׵θ���
