#version 330 core
layout (triangles) in;
layout (triangle_strip, max_vertices = 17) out;
// layout (triangle_strip, max_vertices = 7) out;

uniform int frame;
uniform sampler2D u_Texture;

in VS_OUT {
    vec3 normal;
    vec2 texCoord;
} gs_in[];

out GS_OUT {
    vec3 normal;
    vec3 tanNorm;
    vec2 texCoord;
    vec4 color;
} gs_out;

void CreateGrassBlade(vec4 pos0, vec4 pos1, vec4 pos2, vec3 extendDir, vec4 colA, vec4 colB) {
    
    vec4 center = (pos0 + pos1 + pos2) / 3;

    // also output the new triangle
    vec4 newPosA = .02 * normalize(pos1 - pos0) + pos0;
    // vec4 newPosA = pos0;


    // Animation Option Two, scale them as they spawn (growing effect)
    // TODO(zeke) fix this animation, it's broken (doing a wipe instead of a ping-pong)
    // float upScalar = dot(gs_in[0].normal, vec3(0, 1, 0));
    // float offset = 3.14152965 * .5 - .5 * upScalar;
    /*
    float offset = 0;
    extendDir = extendDir * .6 * (.5  + sin(3.14159265 + offset - .08 + 2 * 3.1415926 * frame / 200.));
    if (length(extendDir) < 0) return;
    */
    vec2 centerTexCoord = (gs_in[0].texCoord + gs_in[1].texCoord + gs_in[2].texCoord) / 3.;
    float centerSample = texture(u_Texture, centerTexCoord).r;

    if (centerSample == 0) return;
    extendDir *= centerSample;
    // Animation option 1
    // Nothin in this func
    
    vec4 col1 = vec4(0, .15, .1, 1);
    vec4 col2 = vec4(.2, .7, .2, 1);

    vec4 newCenter = center * 100;
    float location = sin(newCenter.y) 
        + .53 * sin(newCenter.y / 2.1839 + .4) 
        + .3*sin(newCenter.y * 3.3) 
        + .1 * sin(newCenter.y*7.124) * .02*sin(newCenter.y * 40);
    
    colA = mix(col1, colA, location);
    colB = mix(col2, colB, location);

    gl_Position = pos0;
    gs_out.color = colA;
    gs_out.texCoord = centerTexCoord;
    EmitVertex();

    gl_Position = newPosA;
    gs_out.texCoord = centerTexCoord;
    gs_out.color = colA;
    EmitVertex();

    gl_Position = pos0 + .04 * vec4(extendDir, 0);
    gs_out.color = mix(colA, colB, .4);
    gs_out.texCoord = centerTexCoord;
    gl_Position.y = gl_Position.y - .01;
    EmitVertex();
    
    gl_Position = newPosA + .04 * vec4(extendDir, 0);
    gs_out.color = mix(colA, colB, .4);
    gs_out.texCoord = centerTexCoord;
    gl_Position.y = gl_Position.y - .01;
    EmitVertex();

    gl_Position = pos0 + .08 * vec4(extendDir, 0);
    gs_out.texCoord = centerTexCoord;
    gs_out.color = mix(colA, colB, .8);
    EmitVertex();
    
    gl_Position = newPosA + .08 * vec4(extendDir, 0);
    gs_out.texCoord = centerTexCoord;
    gs_out.color = mix(colA, colB, .8);
    EmitVertex();

    gl_Position = .5 * (newPosA + pos0) + .1 * vec4(extendDir, 0);
    gs_out.texCoord = centerTexCoord;
    gl_Position.y = gl_Position.y + .02;
    gs_out.color = colB;
    EmitVertex();
    EndPrimitive();

}

void main() {
    
    // gl_in is a reserved array for incoming vertex data
    vec3 tangentNorm = vec3( normalize(cross( (gl_in[0].gl_Position - gl_in[1].gl_Position).xyz, (gl_in[2].gl_Position - gl_in[1].gl_Position).xyz))); //gs_in[0].normal;

    //output the incoming triangle
    for (int i = 0; i < 3; i++) {
        gs_out.normal = gs_in[i].normal;
        gs_out.tanNorm = tangentNorm;
        gs_out.texCoord = gs_in[i].texCoord;
        gs_out.color = vec4(0);
        gl_Position = gl_in[i].gl_Position;
        EmitVertex();
    }
    EndPrimitive();
    
    float upScalar = dot(gs_in[0].normal, vec3(0, 1, 0));

    // spawn animation based on normal to see "where grass would grow (how upward)"
    /*
    if (upScalar < sin(2 * 3.14159265 * frame / 200.0)) {
        return;
    }
    */

    vec4 col3 = vec4(3., 35., 48., 255.) / 255.;
    vec4 col4 = vec4(30., 117., 39., 255.) / 255.;

    vec4 col5 = vec4(.1, .2, .3, 1);
    vec4 col6 = vec4(120., 100., 90., 255.) / 255.;

    CreateGrassBlade(.3 * (gl_in[2].gl_Position + gl_in[0].gl_Position),
        .3 * (gl_in[1].gl_Position + gl_in[0].gl_Position),
        gl_in[0].gl_Position,
        tangentNorm,
        col3,
        col4
    );
    CreateGrassBlade(gl_in[0].gl_Position,
        gl_in[1].gl_Position,
        gl_in[2].gl_Position,
        tangentNorm,
        col5,
        col6
    );
}