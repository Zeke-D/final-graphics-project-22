#include <cstring>
#include <string.h>
#include <sstream>
#include "Obj.hpp"
#include <unordered_map>

/*
#if defined(_WIN32)
#define strtok_r strtok_r
#endif
*/

Obj::Obj() {
  // initialize null data indices
  // we do this for a few reasons:
  // - the first is to not have to do weird -1 math later since .obj files are
  //   1-indexed
  // - the second is to make the map from our 3D space of pos/tx/nor coords ->
  //   1D space of index in VBO map work nicely (a size of 0 flattens the space)
  positions.push_back({0, 0, 0});
  textureCoords.push_back({0, 0});
  normals.push_back({0, 0, 0});
}

Obj::~Obj() {
  glDeleteBuffers(Buffer::LENGTH, buffers);
}

void Obj::FromFile(std::string filename) {
  
  std::fstream objFile;
  objFile.open(filename, std::ios::in);
  rootPath = filename.substr(0, filename.find_last_of("/") + 1);

  if (!objFile.is_open()) {
    std::cout << "Error opening object file at path " << filename << std::endl;
  }

  std::string lineType;

  while (objFile >> lineType) {
    if (lineType == "v") {
      Vector3f position;
      objFile >> position.x >> position.y >> position.z;
      positions.push_back(position);
    }
    else if (lineType == "vt") {
      Vector2f texture;
      objFile >> texture.x >> texture.y;
      textureCoords.push_back(texture);
    }
    else if (lineType == "vn") {
      Vector3f normal;
      objFile >> normal.x >> normal.y >> normal.z;
      normals.push_back(normal);
    }
    else if (lineType == "f") {
      std::string curLine;
      getline(objFile, curLine);
      faceLines.push_back(curLine);
    }
    else if (lineType == "mtllib") {
      objFile >> materialFile;
    }
    else {
      // burn the rest of the line
      std::string burned;
      getline(objFile, burned);
    }
  }

  // now that we are done with the file, we can go back and create faces
  ParseFaceLines();

  // ParseMaterialFile();
  
  AddShaders();
  
}

void Obj::AddShaders() {
  
  std::string vertexShader   = shader.LoadShader("./shaders/vert.glsl");
  std::string geometryShader = shader.LoadShader("./shaders/geom.glsl");
  std::string fragmentShader = shader.LoadShader("./shaders/frag.glsl");
  shader.CreateShader(vertexShader, fragmentShader, geometryShader);

}

void Obj::ParseMaterialFile() {
  if (materialFile == "") {
    std::cout << "No material file.\n";
    return;
  }
  std::cout << "Parsing material file: " << materialFile << std::endl;

  std::fstream mtlFile;
  mtlFile.open(rootPath + materialFile, std::ios::in);

  if (!mtlFile.is_open()) {
    std::cout << "Error opening object file at path " << (rootPath + materialFile) << std::endl;
  }

  std::string currentLine;

  while (getline(mtlFile, currentLine)) {
    std::istringstream lineAsStream(currentLine);
    std::string lineType;
    lineAsStream >> lineType;
    if (lineType != "map_Kd") {
      continue;  
    }
    lineAsStream >> diffuseTex;
    break;
  }
  
  if (diffuseTex == "") {
    return;
  }
  
  texture.LoadTexture((rootPath + diffuseTex).c_str(), true);
}

void Obj::ParseFaceLines() {
  
  
  // std::vector<int> vboIndMap;
  // vboIndMap.reserve(positions.size() * textureCoords.size() * normals.size());
  // std::cout << "Attempting to allocate " << (positions.size() * sizeof(Vector3f) * textureCoords.size() * sizeof(Vector2f) * normals.size() * sizeof(Vector3f)) << " bytes..." << std::endl;
  // int* vboIndMap = new int[positions.size() * textureCoords.size() * normals.size()];
  // memset(vboIndMap, -1, positions.size() * textureCoords.size() * normals.size() * sizeof(int));
  /*
  for (int i = 0; i < positions.size() * textureCoords.size() * normals.size(); i++) {
    vboIndMap[i] = -1;
  }
  */

  
  /*
  when adding a face, what we want to do (to do this at some level 
  efficiently) is parse each posInd/texInd/normInd by keeping a map of
  this thruple to an index already in the VBO (for efficiency, this can 
  be an array of length numPositions * numTextinds * numNormals so we don't
  have to run a search on the VBO data every time). if the index 
  is -1, then it is not in the VBO yet and we can add it to the vbo data
  and update the index in the map to reflect its VBO position.
  */
  // Sample face lines:
  //     1//1 5//5 2//2
  //     1/2/3 5/5/3 2/1/2
    
  
  // this needs to be optimized to use a hashmap instead of allocating a massive fuckin array
  struct IndexKey {
    unsigned int posInd;
    unsigned int texInd;
    unsigned int normInd;
  };

  auto ToKey = [this](IndexKey ik) {
    return ik.posInd + ik.texInd * positions.size() + ik.normInd * positions.size() * textureCoords.size();
  };


  std::unordered_map<int, int> vboIndMap;
  
  for (std::string faceLine : faceLines) {

    char* start = new char[faceLine.length() + 1];
    strcpy(start, faceLine.c_str());
    IndexKey readIndexKey;
    unsigned int faceInds[3];

    for (int i = 0; i < 3; i++) {
  
      char* curVert = strtok_r(start, " ", &start);
      char* indStart = curVert;
      GLuint index[3];
      char* indStr;
  
      int count = 0;
      while ((indStr = strtok_r(indStart, "/", &indStart))) {
        index[count++] = atoi(indStr);
      }
  
      if (count == 1) {
        readIndexKey = {
          index[0],
          index[0],
          index[0]
        };
      }
      else if (count == 2) {
        readIndexKey = {
          index[0],
          0,
          index[1]
        };
      }
      else if (count == 3) {
        readIndexKey = {
          index[0],
          index[1],
          index[2]
        };
      }
      else {
        // error parsing, skip it
        continue;
      }

      if (vboIndMap.find(ToKey(readIndexKey)) == vboIndMap.end()) {
        GL_Vertex newVertData = {
          positions.at(readIndexKey.posInd),
          textureCoords.at(readIndexKey.texInd),
          normals.at(readIndexKey.normInd)
        };
        vertexData.push_back(newVertData);
        vboIndMap[ToKey(readIndexKey)] = vertexData.size() - 1;
      }

      faceInds[i] = vboIndMap.at(ToKey(readIndexKey));
    }
    
    delete[] start;

    Triangle face;
    
    face.i1 = faceInds[0];
    face.i2 = faceInds[1];
    face.i3 = faceInds[2];
    
    faceIndices.push_back(face);
  }
  
  // delete[] vboIndMap;
  
}

void Obj::Update(unsigned int screenWidth, unsigned int screenHeight){
  texture.Unbind();
  // Make sure we are updating the correct 'buffers'
  // m_vertexBufferLayout.Bind();
  texture.Bind();  
  shader.Bind();
  // For our object, we apply the texture in the following way
  // Note that we set the value to 0, because we have bound
  // our texture to slot 0.
  shader.SetUniform1i("u_Texture",0);
  
  currentObjFrame = (currentObjFrame + 1) % 10000;
  shader.SetUniform1i("frame", currentObjFrame);

   // Here we apply the 'view' matrix which creates perspective.
  // The first argument is 'field of view'
  // Then perspective
  // Then the near and far clipping plane.
  // Note I cannot see anything closer than 0.1f units from the screen.
  projectionMatrix = glm::perspective(45.0f,(float)screenWidth/(float)screenHeight,1.f,4.0f);
  // Set the uniforms in our current shader
  // transform.Scale(.4, .4, .4);
  transform.LoadIdentity();

  rotAmt += .0005;
  if (rotAmt > 1) {
    rotAmt = 0;
  }
  transform.Translate(0, -.8, -2);
  transform.Rotate(rotAmt * 2 * 3.1415927, 0, 1, 0);
  shader.SetUniformMatrix4fv("modelTransformMatrix",transform.GetTransformMatrix());
  shader.SetUniformMatrix4fv("projectionMatrix", &projectionMatrix[0][0]);
}

void Obj::Bind() {
  shader.Bind();
  glBindVertexArray(VAOId);
  glBindBuffer(GL_ARRAY_BUFFER, buffers[Buffer::VERTICES]);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers[Buffer::FACE_INDICES]);
}

void Obj::GenVBO() {
  DeleteVBO();
  glGenVertexArrays(1, &VAOId);
  glBindVertexArray(VAOId);
  glGenBuffers(Buffer::LENGTH, buffers);
}

void Obj::DeleteVBO() {
  glDeleteBuffers(Buffer::LENGTH, buffers);
}


void Obj::Render() {
  Bind();
  glDrawElements(GL_TRIANGLES, numberObjIndices, GL_UNSIGNED_INT, nullptr);
}

void Obj::CopyObjectToGPU() {
  GenVBO();
  // set up vertice buffer (where a vertex contains both position and normal data)
  glBindBuffer(GL_ARRAY_BUFFER, buffers[Buffer::VERTICES]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GL_Vertex) * vertexData.size(), vertexData.data(), GL_STATIC_DRAW);
  
  // enable position attrib
  glEnableVertexAttribArray(0);
  // we have the stride set to the normal size
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GL_Vertex), 0);

  // enable texCoord attrib
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(GL_Vertex),
     (GLvoid*)offsetof(GL_Vertex, texture));
  
  // enable normal attrib
  glEnableVertexAttribArray(2);
  // we have the stride set to the position size and the offset to the position size
  glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(GL_Vertex), 
    (GLvoid*)offsetof(GL_Vertex, normal));

  // setup triangles / faces buffer
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers[Buffer::FACE_INDICES]);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Triangle) * faceIndices.size(), faceIndices.data(), GL_STATIC_DRAW);
  numberObjIndices = faceIndices.size() * 3;
}


void Obj::print() {
  std::cout << "Printing obj file parse." << std::endl << std::endl;

  std::cout << "Faces:" << std::endl;
  
  int i = 0;
  for (Triangle tri : faceIndices) {
    std::cout << "Face " << i << ":\n";

    std::cout << "vertexData[" << tri.i1 << "] = {\n\t"
      << "\n\tposition: { x: " << vertexData.at(tri.i1).position.x
        << ", y: " << vertexData.at(tri.i1).position.y
        <<", z: " << vertexData.at(tri.i1).position.z << "}"
      << "\n\tnormal: { x: " << vertexData.at(tri.i1).normal.x
        << ", y: " << vertexData.at(tri.i1).normal.y
        <<", z: " << vertexData.at(tri.i1).normal.z << "}\n";

    std::cout << "vertexData[" << tri.i2 << "] = {\n\t"
      << "\n\tposition: { x: " << vertexData.at(tri.i2).position.x
        << ", y: " << vertexData.at(tri.i2).position.y
        <<", z: " << vertexData.at(tri.i2).position.z << "}"
      << "\n\tnormal: { x: " << vertexData.at(tri.i2).normal.x
        << ", y: " << vertexData.at(tri.i2).normal.y
        <<", z: " << vertexData.at(tri.i2).normal.z << "}\n";

    std::cout << "vertexData[" << tri.i3 << "] = {\n\t"
      << "\n\tposition: { x: " << vertexData.at(tri.i3).position.x
        << ", y: " << vertexData.at(tri.i3).position.y
        <<", z: " << vertexData.at(tri.i3).position.z << "}"
      << "\n\tnormal: { x: " << vertexData.at(tri.i3).normal.x
        << ", y: " << vertexData.at(tri.i3).normal.y
        <<", z: " << vertexData.at(tri.i3).normal.z << "}\n";

    i++;
  }

}