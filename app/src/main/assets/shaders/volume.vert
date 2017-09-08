#version 300 es
precision mediump float;

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texCoord;

uniform mat4 Projection;
uniform mat4 Modelview;

out vec3 ray_entry_position;
out vec2 frag_uv;
out vec4 o_color;

void main()
{
    ray_entry_position = position;
    vec4 Position = vec4(position, 1.0);
    gl_Position = Projection * Modelview * Position;
}
