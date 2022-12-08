#define _CRT_SECURE_NO_WARNINGS
#pragma comment(lib, "glew32.lib")
#pragma comment(lib, "freeglut.lib")
#include <iostream>
#include <Windows.h>
#include <mmsystem.h>
#include <time.h>
#include <random>
#include <fstream>
#include <sstream>
#include <string>
#include <cstdlib>
#include <vector>
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

default_random_engine dre;
uniform_real_distribution<float>uid(0, 1);
uniform_real_distribution<float>snowpos(-1, 1);		// 눈 위치
uniform_real_distribution<float>speed(0.001, 0.1);
uniform_real_distribution<float>roadlength(0.1, 0.35);

void make_vertexShaders();
void make_fragmentShaders();
GLuint make_shaderProgram();
GLvoid InitBuffer();
GLchar* filetobuf(const char* file);
void InitShader();
void ReadObj(FILE* objFile);
void ReadObj2(string file, vector<glm::vec3>& vertexInfo);
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

GLuint cubeVAO, cubeVBO;

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

vector<glm::vec3> vvertex;
vector<glm::vec3> vcolor;
vector<glm::ivec3> vface;

//실질적으로 사용하는 변수들 기록
GLUquadricObj* whead, * wbody; //눈사람 선언
GLUquadricObj* snow[100]; //눈
XZ snowposxz[100];		  // 눈

Plane* Screen;		// 시작화면
Plane* Road[100];	//길
Plane* Tree[2];		//tree
Plane* Hat;			// 모자

float light = 1.0;	//빛 백탁

float rxsize = 0.083f;	//길의 폭/2한 값
float rzsize[100];
float rxsize2 = 0.083f;	//길의 폭/2한 값
float rzsize2[100];

XZ saveroad[100]; //길 기록용

XZ lEffect; // 왼쪽 이펙트 기록용
XZ rEffect; // 오른쪽 이펙트 기록용

int jumpcount = 0;
const int snowsize = 30;				// 눈 개수
float snowx = 0.0f, snowy =0.035f, snowz = 0.0f;		// 현재 눈사람 위치 y축 이동X 나중에 점프 추가할때 추가
//float lightx = -0.5f, lightz = 0.0f;		// 조명 위치
bool leftright = false;
bool Sscreen = true;			// 시작화면 만들때
bool music = false;				// 노래 시작되면서 플레이 시작(아직 시작화면 미완성으로 이걸로 대체)
bool bjump = false;				// 점프
bool cameracheck = false;

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
		cerr << "ERROR: vertex shader 컴파일 실패\n" << errorLog << endl;
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
		cerr << "ERROR: fragment shader 컴파일 실패\n" << errorLog << endl;
		exit(-1);
	}
}

GLuint make_shaderProgram()
{
	GLint result;
	GLchar errorLog[512];
	GLuint ShaderProgramID;
	ShaderProgramID = glCreateProgram(); //--- 세이더 프로그램 만들기
	glAttachShader(ShaderProgramID, vertexShader); //--- 세이더 프로그램에 버텍스 세이더 붙이기
	glAttachShader(ShaderProgramID, fragmentShader); //--- 세이더 프로그램에 프래그먼트 세이더 붙이기
	glLinkProgram(ShaderProgramID); //--- 세이더 프로그램 링크하기

	glDeleteShader(vertexShader); //--- 세이더 객체를 세이더 프로그램에 링크했음으로, 세이더 객체 자체는 삭제 가능
	glDeleteShader(fragmentShader);

	glGetProgramiv(ShaderProgramID, GL_LINK_STATUS, &result); // ---세이더가 잘 연결되었는지 체크하기
	if (!result) {
		glGetProgramInfoLog(ShaderProgramID, 512, NULL, errorLog);
		cerr << "ERROR: shader program 연결 실패\n" << errorLog << endl;
		exit(-1);
	}
	glUseProgram(ShaderProgramID); //--- 만들어진 세이더 프로그램 사용하기
	//--- 여러 개의 세이더프로그램 만들 수 있고, 그 중 한개의 프로그램을 사용하려면
	//--- glUseProgram 함수를 호출하여 사용 할 특정 프로그램을 지정한다.
	//--- 사용하기 직전에 호출할 수 있다.
	return ShaderProgramID;
}

void InitShader()
{
	make_vertexShaders(); //--- 버텍스 세이더 만들기
	make_fragmentShaders(); //--- 프래그먼트 세이더 만들기
	shaderID = make_shaderProgram(); //--- 세이더 프로그램 만들기
}

GLvoid InitBuffer() {
	//--- VAO 객체 생성 및 바인딩
	glGenVertexArrays(1, &VAO);
	//--- vertex data 저장을 위한 VBO 생성 및 바인딩.
	glGenBuffers(3, VBO);

	//ReadObj2("tree.obj", vvertex);
	glGenVertexArrays(1, &cubeVAO);
	glGenBuffers(1, &cubeVBO);


}

void main(int argc, char** argv) //--- 윈도우 출력하고 콜백함수 설정 { //--- 윈도우 생성하기
{
	srand((unsigned int)time(NULL));
	glutInit(&argc, argv); // glut 초기화
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH); // 디스플레이 모드 설정
	glutInitWindowPosition(0, 0); // 윈도우의 위치 지정
	glutInitWindowSize(WINDOWX, WINDOWY); // 윈도우의 크기 지정
	glutCreateWindow("Example6");// 윈도우 생성	(윈도우 이름)
	//--- GLEW 초기화하기
	glewExperimental = GL_TRUE;
	glewInit();

	InitShader();
	InitBuffer();

	glutKeyboardFunc(keyboard);
	glutTimerFunc(5, TimerFunction, 1);
	glutDisplayFunc(drawScene); //--- 출력 콜백 함수
	glutReshapeFunc(Reshape);
	glutMainLoop();
}

GLvoid drawScene() //--- 콜백 함수: 그리기 콜백 함수 
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
		for (int i = 0; i < 80; ++i) {
			FL = fopen("cube.obj", "rt");
			ReadObj(FL);
			fclose(FL);
			Road[i] = (Plane*)malloc(sizeof(Plane) * faceNum);
			vectoplane(Road[i]);
			planecolorset(Road[i], 0);
		}
		
		FL = fopen("cube.obj", "rt");
		ReadObj(FL);
		fclose(FL);
		Tree[0] = (Plane*)malloc(sizeof(Plane) * faceNum);
		vectoplane(Tree[0]);
		planecolorset(Tree[0], 0);		

		FL = fopen("Pyramid.obj", "rt");
		ReadObj(FL);
		fclose(FL);
		Tree[1] = (Plane*)malloc(sizeof(Plane) * faceNum);
		vectoplane(Tree[1]);
		planecolorset(Tree[1], 1);
		whead = gluNewQuadric();
		wbody = gluNewQuadric();

		FL = fopen("Pyramid.obj", "rt");
		ReadObj(FL);
		fclose(FL);
		Hat = (Plane*)malloc(sizeof(Plane) * faceNum);
		vectoplane(Hat);
		planecolorset(Hat, 0);

		for (int i = 0; i < 20; ++i) {
			FL = fopen("cube.obj", "rt");
			ReadObj(FL);
			fclose(FL);
			Road[i] = (Plane*)malloc(sizeof(Plane) * faceNum);
			vectoplane(Road[i]);
			planecolorset(Road[i], 0);
		}

		for (int i = 0; i < 100; ++i) {
			snow[i] = gluNewQuadric();
			snowposxz[i].x = snowx + snowpos(dre);
			
			snowposxz[i].z = snowz + snowpos(dre);
			snowposxz[i].speed = speed(dre);
			rzsize[i] = roadlength(dre);
		}

	} // 초기화할 데이터

	glEnable(GL_DEPTH_TEST);

	InitShader();

	glClearColor(BackGround[0], BackGround[1], BackGround[2], 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 	//배경

	glUseProgram(shaderID);
	glBindVertexArray(VAO);// 쉐이더 , 버퍼 배열 사용

	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glm::mat4 TR = glm::mat4(1.0f);

	unsigned int vColorLocation = glGetUniformLocation(shaderID, "outColor");//색상변경
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

	// 카메라
	if (cameracheck){
		glm::mat4 Vw = glm::mat4(1.0f);
		glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 2.0f);
		glm::vec3 cameraDirection = glm::vec3(0.0f, 0.0f, -2.0f);
		glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
		Vw = glm::lookAt(cameraPos, cameraDirection, cameraUp);
		glUniformMatrix4fv(viewLocation, 1, GL_FALSE, &Vw[0][0]);
	}
	else {
		glm::mat4 Vw = glm::mat4(1.0f);
		glm::vec3 cameraPos = glm::vec3(snowx + 1.5f, 1.5f, snowz + 1.0f);
		glm::vec3 cameraDirection = glm::vec3(snowx - 1.5f, -1.5f, snowz - 1.0f);
		glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
		Vw = glm::lookAt(cameraPos, cameraDirection, cameraUp);
		glUniformMatrix4fv(viewLocation, 1, GL_FALSE, &Vw[0][0]);

	}	

	glm::mat4 Pj = glm::mat4(1.0f);
	Pj = glm::perspective(glm::radians(45.0f), (float)WINDOWX / (float)WINDOWY, 0.1f, 100.0f);
	//Pj = glm::translate(Pj, glm::vec3(0.0, 0.0, 0.0));
	glUniformMatrix4fv(projLocation, 1, GL_FALSE, &Pj[0][0]);

	TR = glm::mat4(1.0f);
	modelLocation = glGetUniformLocation(shaderID, "model");
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));

	int lightPosLocation = glGetUniformLocation(shaderID, "lightPos"); //--- lightPos 값 전달: (0.0, 0.0, 5.0);
	//glUniform3f(lightPosLocation, snowx, 1.0f, snowz);
	glUniform3f(lightPosLocation, snowx, 0.5f, snowz - 0.5f);
	int lightColorLocation = glGetUniformLocation(shaderID, "lightColor"); //--- lightColor 값 전달: (1.0, 1.0, 1.0) 백색
	glUniform3f(lightColorLocation, light, light, light);

	for (int i = 0; i < snowsize; ++i)
	{
		TR = glm::mat4(1.0f);
		TR = glm::translate(TR, glm::vec3(snowposxz[i].x, snowposxz[i].y, snowposxz[i].z));
		TR = glm::scale(TR, glm::vec3(0.008f, 0.008f, 0.008f));
		glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
		glUniform3f(vColorLocation, 1.0f, 1.0f, 1.0f);
		gluQuadricDrawStyle(wbody, GLU_FILL);
		gluSphere(snow[i], 1.0, 50, 50); //눈 내리기
	}

	//TR = glm::mat4(1.0f);
	//TR = glm::scale(TR, glm::vec3(1.75f, 1.75f, 0.01f));
	//glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
	//glUniform3f(vColorLocation, 0.73f, 0.5f, 0.3f);
	//for (int i = 0; i < 12; ++i) {
	//	screen[i].Bind();
	//	screen[i].Draw();
	//}

	//tree확인용
	TR = glm::mat4(1.0f);
	TR = glm::translate(TR, glm::vec3(-0.21f, 0.08f, 0.0f));
	TR = glm::scale(TR, glm::vec3(0.07f, 0.17f, 0.07f));
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
	glUniform3f(vColorLocation, 0.73f, 0.56f, 0.56f);
	for (int i = 0; i < 12; ++i) {
		Tree[0][i].Bind();
		Tree[0][i].Draw();
	}//나무 밑둥
	TR = glm::mat4(1.0f);
	TR = glm::translate(TR, glm::vec3(-0.21f, 0.17f, 0.0f));
	TR = glm::scale(TR, glm::vec3(0.15f, 0.125f, 0.15f));
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
	glUniform3f(vColorLocation, 0.4f, 0.8f, 0.66f);
	for (int i = 0; i < 6; ++i) {
		Tree[1][i].Bind();
		Tree[1][i].Draw();
	}//나무 머리 밑
	TR = glm::mat4(1.0f);
	TR = glm::translate(TR, glm::vec3(-0.21f, 0.25f, 0.0f));
	TR = glm::scale(TR, glm::vec3(0.15f, 0.125f, 0.15f));
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
	glUniform3f(vColorLocation, 0.4f, 0.8f, 0.66f);
	for (int i = 0; i < 6; ++i) {
		Tree[1][i].Bind();
		Tree[1][i].Draw();
	}//나무 머리

	TR = glm::mat4(1.0f);
	TR = glm::translate(TR, glm::vec3(snowx, snowy, snowz));
	TR = glm::scale(TR, glm::vec3(0.05f, 0.05f, 0.05f));
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
	glUniform3f(vColorLocation, 1.0f, 0.980392, 0.941176);
	gluQuadricDrawStyle(wbody, GLU_FILL);
	gluSphere(wbody, 0.7, 50, 50); //눈사람 몸통

	TR = glm::mat4(1.0f);
	TR = glm::translate(TR, glm::vec3(snowx, snowy+ 0.045, snowz));
	TR = glm::scale(TR, glm::vec3(0.05f, 0.05f, 0.05f));
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
	glUniform3f(vColorLocation, 1.0f, 0.937255f, 0.835294f);
	gluQuadricDrawStyle(wbody, GLU_FILL);
	gluSphere(whead, 0.5, 50, 50); //눈사람 머리

	TR = glm::mat4(1.0f);
	TR = glm::translate(TR, glm::vec3(snowx, snowy + 0.083, snowz));
	//TR = glm::rotate(TR, (float)glm::radians(10.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	TR = glm::scale(TR, glm::vec3(0.03f, 0.03f, 0.03f));
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
	glUniform3f(vColorLocation, 1.0, 0.0f, 0.0f);
	for (int i = 0; i < 6; ++i) {
		Hat[i].Bind();
		Hat[i].Draw();
	}//모자 
	TR = glm::mat4(1.0f);
	TR = glm::translate(TR, glm::vec3(snowx, snowy + 0.093, snowz));
	TR = glm::scale(TR, glm::vec3(0.02f, 0.02f, 0.02f));
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
	glUniform3f(vColorLocation, 1.0f, 1.0f, 1.0f);
	gluQuadricDrawStyle(wbody, GLU_FILL);
	gluSphere(whead, 0.5, 50, 50); // 모자 꽁지

	TR = glm::mat4(1.0f);
	TR = glm::translate(TR, glm::vec3(snowx - 0.008, snowy + 0.049, snowz + 0.023));
	TR = glm::scale(TR, glm::vec3(0.015f, 0.015f, 0.015f));
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
	glUniform3f(vColorLocation, 0.0f, 0.0f, 0.0f);
	gluQuadricDrawStyle(wbody, GLU_FILL);
	gluSphere(whead, 0.2, 50, 50); // 눈 왼

	TR = glm::mat4(1.0f);
	TR = glm::translate(TR, glm::vec3(snowx + 0.008, snowy + 0.049 , snowz + 0.023));
	TR = glm::scale(TR, glm::vec3(0.015f, 0.015f, 0.015f));
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
	glUniform3f(vColorLocation, 0.0f, 0.0f, 0.0f);
	gluQuadricDrawStyle(wbody, GLU_FILL);
	gluSphere(whead, 0.2, 50, 50); // 눈 오

	TR = glm::mat4(1.0f);
	TR = glm::translate(TR, glm::vec3(snowx, snowy + 0.039, snowz + 0.023));
	TR = glm::scale(TR, glm::vec3(0.025f, 0.025f, 0.025f));
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
	glUniform3f(vColorLocation, 1.0f, 0.388235f, 0.278431f);
	gluQuadricDrawStyle(wbody, GLU_FILL);
	gluSphere(whead, 0.2, 50, 50); //  코

	if (bjump) {
		lEffect.x = snowx - 0.04;
		lEffect.y = snowy - 0.035;
		lEffect.z = snowz + 0.026;
				   
		rEffect.x = snowx + 0.04;
		rEffect.y = snowy - 0.035;
		rEffect.z = snowz;
		
		TR = glm::mat4(1.0f);
		TR = glm::translate(TR, glm::vec3(lEffect.x, lEffect.y, lEffect.z));
		TR = glm::scale(TR, glm::vec3(0.025f, 0.025f, 0.025f));
		glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
		glUniform3f(vColorLocation, 0.827451, 0.827451, 0.827451);
		gluQuadricDrawStyle(wbody, GLU_FILL);
		gluSphere(wbody, 0.2, 50, 50); // 점프할때 이펙트 왼쪽
		TR = glm::mat4(1.0f);
		TR = glm::translate(TR, glm::vec3(rEffect.x, rEffect.y, rEffect.z));
		TR = glm::scale(TR, glm::vec3(0.025f, 0.025f, 0.025f));
		glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
		glUniform3f(vColorLocation, 0.827451, 0.827451, 0.827451);
		gluQuadricDrawStyle(wbody, GLU_FILL);
		gluSphere(wbody, 0.2, 50, 50); // 점프할때 이펙트 오른쪽
	}

	//각도는 0도랑 90도 반복 - 지그재그이므로
	//코드 길 시작!!!! saveroad 코드부분 x와 z 0도일때랑 90도일때 코드가 다름!!
	//saveroad - x 
	// 현재가 saveroad[1]이라고 쳤을 때
	//1. saveroad[0] + 1번째 부분의 z축 scale반값	   + 0번째 부분의 x축 scale의 반값  - 90도
	//2. saveroad[0] + 0번째 부분의 z축 scale의 반값 - 1번째 부분의 x축 scale 반값 - 0도

	//saveroaed - z
	//1. saveroad[0] + 0번째 부분의 z축 scale 반값 + 1번째 부분의 x축 scale 반값 - 90도
	//2. saveroad[0] + 0번째 부분의 x축 scale 반값 + 1번째 부분의 z축 scale 반값 - 0도

	//카메라 위치 변환하고 싶을때는 
	//glm::vec3 cameraPos = glm::vec3(1.5f, 1.5f, 1.5f);   ----- 이부분의 z축 코드 크기 증감하면서 확인
	//glm::vec3 cameraDirection = glm::vec3(-1.5f, -1.5f, -1.5f);  --- 이부분은 그 앞에꺼의 반대의 부호로 z축 값 변경해주기(위에 값을 변경했을 때만)

	//light위치 바꾸고 싶으면 
	//snowx값과 snowz값 바꾸면서 빛 결과 확인해보기

	saveroad[0].x = 0.0f; //scale에서 x부분의 반값 기록해두기
	saveroad[0].z = 0.0f; //scale에서 z부분의 반값 기록해두기
	TR = glm::mat4(1.0f);
	TR = glm::scale(TR, glm::vec3(rxsize * 2, 0.01f, rzsize[0] * 2));
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
	glUniform3f(vColorLocation, 0.7f, 0.7f, 0.7f);
	for (int i = 0; i < 12; ++i) {
		Road[0][i].Bind();
		Road[0][i].Draw();
	} //길1	

	for (int j = 1; j < 80; ++j)
	{
		if (j % 2 == 1) {
			saveroad[j].x = saveroad[j - 1].x + rzsize[j] + rxsize;
			saveroad[j].z = saveroad[j - 1].z + rzsize[j - 1] - rxsize;
		}
		else {
			saveroad[j].x = saveroad[j - 1].x + rzsize[j - 1] - rxsize;
			saveroad[j].z = saveroad[j - 1].z + rzsize[j] + rxsize;
		}
		TR = glm::mat4(1.0f);
		TR = glm::translate(TR, glm::vec3(-saveroad[j].x, 0.01f, -saveroad[j].z));
		if (j % 2 == 1)
			TR = glm::rotate(TR, (float)glm::radians(90.0), glm::vec3(0.0f, 1.0f, 0.0f));
		TR = glm::scale(TR, glm::vec3(rxsize * 2, 0.01f, rzsize[j] * 2));
		glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
		glUniform3f(vColorLocation, 0.7f, 0.7f, 0.7f);
		for (int i = 0; i < 12; ++i) {
			Road[j][i].Bind();
			Road[j][i].Draw();
		} // 나머지 길
	}
	

	glutSwapBuffers();
	glutPostRedisplay();
}

GLvoid Reshape(int w, int h) //--- 콜백 함수: 다시 그리기 콜백 함수
{
	glViewport(0, 0, w, h);
}

void keyboard(unsigned char key, int x, int y) {
	switch (key) {
	case 'a':
		leftright = !leftright;
		break;
	case 32:
		bjump = true;
		cout << "jump" << endl;	
		break;
	case 'p': // 플레이
		music = true;
		Playmusic();
		break;
	case 'c':	// 위치 확인용 카메라 y축 == 0 일 때
		cameracheck = true;
		break;
	case 'x':	// 위치 확인용 카메라 (기존 카메라) 
		cameracheck = false;
		break;
	case 'q': // 종료
		glutLeaveMainLoop();
	}
	glutPostRedisplay();
}

void TimerFunction(int value) {
	for (int i = 0; i < snowsize; ++i) {
		snowposxz[i].y -= snowposxz[i].speed;
		if (snowposxz[i].y <= -0.5f)
			snowposxz[i].y = 1.5f;
	} // 눈내리기

	if (music) {
		if (!leftright) {
			snowz -= 0.01; // z 축이동
			for (int i = 0; i < snowsize; ++i)
				snowposxz[i].z -= 0.01; // 눈 위치 이동
		}
		else {
			snowx -= 0.01; // x축이동
			for (int i = 0; i < snowsize; ++i)
				snowposxz[i].x -= 0.01;
		}

	}
	if (bjump) {
		if (jumpcount < 10) {
			snowy += 0.01;
			lEffect.x += 0.04;
		}					
		else if (jumpcount < 20) {
			snowy -= 0.01;
			lEffect.x -= 0.04;
		}			
		else {
			bjump = false;
			jumpcount = -1;
		}
		++jumpcount;
	}

	glutPostRedisplay();
	glutTimerFunc(5, TimerFunction, 1);
}

void Playmusic() {

	cout << "----playing----" << endl;
	PlaySound(L"music1.wav", 0, SND_FILENAME | SND_ASYNC);
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
	//--- 1. 전체 버텍스 개수 및 삼각형 개수 세기
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


void ReadObj2(string file, vector<glm::vec3>& vertexInfo)
{
	vector<glm::vec3> vertex;
	vector<glm::vec3> vNormal;

	vector<glm::ivec3> vFace;
	vector<glm::ivec3> vnFace;

	ifstream in(file);
	if (!in) {
		cout << "OBJ File NO Have" << endl;
		return;
	}

	while (in) {
		string temp;
		getline(in, temp);

		if (temp[0] == 'v' && temp[1] == ' ') {
			istringstream slice(temp);

			glm::vec3 vertemp;
			char tmpword;
			slice >> tmpword >> vertemp.x >> vertemp.y >> vertemp.z;

			vertex.push_back(vertemp);
		}

		else if (temp[0] == 'v' && temp[1] == 'n' && temp[2] == ' ') {
			istringstream slice(temp);

			glm::vec3 vertemp;
			string tmpword;
			slice >> tmpword >> vertemp.x >> vertemp.y >> vertemp.z;

			vNormal.push_back(vertemp);
		}

		else if (temp[0] == 'f' && temp[1] == ' ') {
			istringstream slice(temp);

			glm::ivec3 vfacetemp;
			glm::ivec3 vnfacetemp;
			for (int i = -1; i < 3; ++i) {
				string word;
				getline(slice, word, ' ');
				if (i == -1) continue;						// f 읽을땐 건너뛴다
				if (word.find("/") != string::npos) {
					istringstream slash(word);
					string slashtmp;
					getline(slash, slashtmp, '/');

					vfacetemp[i] = atoi(slashtmp.c_str()) - 1;			//f 읽을땐 첫번째값만(v)	//배열인덱스 쓸거라 -1해줌

					getline(slash, slashtmp, '/');
					getline(slash, slashtmp, '/');
					vnfacetemp[i] = atoi(slashtmp.c_str()) - 1;
				}
				else {
					vfacetemp[i] = atoi(word.c_str()) - 1;			//f 읽을땐 첫번째값만(v)	//배열인덱스 쓸거라 -1해줌
				}
			}
			vFace.push_back(vfacetemp);
			vnFace.push_back(vnfacetemp);
		}
	}

	for (int i = 0; i < vFace.size(); ++i) {
		vertexInfo.push_back(vertex[vFace[i].x]);
		vertexInfo.push_back(vNormal[vnFace[i].x]);

		vertexInfo.push_back(vertex[vFace[i].y]);
		vertexInfo.push_back(vNormal[vnFace[i].y]);

		vertexInfo.push_back(vertex[vFace[i].z]);
		vertexInfo.push_back(vNormal[vnFace[i].z]);
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
//} //길테두리왼
//TR = glm::mat4(1.0f);
//TR = glm::translate(TR, glm::vec3(0.125f, 0.0125f, -0.125f));
//TR = glm::scale(TR, glm::vec3(0.025f, 0.025f, 1.25f));
//glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
//glUniform3f(vColorLocation, 0.7f, 0.7f, 0.7f);
//for (int i = 0; i < 12; ++i) {
//	Road[3][i].Bind();
//	Road[3][i].Draw();
//} //길테두리오
