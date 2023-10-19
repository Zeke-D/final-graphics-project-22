#include "Obj.hpp"
#include <sstream>
#include <iostream>
#include <fstream>
#include "GraphicsProgram.hpp"

GraphicsProgram::GraphicsProgram(int w, int h): screenWidth(w), screenHeight(h) {
  bool success = true;
  std::stringstream errorStream;
  gWindow = NULL;
  
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    errorStream << "SDL failed to initialize. SDL Error: " << SDL_GetError() << std::endl;
    success = false;
  }
  else {
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 4 );
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 1 );
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );

    // SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 24);
    
    gWindow = SDL_CreateWindow("Model Loader",
      SDL_WINDOWPOS_UNDEFINED,
      SDL_WINDOWPOS_UNDEFINED,
      screenWidth,
      screenHeight,
      SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN
    );
    
    if (gWindow == NULL) {
      errorStream << "Window could not be created. SDL Error: " << SDL_GetError() << std::endl;
      success = false;
    }
    
    gContext = SDL_GL_CreateContext( gWindow );
    if (gContext == NULL) {
      errorStream << "OpenGL Context could not be created. SDL Error: " << SDL_GetError() << std::endl;
      success = false;
    }
    
    if (!gladLoadGLLoader(SDL_GL_GetProcAddress)) {
      errorStream << "Failed to initialize GLAD." << std::endl;
      success = false;
    }

    if (!InitGL()) {
      errorStream << "Failed to initialize OpenGL." << std::endl;
      success = false;
    }
    
  }
  
  if (!success) {
    std::cout << errorStream.str() << std::endl;
    return;
  }

  std::cout << "No GLAD, SDL, or OpenGL errors on initialization." << std::endl;
  isRunning = true;
  activeObjectInd = 0;
  
  Framebuffer* newFramebuffer = new Framebuffer();
  newFramebuffer->Create(w, h);
  m_frameBuffers.push_back(newFramebuffer);
  
}

GraphicsProgram::~GraphicsProgram() {

  SDL_DestroyWindow( gWindow );
  gWindow = nullptr;
  SDL_Quit();

  for (Framebuffer* fb : m_frameBuffers) {
    delete fb;
  }

}

void GraphicsProgram::EnableGLFeatures() {
  
  glDepthMask(true); // maybe necessary for depth buffer clear?
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_TEXTURE_2D);
  
}


bool GraphicsProgram::InitGL() {
  bool success = true;
  
  // enable gl features
  EnableGLFeatures();
  
  
  // setup object framebuffer
  // First make the texture that we will render texture coordinates to
  glGenTextures(1, &objectTexCoordTexture);
  glActiveTexture(GL_TEXTURE1); // TODO(zeke) does this need to be different from GL_TEXTURE0??
  glBindTexture(GL_TEXTURE_2D, objectTexCoordTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); 
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, screenWidth, screenHeight, 0, GL_RGB, GL_UNSIGNED_INT, 0);
  
  // Next make the render buffer we will output the image to
  glGenRenderbuffers(1, &objectRenderBuffer);
  glBindRenderbuffer(GL_RENDERBUFFER, objectRenderBuffer);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_RGB, screenWidth, screenHeight);

  glGenFramebuffers(1, &objectFBO);
  glBindFramebuffer(GL_FRAMEBUFFER, objectFBO);
  EnableGLFeatures();
  
  // set the bound framebuffer's renderbuffer to our object renderbuffer at color attachment 0
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, objectRenderBuffer);
  // set the second output of the framebuffer to be our texture holding texture coords of each frag
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, objectTexCoordTexture, 0);
  GLenum drawBuffs[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
  glDrawBuffers(2, drawBuffs);
  
  // setup the depth buffer
  glGenRenderbuffers(1, &objectDepthRenderBuffer);
  glBindRenderbuffer(GL_RENDERBUFFER, objectDepthRenderBuffer);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, screenWidth, screenHeight);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, objectDepthRenderBuffer);
  
  
  // be nice, Reset back to default
  glBindFramebuffer(GL_FRAMEBUFFER, 0); // bind framebuffer back to default
  glBindRenderbuffer(GL_RENDERBUFFER, 0);
  glBindTexture(GL_TEXTURE_2D, 0);


  
  // setup painting framebuffer
  glGenFramebuffers(1, &paintingFBO);
  glBindFramebuffer(GL_FRAMEBUFFER, paintingFBO);

  glGenTextures(1, &previousFrameTexture);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, previousFrameTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); 
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, screenWidth, screenHeight, 0, GL_RGB, GL_UNSIGNED_INT, 0);
  glBindTexture(GL_TEXTURE_2D, 0);
  
  glGenRenderbuffers(1, &paintingRenderBuffer);
  glBindRenderbuffer(GL_RENDERBUFFER, paintingRenderBuffer);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_RGB, screenWidth, screenHeight);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, paintingRenderBuffer);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glBindRenderbuffer(GL_RENDERBUFFER, 0);
  
  // setup shader
  std::string vertShader = paintingShader->LoadShader("./shaders/fboVert.glsl");
  std::string fragShader = paintingShader->LoadShader("./shaders/fboFrag.glsl");
  paintingShader->CreateShader(vertShader, fragShader, "");

  float x{0}, y{0}, w{1}, h{1};
  // setup quad
  float quad[] = {
      // First triangle
      // x and y cooridnates followed by texture coordinates
      x-w, y+h,  0.0f, 1.0f, // x,y,s,t
      x-w, y-h,  0.0f, 0.0f,
      x+w, y-h,  1.0f, 0.0f,
      // Second triangle
      x+-w, y+h,  0.0f, 1.0f,
      x+ w, y-h,  1.0f, 0.0f,
      x+ w, y+h,  1.0f, 1.0f
  };

  // screen quad VAO
  glGenVertexArrays(1, &quadVAO);
  glGenBuffers(1, &quadVBO);
  glBindVertexArray(quadVAO);

  glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(quad), &quad, GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2*sizeof(float)));
  
  return success;
}


bool GraphicsProgram::CheckLinkStatus(GLuint program) {
  GLuint id;
  int result;
  glGetProgramiv(program, GL_LINK_STATUS, &result);
  if (result == GL_FALSE) {
    int length;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
    char* errorMsgs = new char[length];
    glGetProgramInfoLog(program, length, &length, errorMsgs);
    std::cout << "ERROR: Linking program failed. " << errorMsgs << std::endl;
    delete[] errorMsgs;
    return false;
  }
  return true;
}



void GraphicsProgram::Loop() {


  // generate the buffers  
  for (Obj* obj : objects) {
    obj->CopyObjectToGPU();
  }

  objects[activeObjectInd]->Bind();
  objects.at(activeObjectInd)->texture.SetTexID(previousFrameTexture);
  
  
  while (isRunning) {
    HandleTextInput();
    Update();
    Render();
  }
}

void GraphicsProgram::Update() {
  // only update the active object
  objects.at(activeObjectInd)->Update(screenWidth, screenHeight);
}

void GraphicsProgram::Render() {
  

  // render the object to the object fbo
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, previousFrameTexture);
  glBindFramebuffer(GL_FRAMEBUFFER, objectFBO);
  // glClearColor( 239.f/ 255.0f, 194.f/ 255.0f, 11.f/ 255.0f, 1.f);
  glClearColor( 10.f/ 255.0f, 10.f/ 255.0f, 30.f/ 255.0f, 1.f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  objects.at(activeObjectInd)->Render();

  // copy this object image to the default render buffer
  glBindFramebuffer(GL_READ_FRAMEBUFFER, objectFBO);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
  glBlitFramebuffer(0, 0, screenWidth, screenHeight, 0, 0, screenWidth, screenHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST);
  
  // render the debug texture
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, paintingFBO);
  glBindVertexArray(quadVAO);
  glEnable(GL_TEXTURE_2D);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, previousFrameTexture);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, objectTexCoordTexture);
  paintingShader->Bind();
  paintingShader->SetUniform1i("u_DiffuseMap", 0);
  paintingShader->SetUniform1i("u_TexCoord", 1);
  glDrawArrays(GL_TRIANGLES, 0, 6);
  glBindFramebuffer(GL_READ_FRAMEBUFFER, paintingFBO);
  // /*
  // this block renders the texture on the lower left of the screen
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
  glBlitFramebuffer(0, 0, screenWidth, screenHeight, 0, 0, screenWidth / 5., screenHeight / 5., GL_COLOR_BUFFER_BIT, GL_NEAREST);
  // */
  // draw the texture coordinates debug texture on second from lower left
  glBindFramebuffer(GL_READ_FRAMEBUFFER, objectFBO);
  glReadBuffer(GL_COLOR_ATTACHMENT1);
  glBlitFramebuffer(0, 0, screenWidth, screenHeight, 0, screenHeight/5., screenWidth / 5., 2 * screenHeight / 5., GL_COLOR_BUFFER_BIT, GL_NEAREST);
  glReadBuffer(GL_COLOR_ATTACHMENT0);



  

  
  glBindFramebuffer(GL_READ_FRAMEBUFFER, paintingFBO);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, previousFrameTexture);
  glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 0, 0, screenWidth, screenHeight, 0);
  
  glBindTexture(GL_TEXTURE_2D, 0);
  
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  SDL_Delay(25);
  SDL_GL_SwapWindow(gWindow);


}

void GraphicsProgram::HandleTextInput() {
  SDL_Event e;
  while (SDL_PollEvent(&e) != 0) {
    if (e.type == SDL_QUIT) {
      isRunning = false;
    }
    if (e.type == SDL_MOUSEMOTION) {
      // std::cout << e.motion.x << ", " << e.motion.y << std::endl;
      paintingShader->SetUniform2f("u_mouseCoord", (float)e.motion.x, (float)e.motion.y);
    }
    if (e.type == SDL_KEYDOWN) {
      if (e.key.keysym.sym >= SDLK_1 && e.key.keysym.sym <= SDLK_9
      && e.key.keysym.sym - SDLK_1 < objects.size()) {
        std::cout << "Switching to object " << e.key.keysym.sym - SDLK_1 << std::endl;
        activeObjectInd = e.key.keysym.sym - SDLK_1;
        // CopyObjectToGPU(objects[activeObjectInd]);
        // objects[e.key.keysym.sym - SDLK_1]->texture.CopyTextureToGPU(false);
      }
      switch (e.key.keysym.sym) {
        case SDLK_q: {
          isRunning = false;
          break;
        }
        case SDLK_w: {
          wireframe = !wireframe;
          if (wireframe) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
          }
          else {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
          }
          
        }
        default:
          break;
      }
    }
  }
}

void GraphicsProgram::RegisterObject(Obj* obj) {
  objects.push_back(obj);
}
