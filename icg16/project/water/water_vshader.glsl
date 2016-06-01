#version 330

in vec2 position;
out vec2 uv;
out vec3 light_dir, view_pos;
out float time;
out vec4 pos;

uniform mat4 MV, P;
uniform vec3 light_pos;
uniform float uniTime;


float height(vec2 pos) {
    float amplitude = 0.00321f;//0.0625f;
    float height = 2.5f *amplitude * ((sin(uniTime * 2.f + (pos.x + pos.y +0.02f)*3.14159f*10.0f) + 1.f)/2.f);
    return height;
}

void main() {
    uv = (position + vec2(128.f)) / 256.f;

    vec3 pos_3d = vec3(position.x, height(uv * 32), -position.y);
    vec4 vpoint_mv = MV * vec4(pos_3d, 1.0);
    gl_Position = P * vpoint_mv;

    vec3 off = vec3(1/ 2048.0, 1 / 2048.0, 0);
    float hL = height(uv - off.xz);
    float hR = height(uv + off.xz);
    float hD = height(uv - off.zy);
    float hU = height(uv + off.zy);

    vec3 N = vec3(0.0);
    N.x = hL - hR;
    N.y = hD - hU;
    N.z = 1.0 / 2048.0;
    N = normalize(N);
    vec4 tmp = MV * vec4(N, 0.f);

    view_pos = normalize(tmp.xyz);
    light_dir = normalize(vec4(light_pos, 1.0) - vpoint_mv).xyz;
    time = uniTime/80;
    pos = gl_Position;
}

