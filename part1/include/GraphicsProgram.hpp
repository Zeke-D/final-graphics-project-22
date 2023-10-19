#ifndef GRAPHICSPROGRAM
#define GRAPHICSPROGRAM

#include <SDL2/SDL.h>
#include "glad/glad.h"
#include <string>
#include "Framebuffer.hpp"

class GraphicsProgram {
  public:
    GraphicsProgram(int w, int h);
    ~GraphicsProgram();
    void Update();
    void Render();
    void Loop();
    void HandleTextInput();
    GLuint GetGPUBufferName(Buffer b);
    bool InitGL();
    void EnableGLFeatures();
    bool CheckLinkStatus(GLuint program);
    void RegisterObject(Obj* obj);

  private:
    unsigned int screenWidth;
    unsigned int screenHeight;
    bool isRunning;
    bool wireframe = true;
    unsigned int numberObjIndices;
    unsigned int activeObjectInd = 0;

    SDL_Window* gWindow;
    SDL_GLContext gContext;
    // the buffers, one for the vertices, one for the normals, one for the faces (vert / norm indices)
    std::vector<Obj*> objects;
    std::vector<Framebuffer*> m_frameBuffers;

    // testing stuff
    // literally proof of concepting these in place
    GLuint paintingFBO;
    GLuint previousFrameTexture;
    GLuint paintingRenderBuffer;
    std::shared_ptr<Shader> paintingShader = std::make_shared<Shader>();
    GLuint quadVAO;
    GLuint quadVBO;
  
    GLuint objectFBO;
    GLuint objectTexCoordTexture; // where we render the object textcoords to, first color-attachment
    GLuint objectRenderBuffer;   // where we render the actual shaded object to, second color-attachment
    GLuint objectDepthRenderBuffer; // where the depth component will be stored

};
#endif
