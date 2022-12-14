#define _CRT_SECURE_NO_WARNINGS
#define STB_IMAGE_IMPLEMENTATION
#pragma comment(lib, "glew32.lib")
#pragma comment(lib, "freeglut.lib")
#include <iostream>
#include <Windows.h>
#include <mmsystem.h>
#include <time.h>
#include <random>
#include <fstream>
#include <sstream>
#include <string.h>
#include <cstdlib>
#include <vector>
#include <gl/glew.h>
#include <gl/freeglut.h>
#include <gl/freeglut_ext.h>
#include <stdlib.h>
#include <gl/glm/glm.hpp>
#include <gl/glm/ext.hpp>
#include <gl/glm/gtc/matrix_transform.hpp>
#include "stb_image.h"
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
void keyboard(unsigned char, int, int);
void Mouse(int button, int state, int x, int y);
void Motion(int x, int y);
void TimerFunction(int value);
GLvoid drawScene(GLvoid);
GLvoid Reshape(int w, int h);
void Playmusic();
GLubyte* LoadDIBitmap(const char* filename, BITMAPINFO** info);

GLuint shaderID;
GLint width, height;
int widthImage, heightImage, numberOfChannel;

GLuint vertexShader;
GLuint fragmentShader;

GLuint cubeVAO, cubeVBO;

GLuint VAO, VBO[4];

GLuint texture[7];//텍스쳐 이미지
int Imagenum = 0; //몇번째 이미지 사용할건지
bool playarrive = false;
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
	GLfloat tex[6];

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

		glBindBuffer(GL_ARRAY_BUFFER, VBO[3]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(tex), tex, GL_STATIC_DRAW);
		glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), 0);
		glEnableVertexAttribArray(3);

	}
	void Draw() {
		glBindVertexArray(VAO);
		glDrawArrays(GL_TRIANGLES, 0, 3);
	}
	void textureDraw() {
		glUseProgram(shaderID);
		glBindVertexArray(VAO);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture[Imagenum]);

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

struct Citem {
	bool btype;
	bool bget = false;
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
Plane* Box[40];	//길
Plane* Tree[2];		//tree
Plane* Hat;			// 모자
Plane* Obstacle[100];	// 장애물
Plane* Item[80];

float light = 1.0;	//빛 백탁

float rxsize = 0.083f;	//길의 폭/2한 값
float rzsize[100];
float rxsize2 = 0.083f;	//길의 폭/2한 값
float rzsize2[100];
float itemrotate = 0.0f;

XZ saveroad[100]; //길 기록용
XZ saveobst[100];  // 장애물 기록용
XZ saveitem[100]; // 아에템 기록용
XZ lEffect; // 왼쪽 이펙트 기록용
XZ rEffect; // 오른쪽 이펙트 기록용

XZ treepos[40];	//tree 위치
XZ boxpos[40];	//box 위치

Citem citem[20];
int jumpcount = 0;
int anum = 0;			// a 눌렀을 때 카운트


const int snowsize = 30;				// 눈 개수
float snowx = 0.0f, snowy = 0.035f, snowz = 0.0f;		// 현재 눈사람 위치 y축 이동X 나중에 점프 추가할때 추가
float diex = 0.0f, diey = 0.0f, diez = 0.0f;
float snowspeed = 0.015f;
float screeny = 0.0f;		// 시작화면y축
float faceroate = 0.0f;
//float lightx = -0.5f, lightz = 0.0f;		// 조명 위치
bool leftright = false;
bool bscreen = true;			// 시작화면 만들때
bool music = false;				// 노래 시작되면서 플레이 시작(아직 시작화면 미완성으로 이걸로 대체)
bool bjump = false;				// 점프
int cameracheck = 1;	//카메라 방향
bool die = false;//죽음 체크
bool diedir = false;
float dierotate = 0.0f;
int diecount = 0;//죽음 타이머에서 사용
bool roadon = false;
bool timer = false; //죽었을 때 더이상 충돌체크를 하지 않기 위함

void make_vertexShaders()
{
	GLchar* vertexShaderSource;

	vertexShaderSource = filetobuf("texturevertex.glsl");

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
	GLchar* fragmentShaderSource = filetobuf("texturefragment.glsl");

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

void InitTexture()
{
	string map[7] = { "start.png","boxtop.png", "boxside.png", "boxbottom.png", "road_texture.png", "road_texture2.png" ,"block.png"};
	glGenTextures(7, texture); //--- 텍스처 생성

	for (int i = 0; i < 7; ++i) {
		glBindTexture(GL_TEXTURE_2D, texture[i]); //--- 텍스처 바인딩
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT); //--- 현재 바인딩된 텍스처의 파라미터 설정하기
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		stbi_set_flip_vertically_on_load(true);
		unsigned char* data = stbi_load(map[i].c_str(), &widthImage, &heightImage, &numberOfChannel, 0);//--- 텍스처로 사용할 비트맵 이미지 로드하기
		glTexImage2D(GL_TEXTURE_2D, 0, 4, widthImage, heightImage, 0, GL_RGBA, GL_UNSIGNED_BYTE, data); //---텍스처 이미지 정의
		stbi_image_free(data);
	}
	glUseProgram(shaderID);
	int tLocation = glGetUniformLocation(shaderID, "outTex"); //--- outTexture 유니폼 샘플러의 위치를 가져옴
	glUniform1i(tLocation, 0); //--- 샘플러를 0번 유닛으로 설정
}

GLubyte* LoadDIBitmap(const char* filename, BITMAPINFO** info)
{
	FILE* fp;
	GLubyte* bits;
	int bitsize, infosize;
	BITMAPFILEHEADER header;

	//--- 바이너리 읽기 모드로 파일을 연다
	if ((fp = fopen(filename, "rb")) == NULL)
		return NULL;
	//--- 비트맵 파일 헤더를 읽는다.
	if (fread(&header, sizeof(BITMAPFILEHEADER), 1, fp) < 1) {
		fclose(fp); return NULL;
	}
	//--- 파일이 BMP 파일인지 확인한다.
	if (header.bfType != 'MB') {
		fclose(fp); return NULL;
	}
	//--- BITMAPINFOHEADER 위치로 간다.
	infosize = header.bfOffBits - sizeof(BITMAPFILEHEADER);
	//--- 비트맵 이미지 데이터를 넣을 메모리 할당을 한다.
	if ((*info = (BITMAPINFO*)malloc(infosize)) == NULL) {
		fclose(fp); return NULL;
	}
	//--- 비트맵 인포 헤더를 읽는다.
	if (fread(*info, 1, infosize, fp) < (unsigned int)infosize) {
		free(*info);
		fclose(fp); return NULL;
	}
	//--- 비트맵의 크기 설정
	if ((bitsize = (*info)->bmiHeader.biSizeImage) == 0)
		bitsize = ((*info)->bmiHeader.biWidth *
			(*info)->bmiHeader.biBitCount + 7) / 8.0 *
		abs((*info)->bmiHeader.biHeight);
	//--- 비트맵의 크기만큼 메모리를 할당한다.
	if ((bits = (unsigned char*)malloc(bitsize)) == NULL) {
		free(*info);
		fclose(fp); return NULL;
	}
	//--- 비트맵 데이터를 bit(GLubyte 타입)에 저장한다.
	if (fread(bits, 1, bitsize, fp) < (unsigned int)bitsize) {
		free(*info); free(bits);
		fclose(fp); return NULL;
	}
	fclose(fp);
	return bits;
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
	glGenBuffers(4, VBO);
}

void main(int argc, char** argv) //--- 윈도우 출력하고 콜백함수 설정 { //--- 윈도우 생성하기
{
	srand((unsigned int)time(NULL));
	glutInit(&argc, argv); // glut 초기화
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH); // 디스플레이 모드 설정
	glutInitWindowPosition(0, 0); // 윈도우의 위치 지정
	glutInitWindowSize(WINDOWX, WINDOWY); // 윈도우의 크기 지정
	glutCreateWindow("teamproject");// 윈도우 생성	(윈도우 이름)
	//--- GLEW 초기화하기
	glewExperimental = GL_TRUE;
	glewInit();

	InitTexture();
	InitShader();
	InitBuffer();

	glutKeyboardFunc(keyboard);
	glutTimerFunc(20, TimerFunction, 1);
	glutDisplayFunc(drawScene); //--- 출력 콜백 함수
	glutReshapeFunc(Reshape);
	glutMainLoop();
}

GLvoid drawScene() //--- 콜백 함수: 그리기 콜백 함수 
{
	if (start) {
		start = FALSE;

		FL = fopen("cube.obj", "rt");
		ReadObj(FL);
		fclose(FL);
		Screen = (Plane*)malloc(sizeof(Plane) * faceNum);
		vectoplane(Screen);
		planecolorset(Screen, 0);

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
		for (int i = 0; i < 40; ++i) {
			FL = fopen("cube.obj", "rt");
			ReadObj(FL);
			fclose(FL);
			Box[i] = (Plane*)malloc(sizeof(Plane) * faceNum);
			vectoplane(Box[i]);
			planecolorset(Box[i], 0);
		}
		for (int i = 0; i < 80; ++i) {
			FL = fopen("cube.obj", "rt");
			ReadObj(FL);
			fclose(FL);
			Obstacle[i] = (Plane*)malloc(sizeof(Plane) * faceNum);
			vectoplane(Obstacle[i]);
			planecolorset(Obstacle[i], 0);
		}

		for (int i = 0; i < 80; ++i) {
			FL = fopen("cube.obj", "rt");
			ReadObj(FL);
			fclose(FL);
			Item[i] = (Plane*)malloc(sizeof(Plane) * faceNum);
			vectoplane(Item[i]);
			planecolorset(Item[i], 0);
		}

		for (int i = 0; i < 100; ++i) {
			snow[i] = gluNewQuadric();
			snowposxz[i].x = snowx + snowpos(dre);
			snowposxz[i].z = snowz + snowpos(dre);
			snowposxz[i].speed = speed(dre);
			rzsize[i] = roadlength(dre);
		}
		for (int i = 0; i < 4; ++i) {
			if (i % 2 == 0)
				citem[i].btype = false;
			else
				citem[i].btype = true;
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


	// 카메라
	if (cameracheck == 1) {
		glm::mat4 Vw = glm::mat4(1.0f);
		glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 2.0f);
		glm::vec3 cameraDirection = glm::vec3(0.0f, 0.0f, -2.0f);
		glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
		Vw = glm::lookAt(cameraPos, cameraDirection, cameraUp);
		glUniformMatrix4fv(viewLocation, 1, GL_FALSE, &Vw[0][0]);
	}
	else if (cameracheck == 2) {
		glm::mat4 Vw = glm::mat4(1.0f);
		glm::vec3 cameraPos = glm::vec3(snowx + 1.5f, 1.5f, snowz + 1.0f);
		glm::vec3 cameraDirection = glm::vec3(snowx - 1.5f, -1.5f, snowz - 1.0f);
		glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
		Vw = glm::lookAt(cameraPos, cameraDirection, cameraUp);
		glUniformMatrix4fv(viewLocation, 1, GL_FALSE, &Vw[0][0]);
	}
	else if (cameracheck == 3) {																										// 1인칭 모드
		glm::mat4 Vw = glm::mat4(1.0f);
		glm::mat4 Cp = glm::mat4(1.0f);

		Cp = glm::rotate(Cp, (float)glm::radians(-faceroate), glm::vec3(0.0f, 1.0f, 0.0f));

		glm::vec3 cameraPos = glm::vec4(snowx, snowy + 0.05f, snowz, 0.0f);
		glm::vec3 cameraDirection = glm::vec4(0.0, 0.0f, -0.05f, 0.0f) * Cp;
		glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

		Vw = glm::lookAt(cameraPos, cameraPos + cameraDirection, cameraUp);
		glUniformMatrix4fv(viewLocation, 1, GL_FALSE, &Vw[0][0]);
	}

	glm::mat4 Pj = glm::mat4(1.0f);
	Pj = glm::perspective(glm::radians(45.0f), (float)WINDOWX / (float)WINDOWY, 0.1f, 100.0f);
	glUniformMatrix4fv(projLocation, 1, GL_FALSE, &Pj[0][0]);

	TR = glm::mat4(1.0f);
	modelLocation = glGetUniformLocation(shaderID, "model");
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));

	int lightPosLocation = glGetUniformLocation(shaderID, "lightPos"); //--- lightPos 값 전달: (0.0, 0.0, 5.0);
	if (bscreen)
		glUniform3f(lightPosLocation, snowx, snowy, snowz + 2.0f);
	else
		glUniform3f(lightPosLocation, snowx, 0.5f, snowz - 0.5f);
	int lightColorLocation = glGetUniformLocation(shaderID, "lightColor"); //--- lightColor 값 전달: (1.0, 1.0, 1.0) 백색
	glUniform3f(lightColorLocation, light, light, light);

	GLuint select = glGetUniformLocation(shaderID, "select");
	glUniform1i(select, 1); //텍스쳐 사용여부 0은 사용 x 다른 숫자는 사용 o
	glUniform3f(lightColorLocation, light, light, light);
	if (bscreen) {
		TR = glm::mat4(1.0f);
		TR = glm::translate(TR, glm::vec3(0.0f, screeny, 0.5f));
		TR = glm::scale(TR, glm::vec3(1.65f, 1.35f, 0.01f));
		glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
		glUniform3f(vColorLocation, 0.8f, 0.8f, 0.8f);
		Imagenum = 0;
		for (int i = 0; i < 12; ++i) {
			Screen[i].Bind();
			Screen[i].textureDraw();
		} //시작화면		
	}
	glUniform1i(select, 1); //텍스쳐 사용여부 0은 사용 x 다른 숫자는 사용 o
	Imagenum = 5;
	saveroad[0].x = 0.0f; //scale에서 x부분의 반값 기록해두기
	saveroad[0].z = 0.0f; //scale에서 z부분의 반값 기록해두기
	TR = glm::mat4(1.0f);
	TR = glm::scale(TR, glm::vec3(rxsize * 2, 0.01f, rzsize[0] * 2));
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
	glUniform3f(vColorLocation, 0.7f, 0.7f, 0.7f);
	for (int i = 0; i < 12; ++i) {
		Road[0][i].Bind();
		Road[0][i].textureDraw();
	} //길1	
	treepos[0].x = saveroad[0].x - 0.21f;
	treepos[0].z = saveroad[0].z;
	boxpos[0].x = saveroad[0].x - 0.35f;
	boxpos[0].z = saveroad[0].z;
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
		TR = glm::translate(TR, glm::vec3(-saveroad[j].x, 0.0f, -saveroad[j].z));
		if (j % 2 == 1)
			TR = glm::rotate(TR, (float)glm::radians(90.0), glm::vec3(0.0f, 1.0f, 0.0f));
		TR = glm::scale(TR, glm::vec3(rxsize * 2, 0.01f, rzsize[j] * 2));
		glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
		glUniform3f(vColorLocation, 0.7f, 0.7f, 0.7f);
		for (int i = 0; i < 12; ++i) {
			Road[j][i].Bind();
			Road[j][i].textureDraw();
		} // 나머지 길
		if (j % 4 == 0) {
			treepos[j / 2].x = saveroad[j].x - 0.21f;
			treepos[j / 2].z = saveroad[j].z;
			boxpos[j / 2].x = saveroad[j].x - 0.35f;
			boxpos[j / 2].z = saveroad[j].z;
		}
		else if (j % 2 == 0) {
			treepos[j / 2].x = saveroad[j].x + 0.28f;
			treepos[j / 2].z = saveroad[j].z - 0.05;
			boxpos[j / 2].x = saveroad[j].x + 0.35f;
			boxpos[j / 2].z = saveroad[j].z - 0.05;
		}
	}
	glUniform1i(select, 0); //텍스쳐 사용여부 0은 사용 x 다른 숫자는 사용 o

	for (int i = 0; i < 40; ++i)
	{
		TR = glm::mat4(1.0f);
		TR = glm::translate(TR, glm::vec3(-treepos[i].x, 0.08f, -treepos[i].z));
		TR = glm::scale(TR, glm::vec3(0.07f, 0.17f, 0.07f));
		glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
		glUniform3f(vColorLocation, 0.73f, 0.56f, 0.56f);
		for (int i = 0; i < 12; ++i) {
			Tree[0][i].Bind();
			Tree[0][i].Draw();
		}//나무 밑둥
		TR = glm::mat4(1.0f);
		TR = glm::translate(TR, glm::vec3(-treepos[i].x, 0.17f, -treepos[i].z));
		TR = glm::scale(TR, glm::vec3(0.15f, 0.125f, 0.15f));
		glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
		glUniform3f(vColorLocation, 0.4f, 0.8f, 0.66f);
		for (int i = 0; i < 6; ++i) {
			Tree[1][i].Bind();
			Tree[1][i].Draw();
		}//나무 머리 밑
		TR = glm::mat4(1.0f);
		TR = glm::translate(TR, glm::vec3(-treepos[i].x, 0.25f, -treepos[i].z));
		TR = glm::scale(TR, glm::vec3(0.15f, 0.125f, 0.15f));
		glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
		glUniform3f(vColorLocation, 0.4f, 0.8f, 0.66f);
		for (int i = 0; i < 6; ++i) {
			Tree[1][i].Bind();
			Tree[1][i].Draw();
		}
	}//나무 머리
	glUniform1i(select, 1); //텍스쳐 사용여부 0은 사용 x 다른 숫자는 사용 o
	for (int i = 0; i < 40; ++i) {
		TR = glm::mat4(1.0f);
		TR = glm::translate(TR, glm::vec3(-boxpos[i].x, 0.5 * 0.1, -boxpos[i].z));
		TR = glm::scale(TR, glm::vec3(0.1f, 0.1f, 0.1f));
		glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
		glUniform3f(vColorLocation, 0.4f, 0.8f, 0.66f);
		for (int j = 0; j < 12; ++j) {
			if (j == 4 || j == 5)
				Imagenum = 1;
			else if (j == 8 || j == 9)
				Imagenum = 3;
			else
				Imagenum = 2;
			Box[i][j].Bind();
			Box[i][j].textureDraw();
		}
	}
	glUniform1i(select, 0); //텍스쳐 사용여부 0은 사용 x 다른 숫자는 사용 o
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
	//tree확인용
	TR = glm::mat4(1.0f);
	TR = glm::translate(TR, glm::vec3(snowx, snowy, snowz));
	TR = glm::scale(TR, glm::vec3(0.05f, 0.05f, 0.05f));
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
	glUniform3f(vColorLocation, 1.0f, 0.980392, 0.941176);
	gluQuadricDrawStyle(wbody, GLU_FILL);
	gluSphere(wbody, 0.7, 50, 50); //눈사람 몸통

	TR = glm::mat4(1.0f);
	TR = glm::translate(TR, glm::vec3(diex, diey, diez));
	TR = glm::rotate(TR, (float)glm::radians(dierotate), glm::vec3(0.0f, 0.0f, 1.0f));
	TR = glm::translate(TR, glm::vec3(snowx, snowy + 0.045, snowz));
	TR = glm::scale(TR, glm::vec3(0.05f, 0.05f, 0.05f));
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
	glUniform3f(vColorLocation, 1.0f, 0.937255f, 0.835294f);
	gluQuadricDrawStyle(whead, GLU_FILL);
	gluSphere(whead, 0.5, 50, 50); //눈사람 머리

	TR = glm::mat4(1.0f);
	TR = glm::translate(TR, glm::vec3(diex, diey, diez));
	TR = glm::rotate(TR, (float)glm::radians(dierotate), glm::vec3(0.0f, 0.0f, 1.0f));
	TR = glm::translate(TR, glm::vec3(snowx, snowy + 0.083, snowz));
	TR = glm::scale(TR, glm::vec3(0.03f, 0.03f, 0.03f));
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
	glUniform3f(vColorLocation, 1.0, 0.0f, 0.0f);
	for (int i = 0; i < 6; ++i) {
		Hat[i].Bind();
		Hat[i].Draw();
	}//모자 

	TR = glm::mat4(1.0f);
	TR = glm::translate(TR, glm::vec3(diex, diey, diez));
	TR = glm::rotate(TR, (float)glm::radians(dierotate), glm::vec3(0.0f, 0.0f, 1.0f));
	TR = glm::translate(TR, glm::vec3(snowx, snowy + 0.093, snowz));
	TR = glm::scale(TR, glm::vec3(0.02f, 0.02f, 0.02f));
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
	glUniform3f(vColorLocation, 1.0f, 1.0f, 1.0f);
	gluQuadricDrawStyle(whead, GLU_FILL);
	gluSphere(whead, 0.5, 50, 50); // 모자 꽁지

	TR = glm::mat4(1.0f);
	TR = glm::translate(TR, glm::vec3(diex, diey, diez));
	TR = glm::rotate(TR, (float)glm::radians(dierotate), glm::vec3(0.0f, 0.0f, 1.0f));
	TR = glm::translate(TR, glm::vec3(snowx, snowy, snowz));
	TR = glm::rotate(TR, (float)glm::radians(faceroate), glm::vec3(0.0f, 1.0f, 0.0f));
	TR = glm::translate(TR, glm::vec3(-0.008f, 0.049f, 0.023f));
	TR = glm::scale(TR, glm::vec3(0.015f, 0.015f, 0.015f));
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
	glUniform3f(vColorLocation, 0.0f, 0.0f, 0.0f);
	gluQuadricDrawStyle(whead, GLU_FILL);
	gluSphere(whead, 0.2, 50, 50); // 눈 왼

	TR = glm::mat4(1.0f);
	TR = glm::translate(TR, glm::vec3(diex, diey, diez));
	TR = glm::rotate(TR, (float)glm::radians(dierotate), glm::vec3(0.0f, 0.0f, 1.0f));
	TR = glm::translate(TR, glm::vec3(snowx, snowy, snowz));
	TR = glm::rotate(TR, (float)glm::radians(faceroate), glm::vec3(0.0f, 1.0f, 0.0f));
	TR = glm::translate(TR, glm::vec3(0.008f, 0.049f, 0.023f));
	TR = glm::scale(TR, glm::vec3(0.015f, 0.015f, 0.015f));
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
	glUniform3f(vColorLocation, 0.0f, 0.0f, 0.0f);
	gluQuadricDrawStyle(whead, GLU_FILL);
	gluSphere(whead, 0.2, 50, 50); // 눈 오

	TR = glm::mat4(1.0f);
	TR = glm::translate(TR, glm::vec3(diex, diey, diez));
	TR = glm::rotate(TR, (float)glm::radians(dierotate), glm::vec3(0.0f, 0.0f, 1.0f));
	TR = glm::translate(TR, glm::vec3(snowx, snowy, snowz));
	TR = glm::rotate(TR, (float)glm::radians(faceroate), glm::vec3(0.0f, 1.0f, 0.0f));
	TR = glm::translate(TR, glm::vec3(0.0f, 0.039, 0.023));
	TR = glm::scale(TR, glm::vec3(0.025f, 0.025f, 0.025f));
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
	glUniform3f(vColorLocation, 1.0f, 0.388235f, 0.278431f);
	gluQuadricDrawStyle(whead, GLU_FILL);
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

	saveobst[0].x = 0.0f; //scale에서 x부분의 반값 기록해두기
	saveobst[0].z = 0.0f; //scale에서 z부분의 반값 기록해두기
	for (int j = 1; j < 80; ++j)
	{
		if (j % 2 == 1) {
			saveobst[j].x = saveobst[j - 1].x + rzsize[j] + rxsize;
			saveobst[j].z = saveobst[j - 1].z + rzsize[j - 1] - rxsize;
		}
		else {
			saveobst[j].x = saveobst[j - 1].x + rzsize[j - 1] - rxsize;
			saveobst[j].z = saveobst[j - 1].z + rzsize[j] + rxsize;
		}
		TR = glm::mat4(1.0f);
		TR = glm::translate(TR, glm::vec3(-saveobst[j].x, 0.03f, -saveobst[j].z));
		if (j % 2 == 1)
			TR = glm::rotate(TR, (float)glm::radians(90.0), glm::vec3(0.0f, 1.0f, 0.0f));
		TR = glm::scale(TR, glm::vec3(0.13, 0.03f, 0.02));
		glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
		glUniform3f(vColorLocation, 0.74, 0.59, 0.5);
		if (j % 20 != 0)
		{
			if (j % 4 == 0) {
				for (int i = 0; i < 12; ++i) {
					Obstacle[j][i].Bind();
					Obstacle[j][i].Draw();
				}
			}
		}
	} // 장애물

	glUniform1i(select, 1); //텍스쳐 사용여부 0은 사용 x 다른 숫자는 사용 o
	saveitem[0].x = 0.0f; //scale에서 x부분의 반값 기록해두기
	saveitem[0].z = 0.0f; //scale에서 z부분의 반값 기록해두기
	for (int j = 1; j < 80; ++j)
	{
		if (j % 2 == 1) {
			saveitem[j].x = saveitem[j - 1].x + rzsize[j] + rxsize;
			saveitem[j].z = saveitem[j - 1].z + rzsize[j - 1] - rxsize;
		}
		else {
			saveitem[j].x = saveitem[j - 1].x + rzsize[j - 1] - rxsize;
			saveitem[j].z = saveitem[j - 1].z + rzsize[j] + rxsize;
		}
		if (j % 20 == 0) {
			TR = glm::mat4(1.0f);
			TR = glm::translate(TR, glm::vec3(-saveitem[j].x, 0.05f, -saveitem[j].z));
			TR = glm::rotate(TR, (float)glm::radians(itemrotate), glm::vec3(0.0f, 1.0f, 0.0f));
			TR = glm::scale(TR, glm::vec3(0.06, 0.06f, 0.06));
			glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
			if (!citem[j / 20].bget) {
				Imagenum = 6;
				for (int i = 0; i < 12; ++i) {
					Item[j][i].Bind();
					Item[j][i].textureDraw();
				}
			}
		}
	} // 아이템

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
		++diedir;
		++anum;
		if (leftright)
			faceroate = 90.0f;
		else
			faceroate = 0.0f;
		break;
	case 'r':
		bscreen = true;
		cameracheck = 1;
		diecount = 0;
		snowx = 0.0f, snowy = 0.035f, snowz = 0.0f;
		diex = 0.0f, diey = 0.0f, diez = 0.0f;
		screeny = 0.0f;
		faceroate = 0.0f;
		music = false;
		timer = false;
		dierotate = 0.0f;
		die = false;
		jumpcount = 0;
		anum = 0;
		leftright = false;
		for (int i = 0; i < 100; ++i) {
			snowposxz[i].x = snowx + snowpos(dre);
			snowposxz[i].z = snowz + snowpos(dre);
			snowposxz[i].speed = speed(dre);
		}
		snowspeed = 0.015f;
		break;
	case 32:
		bjump = true;
		break;
	case 'p': // 플레이
		bscreen = false;
		music = true;
		timer = true;
		Playmusic();
		cameracheck = 2;
		break;
	case 'c':	// 위치 확인용 카메라 y축 == 0 일 때
		cameracheck = 1;
		break;
	case 'x':	// 위치 확인용 카메라 (기존 카메라) 
		cameracheck = 2;
		break;
	case 'b':
		cameracheck = 3;
		snowspeed = 0.01;
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

	++itemrotate;
	if (timer) {
		if (music) {
			if (anum < 79) {
				if (!leftright) {
					snowz -= snowspeed; // z 축이동
					for (int i = 0; i < snowsize; ++i)
						snowposxz[i].z -= snowspeed; // 눈 위치 이동
				}
				else {
					snowx -= snowspeed; // x축이동
					for (int i = 0; i < snowsize; ++i)
						snowposxz[i].x -= snowspeed;
				}
			}
			else if (anum >= 79) {
				if (!playarrive) {
					PlaySound(L"arrive.wav", 0, SND_FILENAME | SND_ASYNC);
					playarrive = !playarrive;
				}
				if (jumpcount < 10) {
					snowy += 0.01;
				}
				else if (jumpcount < 20) {
					snowy -= 0.01;
				}
				else {
					bjump = false;
					jumpcount = -1;
				}
				++jumpcount;
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

		roadon = false;
		die = false;
		for (int i = 0; i < 80; ++i) {
			if (i % 2 == 0) {
				if (snowx >= -saveroad[i].x - rxsize && snowx <= -saveroad[i].x + rxsize)
					if (snowz >= -saveroad[i].z - rzsize[i] && snowz <= -saveroad[i].z + rzsize[i])
						roadon = true;
			}
			else {
				if (snowx >= -saveroad[i].x - rzsize[i] && snowx <= -saveroad[i].x + rzsize[i])
					if (snowz >= -saveroad[i].z - rxsize && snowz <= -saveroad[i].z + rxsize)
						roadon = true;
			}
		}

		for (int i = 1; i < 80; ++i) {
			if (i % 20 == 0)
			{
				if (snowx >= -saveitem[i].x - 0.03 && snowx <= -saveitem[i].x + 0.03)
					if (snowz >= -saveitem[i].z - 0.03 && snowz <= -saveitem[i].z + 0.03)
						if (!bjump) {
							if (!citem[i / 20].bget) {
								citem[i / 20].bget = true;
								if (citem[i / 20].btype)
									snowspeed += 0.0025f;//속도 증가
								else
									snowspeed -= 0.0025f;//속도 감소
							}
						}
			}
		}
		for (int i = 1; i < 80; ++i) {
			if (i % 20 != 0)
			{
				if (i % 4 == 0) {
					if (snowx >= -saveobst[i].x - 0.065 && snowx <= -saveobst[i].x + 0.065)
						if (snowz >= -saveobst[i].z - 0.01 && snowz <= -saveobst[i].z + 0.01)
							if (!bjump)
								die = true;
				}
			}
		}
		if (!roadon)
			die = true;

		if (die) {
			music = false;
			cout << anum << " die ";
		}
		if (die) {
			if (diecount < 20) {
				if (diedir % 2 == 0) {
					diex += 0.00125;
					diey -= 0.00125;
					dierotate += 2.0f;
				}
				else {
					diex += 0.0025;
					diey -= 0.0025;
				}
			}
			else {
				timer = false;
			}
			++diecount;
		}
	}
	glutPostRedisplay();
	glutTimerFunc(20, TimerFunction, 1);
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
	if (a == 0) {
		for (int i = 0; i < faceNum; i += 2) {
			float R = (rand() % 11) * 0.1;
			float G = (rand() % 11) * 0.1;
			float B = (rand() % 11) * 0.1;
			for (int j = 0; j < 3; ++j) {
				p[i].color[j * 3] = 1.0;
				p[i].color[j * 3 + 1] = 1.0;
				p[i].color[j * 3 + 2] = 1.0;

				p[i + 1].color[j * 3] = 1.0;
				p[i + 1].color[j * 3 + 1] = 1.0;
				p[i + 1].color[j * 3 + 2] = 1.0;
			}


		}

		for (int i = 0; i < faceNum; ++i) {
			if (i == 0) {
				p[i].tex[0] = 1.0;
				p[i].tex[1] = 0.0;
				p[i].tex[2] = 0.0;
				p[i].tex[3] = 1.0;
				p[i].tex[4] = 0.0;
				p[i].tex[5] = 0.0;
			}
			else if (i == 1) {
				p[i].tex[0] = 1.0;
				p[i].tex[1] = 0.0;
				p[i].tex[2] = 1.0;
				p[i].tex[3] = 1.0;
				p[i].tex[4] = 0.0;
				p[i].tex[5] = 1.0;
			}
			else if (i == 2) {
				p[i].tex[0] = 0.0;
				p[i].tex[1] = 0.0;
				p[i].tex[2] = 1.0;
				p[i].tex[3] = 1.0;
				p[i].tex[4] = 1.0;
				p[i].tex[5] = 0.0;
			}
			else if (i == 3) {
				p[i].tex[0] = 0.0;
				p[i].tex[1] = 0.0;
				p[i].tex[2] = 1.0;
				p[i].tex[3] = 0.0;
				p[i].tex[4] = 1.0;
				p[i].tex[5] = 1.0;
			}
			else if (i == 4) {
				p[i].tex[0] = 0.0;
				p[i].tex[1] = 1.0;
				p[i].tex[2] = 1.0;
				p[i].tex[3] = 0.0;
				p[i].tex[4] = 1.0;
				p[i].tex[5] = 1.0;
			}
			else if (i == 5) {
				p[i].tex[0] = 0.0;
				p[i].tex[1] = 1.0;
				p[i].tex[2] = 0.0;
				p[i].tex[3] = 0.0;
				p[i].tex[4] = 1.0;
				p[i].tex[5] = 0.0;
			}
			else if (i == 6) {
				p[i].tex[0] = 1.0;
				p[i].tex[1] = 0.0;
				p[i].tex[2] = 1.0;
				p[i].tex[3] = 1.0;
				p[i].tex[4] = 0.0;
				p[i].tex[5] = 1.0;
			}
			else if (i == 7) {
				p[i].tex[0] = 1.0;
				p[i].tex[1] = 0.0;
				p[i].tex[2] = 0.0;
				p[i].tex[3] = 1.0;
				p[i].tex[4] = 0.0;
				p[i].tex[5] = 0.0;
			}
			else if (i == 8) {
				p[i].tex[0] = 1.0;
				p[i].tex[1] = 1.0;
				p[i].tex[2] = 0.0;
				p[i].tex[3] = 1.0;
				p[i].tex[4] = 0.0;
				p[i].tex[5] = 0.0;
			}
			else if (i == 9) {
				p[i].tex[0] = 1.0;
				p[i].tex[1] = 1.0;
				p[i].tex[2] = 0.0;
				p[i].tex[3] = 0.0;
				p[i].tex[4] = 1.0;
				p[i].tex[5] = 0.0;
			}
			else if (i == 10) {
				p[i].tex[0] = 0.0;
				p[i].tex[1] = 0.0;
				p[i].tex[2] = 1.0;
				p[i].tex[3] = 0.0;
				p[i].tex[4] = 1.0;
				p[i].tex[5] = 1.0;
			}
			else {
				p[i].tex[0] = 0.0;
				p[i].tex[1] = 0.0;
				p[i].tex[2] = 1.0;
				p[i].tex[3] = 1.0;
				p[i].tex[4] = 0.0;
				p[i].tex[5] = 1.0;
			}
		}

	}

	else if (a == 1) {
		for (int i = 0; i < 2; ++i) {
			for (int j = 0; j < 3; ++j) {
				p[i].color[j * 3] = 1.0;
				p[i].color[j * 3 + 1] = 1.0;
				p[i].color[j * 3 + 2] = 1.0;
			}
		}
		for (int i = 2; i < faceNum; ++i) {
			for (int j = 0; j < 3; ++j) {
				p[i].color[j * 3] = 1.0;
				p[i].color[j * 3 + 1] = 1.0;
				p[i].color[j * 3 + 2] = 1.0;
			}
		}

		for (int i = 0; i < faceNum; ++i) {
			if (i < 4) {
				p[i].tex[0] = 0.0;
				p[i].tex[1] = 0.0;
				p[i].tex[2] = 1.0;
				p[i].tex[3] = 0.0;
				p[i].tex[4] = 0.5;
				p[i].tex[5] = 1.0;
			}
			else if (i == 4) {
				p[i].tex[0] = 0.0;
				p[i].tex[1] = 1.0;
				p[i].tex[2] = 0.0;
				p[i].tex[3] = 0.0;
				p[i].tex[4] = 1.0;
				p[i].tex[5] = 1.0;
			}
			else {
				p[i].tex[0] = 1.0;
				p[i].tex[1] = 0.0;
				p[i].tex[2] = 1.0;
				p[i].tex[3] = 1.0;
				p[i].tex[4] = 0.0;
				p[i].tex[5] = 0.0;
			}
		}
	}
}