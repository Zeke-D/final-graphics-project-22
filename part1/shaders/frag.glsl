  #version 330 core
  
  in GS_OUT {
    vec3 normal;
    vec3 tanNorm;
    vec2 texCoord;
    vec4 color;
  } fr_in;

  layout(location = 0) out vec4 color;
  layout(location = 1) out vec3 texCoord;

  uniform sampler2D u_Texture;

  void main()
  {
    // some simple lighting if we want
    float lighting = .5 + .5 * dot(fr_in.tanNorm, vec3(0, 2, -1));
  
    vec3 porcelain = vec3(230., 240., 255.) / 255.;
    vec3 dirtColor = vec3(100., 50., 37.) / 255.;
    
    vec3 albedoModel = mix(porcelain, dirtColor, texture(u_Texture, fr_in.texCoord).r);
    vec3 modelColor = albedoModel * lighting;
  
    color = mix(vec4(modelColor, 1.), fr_in.color, fr_in.color.a); 
      // color = vec4(fr_in.texCoord, 0, 1); // texture(u_Texture, fr_in.texCoord);
    // color = texture(u_Texture, fr_in.texCoord).r;
  
    texCoord = vec3(fr_in.texCoord, 0);
  }
