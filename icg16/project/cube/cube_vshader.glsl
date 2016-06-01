#version 330

uniform mat4 MVP;

in vec3 vpoint;
out vec3 texcoords;

void main()
{
    gl_Position =   MVP * vec4(vpoint, 1.0);
    texcoords = vpoint;
}
