// ==================================================================
#version 330 core

layout(location=0)in vec3 position; // We explicitly state which is the vertex information 
				    // i.e., The first 3 floats are positional data, 
				    // we are putting in our position vector
layout(location=1)in vec2 texture; // Our second attribute is the color attributes of each
				       // vertex.
layout(location=2)in vec3 normal; // Our second attribute is the color attributes of each
				       // vertex.

out VS_OUT {
	vec3 normal;
	vec2 texCoord;
} vs_out;

uniform mat4 modelTransformMatrix;
uniform mat4 projectionMatrix;

void main()
{
  // gl_Position is a special glsl variable that tells us what
  // postiion to put things in.
  // It takes in exactly 4 things.
  // Note that 'w' (the 4th dimension) should be 1.
  vec4 oldPosition = vec4(position.x, position.y, position.z, 1.0f);
  vec4 newPositon = modelTransformMatrix * oldPosition;
  vec4 projectedPosition = projectionMatrix * newPositon;
	
  gl_Position = projectedPosition;
  // gl_Position = vec4(position.x / 2., position.y / 2., position.z / 2., 1.0f);
	
  vs_out.texCoord = texture;
	
  // Store the vertex color that we take in as what we will output
  // to the next stage in the graphics pipeline.
  vs_out.normal = normal;

}
// ==================================================================
