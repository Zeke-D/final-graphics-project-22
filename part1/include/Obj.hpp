#ifndef OBJPARSER
#define OBJPARSER

#include <string>
#include <vector>
#include <iostream>
#include <fstream>

#include "Texture.hpp"
#include "Shader.hpp"
#include "Transform.hpp"

#include "glad/glad.h"

#include "glm/vec3.hpp"
#include "glm/gtc/matrix_transform.hpp"

enum Buffer {
  VERTICES,
  FACE_INDICES,
  LENGTH
};

// takes indices of GL_Vertices in CCW winding order
struct Triangle {
  GLuint i1, i2, i3;
};

struct Vector3f {
  GLfloat x, y, z;
};

struct Vector2f {
  GLfloat x, y;
};

struct GL_Vertex {
  Vector3f position;
  Vector2f texture;
  Vector3f normal;
};

class Obj {

  public:

    Obj();
    ~Obj();
  
    void FromFile(std::string filename);
    void Update(unsigned int screenWidth, unsigned int screenHeight);
    void Render();
    void Bind();
    void DeleteVBO();
    void GenVBO();
    void AddShaders();
    void ParseFaceLines();
    void ParseMaterialFile();
    void CopyObjectToGPU();
    void print();


    std::string materialFile = "";
    std::string rootPath;
    std::string diffuseTex;
  
    Texture texture;
    Shader shader;
    Transform transform;
    glm::mat4 projectionMatrix;
    float rotAmt;

    std::vector<Vector3f> positions;
    std::vector<Vector2f> textureCoords;
    std::vector<Vector3f> normals;

    std::vector<std::string> faceLines;
    std::vector<GL_Vertex> vertexData;
    std::vector<Triangle> faceIndices;
    GLuint buffers[Buffer::LENGTH];

  private:
    unsigned int currentObjFrame = 0;
    unsigned int numberObjIndices = 0;
    GLuint VAOId;

};

#endif
