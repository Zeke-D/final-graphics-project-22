#include "Obj.hpp"
#include "GraphicsProgram.hpp"

int main(int argc, char** argv) {
  
  if (argc < 2) {
    std::cout << "Please provide a .obj file to load as a command line argument." << std::endl;
    return 0;
  }
  
  std::cout << "Reading file: " << argv[1] << std::endl;
  
  GraphicsProgram prog(1200, 740);
  
  std::vector<Obj*> objects;

  // Load the obj file
  for (int i = 1; i < argc && i <= 9; i++) {
    Obj* obj = new Obj();
    obj->FromFile(argv[i]);
    objects.push_back(obj);
    prog.RegisterObject(obj);
    // obj.print();
  }

  // Render the object
  prog.Loop();
  
  // deallocate objects when program ends
  for (Obj* obj : objects) {
    delete obj;
  }

  return 0;  
}
