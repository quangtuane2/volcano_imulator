#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>

const unsigned int SCR_WIDTH = 1200;
const unsigned int SCR_HEIGHT = 800;

// Buffers
GLuint VAO, VBO[2];
std::vector<float> vertices;
std::vector<float> normals;
GLuint shaderProgram;

// Camera
float rotationX = 0.0f, rotationY = 0.0f;
float zoom = 1.0f;
bool mousePressed = false;
double lastX, lastY;

//  Ma trận 4x4 cơ bản
struct Matrix4x4 {
    float m[16];
    Matrix4x4() {
        for(int i=0;i<16;i++) m[i]=0;
        m[0]=m[5]=m[10]=m[15]=1.0f;
    }
};
Matrix4x4 multiply(const Matrix4x4 &a,const Matrix4x4 &b){
    Matrix4x4 r;
    for(int i=0;i<4;i++){
        for(int j=0;j<4;j++){
            r.m[i*4+j] = a.m[i*4+0]*b.m[0*4+j] + a.m[i*4+1]*b.m[1*4+j]
                        + a.m[i*4+2]*b.m[2*4+j] + a.m[i*4+3]*b.m[3*4+j];
        }
    }
    return r;
}
Matrix4x4 rotateXY(float ax,float ay){
    Matrix4x4 rx, ry;
    float cx=cosf(ax), sx=sinf(ax);
    rx.m[5]=cx; rx.m[6]=-sx; rx.m[9]=sx; rx.m[10]=cx;
    float cy=cosf(ay), sy=sinf(ay);
    ry.m[0]=cy; ry.m[2]=sy; ry.m[8]=-sy; ry.m[10]=cy;
    return multiply(ry, rx);
}
Matrix4x4 scale(float sx,float sy,float sz){
    Matrix4x4 s;
    s.m[0]=sx; s.m[5]=sy; s.m[10]=sz;
    return s;
}
Matrix4x4 translate(float tx,float ty,float tz){
    Matrix4x4 t;
    t.m[12]=tx; t.m[13]=ty; t.m[14]=tz;
    return t;
}

// Noise đơn giản
float simpleNoise(float x,float z,float freq){
    return (sinf(x*freq)*cosf(z*freq))*0.2f;
}

// Tính normal cho 3 đỉnh
void computeNormal(const float* v0,const float* v1,const float* v2,float* outNormal){
    float U[3]={v1[0]-v0[0],v1[1]-v0[1],v1[2]-v0[2]};
    float V[3]={v2[0]-v0[0],v2[1]-v0[1],v2[2]-v0[2]};
    outNormal[0]=U[1]*V[2]-U[2]*V[1];
    outNormal[1]=U[2]*V[0]-U[0]*V[2];
    outNormal[2]=U[0]*V[1]-U[1]*V[0];
    float len = sqrtf(outNormal[0]*outNormal[0]+outNormal[1]*outNormal[1]+outNormal[2]*outNormal[2]);
    if(len>0){ outNormal[0]/=len; outNormal[1]/=len; outNormal[2]/=len;}
}

// Thêm tam giác và normal
void addTriangle(float* v0,float* v1,float* v2){
    vertices.insert(vertices.end(),{v0[0],v0[1],v0[2], v1[0],v1[1],v1[2], v2[0],v2[1],v2[2]});
    float n[3]; computeNormal(v0,v1,v2,n);
    for(int i=0;i<3;i++) normals.insert(normals.end(),{n[0],n[1],n[2]});
}

// Tạo núi lửa chi tiết
void createDetailedVolcano(){
    srand(time(0));
    const int BASE_SEGMENTS=64;
    const int HEIGHT_SEGMENTS=8;
    const float BASE_RADIUS=2.0f;
    const float CRATER_RADIUS=0.3f;
    const float VOLCANO_HEIGHT=2.5f;
    const float CRATER_DEPTH=0.4f;

    // Thân núi
    for(int layer=0;layer<HEIGHT_SEGMENTS;layer++){
        float h0=(VOLCANO_HEIGHT/HEIGHT_SEGMENTS)*layer;
        float h1=(VOLCANO_HEIGHT/HEIGHT_SEGMENTS)*(layer+1);
        float r0=BASE_RADIUS-(BASE_RADIUS-CRATER_RADIUS)*(h0/VOLCANO_HEIGHT);
        float r1=BASE_RADIUS-(BASE_RADIUS-CRATER_RADIUS)*(h1/VOLCANO_HEIGHT);
        for(int i=0;i<BASE_SEGMENTS;i++){
            float a0=2.0f*M_PI*i/BASE_SEGMENTS;
            float a1=2.0f*M_PI*(i+1)/BASE_SEGMENTS;
            float n0=1.0f+simpleNoise(cosf(a0),sinf(a0),3.0f+layer);
            float n1=1.0f+simpleNoise(cosf(a1),sinf(a1),3.0f+layer);
            float n2=1.0f+simpleNoise(cosf(a0),sinf(a0),3.0f+layer+1);
            float n3=1.0f+simpleNoise(cosf(a1),sinf(a1),3.0f+layer+1);

            float v0[3]={r0*n0*cosf(a0),h0,r0*n0*sinf(a0)};
            float v1[3]={r1*n3*cosf(a1),h1,r1*n3*sinf(a1)};
            float v2[3]={r0*n1*cosf(a1),h0,r0*n1*sinf(a1)};
            addTriangle(v0,v1,v2);

            float v3[3]={r0*n0*cosf(a0),h0,r0*n0*sinf(a0)};
            float v4[3]={r1*n2*cosf(a0),h1,r1*n2*sinf(a0)};
            float v5[3]={r1*n3*cosf(a1),h1,r1*n3*sinf(a1)};
            addTriangle(v3,v4,v5);
        }
    }

    // Đáy núi
    for(int i=0;i<BASE_SEGMENTS;i++){
        float a0=2.0f*M_PI*i/BASE_SEGMENTS;
        float a1=2.0f*M_PI*(i+1)/BASE_SEGMENTS;
        float v0[3]={0,0,0};
        float v1[3]={BASE_RADIUS*cosf(a0),0,BASE_RADIUS*sinf(a0)};
        float v2[3]={BASE_RADIUS*cosf(a1),0,BASE_RADIUS*sinf(a1)};
        addTriangle(v0,v1,v2);
    }

    // Miệng núi
    const int CRATER_SEGMENTS=32;
    float cTop=VOLCANO_HEIGHT;
    float cBot=VOLCANO_HEIGHT-CRATER_DEPTH;
    for(int i=0;i<CRATER_SEGMENTS;i++){
        float a0=2.0f*M_PI*i/CRATER_SEGMENTS;
        float a1=2.0f*M_PI*(i+1)/CRATER_SEGMENTS;
        float v0[3]={CRATER_RADIUS*cosf(a0),cTop,CRATER_RADIUS*sinf(a0)};
        float v1[3]={CRATER_RADIUS*cosf(a1),cTop,CRATER_RADIUS*sinf(a1)};
        float v2[3]={CRATER_RADIUS*0.8f*cosf(a0),cBot,CRATER_RADIUS*0.8f*sinf(a0)};
        addTriangle(v0,v1,v2);

        float v3[3]={CRATER_RADIUS*cosf(a1),cTop,CRATER_RADIUS*sinf(a1)};
        float v4[3]={CRATER_RADIUS*0.8f*cosf(a1),cBot,CRATER_RADIUS*0.8f*sinf(a1)};
        float v5[3]={CRATER_RADIUS*0.8f*cosf(a0),cBot,CRATER_RADIUS*0.8f*sinf(a0)};
        addTriangle(v3,v4,v5);
    }

    // Đáy miệng (dung nham)
    for(int i=0;i<CRATER_SEGMENTS;i++){
        float a0=2.0f*M_PI*i/CRATER_SEGMENTS;
        float a1=2.0f*M_PI*(i+1)/CRATER_SEGMENTS;
        float v0[3]={0,cBot,0};
        float v1[3]={CRATER_RADIUS*0.8f*cosf(a0),cBot,CRATER_RADIUS*0.8f*sinf(a0)};
        float v2[3]={CRATER_RADIUS*0.8f*cosf(a1),cBot,CRATER_RADIUS*0.8f*sinf(a1)};
        addTriangle(v0,v1,v2);
    }

    std::cout<<"Tạo núi lửa xong: "<<vertices.size()/3<<" đỉnh\n";
}

// Vertex shader
const char* vertexShaderSource = R"(
#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
uniform mat4 uTransform;
out vec3 vNormal;
out vec3 vPos;
void main(){
    vPos = aPos;
    vNormal = aNormal;
    gl_Position = uTransform*vec4(aPos,1.0);
}
)";

// Fragment shader
const char* fragmentShaderSource = R"(
#version 330 core
in vec3 vNormal;
in vec3 vPos;
out vec4 FragColor;
vec3 getVolcanoColor(float height){
    vec3 deepBrown = vec3(0.3,0.15,0.05);
    vec3 earthBrown = vec3(0.5,0.25,0.1);
    vec3 rockGray = vec3(0.4,0.35,0.3);
    vec3 volcanicOrange = vec3(0.8,0.3,0.1);
    vec3 lavaRed = vec3(1.0,0.2,0.05);
    if(height<0.5) return mix(deepBrown,earthBrown,height*2.0);
    else if(height<1.2) return mix(earthBrown,rockGray,(height-0.5)/0.7);
    else if(height<2.0) return mix(rockGray,volcanicOrange,(height-1.2)/0.8);
    else return mix(volcanicOrange,lavaRed,(height-2.0)/0.5);
}
void main(){
    vec3 lightDir = normalize(vec3(1.0,2.0,1.0));
    float diff = max(dot(normalize(vNormal),lightDir),0.36);
    vec3 baseColor = getVolcanoColor(vPos.y);
    float lavaGlow = (vPos.y>2.1)?(sin(vPos.x*10.0+vPos.z*10.0)*0.3+0.7):0.0;
    baseColor += vec3(0.3,0.1,0.0)*lavaGlow;
    FragColor = vec4(baseColor*diff,1.0);
}
)";

// Compile shader
GLuint compileShader(){
    GLuint vs=glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs,1,&vertexShaderSource,NULL);
    glCompileShader(vs);
    GLint success; char info[512];
    glGetShaderiv(vs,GL_COMPILE_STATUS,&success);
    if(!success){glGetShaderInfoLog(vs,512,NULL,info);std::cout<<"Vertex Shader Error:\n"<<info<<std::endl;}
    GLuint fs=glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs,1,&fragmentShaderSource,NULL);
    glCompileShader(fs);
    glGetShaderiv(fs,GL_COMPILE_STATUS,&success);
    if(!success){glGetShaderInfoLog(fs,512,NULL,info);std::cout<<"Fragment Shader Error:\n"<<info<<std::endl;}
    GLuint prog=glCreateProgram();
    glAttachShader(prog,vs);glAttachShader(prog,fs);glLinkProgram(prog);
    glGetProgramiv(prog,GL_LINK_STATUS,&success);
    if(!success){glGetProgramInfoLog(prog,512,NULL,info);std::cout<<"Shader Program Link Error:\n"<<info<<std::endl;}
    glDeleteShader(vs);glDeleteShader(fs);
    return prog;
}

// Setup buffers
void setupBuffers(){
    glGenVertexArrays(1,&VAO);
    glGenBuffers(2,VBO);
    glBindVertexArray(VAO);

    // Vertex
    glBindBuffer(GL_ARRAY_BUFFER,VBO[0]);
    glBufferData(GL_ARRAY_BUFFER,vertices.size()*sizeof(float),vertices.data(),GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,0,(void*)0);
    glEnableVertexAttribArray(0);

    // Normal
    glBindBuffer(GL_ARRAY_BUFFER,VBO[1]);
    glBufferData(GL_ARRAY_BUFFER,normals.size()*sizeof(float),normals.data(),GL_STATIC_DRAW);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,0,(void*)0);
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

// Input callbacks
void mouseButtonCallback(GLFWwindow* w,int b,int a,int m){
    if(b==GLFW_MOUSE_BUTTON_LEFT){
        mousePressed=(a==GLFW_PRESS);
        if(mousePressed) glfwGetCursorPos(w,&lastX,&lastY);
    }
}
void cursorPosCallback(GLFWwindow* w,double x,double y){
    if(mousePressed){
        rotationY += (x-lastX)*0.01f;
        rotationX += (y-lastY)*0.01f;
        if(rotationX>1.5f) rotationX=1.5f;
        if(rotationX<-1.5f) rotationX=-1.5f;
        lastX=x; lastY=y;
    }
}
void scrollCallback(GLFWwindow* w,double xoffset,double yoffset){
    zoom += yoffset*0.1f;
    if(zoom<0.2f) zoom=0.2f;
    if(zoom>3.0f) zoom=3.0f;
}

int main(){
    std::cout<<"Khởi tạo núi lửa 3D..."<<std::endl;
    if(!glfwInit()){std::cout<<"GLFW init error\n";return -1;}
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH,SCR_HEIGHT,"Nui Lua 3D",NULL,NULL);
    if(!window){std::cout<<"Window creation error\n";glfwTerminate();return -1;}
    glfwMakeContextCurrent(window);
    if(glewInit()!=GLEW_OK){std::cout<<"GLEW init error\n";return -1;}
    std::cout<<"OpenGL version: "<<glGetString(GL_VERSION)<<std::endl;

    createDetailedVolcano();
    setupBuffers();
    shaderProgram = compileShader();

    glfwSetMouseButtonCallback(window,mouseButtonCallback);
    glfwSetCursorPosCallback(window,cursorPosCallback);
    glfwSetScrollCallback(window,scrollCallback);

    glEnable(GL_DEPTH_TEST);

    while(!glfwWindowShouldClose(window)){
        if(glfwGetKey(window,GLFW_KEY_ESCAPE)==GLFW_PRESS) glfwSetWindowShouldClose(window,true);
        glClearColor(0.7f,0.9f,1.0f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);

        Matrix4x4 rotMat = rotateXY(rotationX,rotationY);
        Matrix4x4 scaleMat = scale(0.3f*zoom,0.3f*zoom,0.3f*zoom);
        Matrix4x4 transMat = translate(0.0f,-0.5f,0.0f);
        Matrix4x4 finalMat = multiply(transMat,multiply(rotMat,scaleMat));

        glUniformMatrix4fv(glGetUniformLocation(shaderProgram,"uTransform"),1,GL_FALSE,finalMat.m);
        glDrawArrays(GL_TRIANGLES,0,vertices.size()/3);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1,&VAO);
    glDeleteBuffers(2,VBO);
    glDeleteProgram(shaderProgram);
    glfwTerminate();
    std::cout<<"Thoát chương trình.\n";
    return 0;
}
