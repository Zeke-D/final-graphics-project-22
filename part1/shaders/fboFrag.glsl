// ====================================================
#version 330 core

// ======================= uniform ====================
// If we have texture coordinates, they are stored in this sampler.
uniform sampler2D u_DiffuseMap; 
uniform sampler2D u_TexCoord; 
uniform vec2 u_mouseCoord;

// ======================= IN =========================
in vec2 v_texCoord; // Import our texture coordinates from vertex shader

// ======================= out ========================
// The final output color of each 'fragment' from our fragment shader.
out vec3 FragColor;

void main()
{
    // Store our final texture color
    vec3 diffuseColor = vec3(0.0);
    vec2 normMouseCoord = vec2(u_mouseCoord.x / 1200, 1. - (u_mouseCoord.y / 740));
    vec2 mouseCoordToTexCoord = texture(u_TexCoord, normMouseCoord).rg;
    
    diffuseColor = texture(u_DiffuseMap, v_texCoord).rgb;
    diffuseColor += 5 * vec3(clamp(.03 - length(v_texCoord - mouseCoordToTexCoord), 0, 1));
    // diffuseColor += clamp(.01 - length(v_texCoord.xy - mouseCoordToTexCoord), 0, 1);
    // diffuseColor = vec3(mouseCoordToTexCoord, 0);
    // diffuseColor = vec3(normMouseCoord.x, normMouseCoord.y, 0);
    FragColor = diffuseColor;
}
// ==================================================================
