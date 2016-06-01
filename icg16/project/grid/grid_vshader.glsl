#version 330

in vec2 position;

uniform int enableTerrain;

uniform mat4 MV, P;
uniform sampler2D tex_grid;

uniform vec3 light_pos;

out vec3 light_dir, N_mv, view_dir;
out float height;
out vec2 uv;

void main() {
    uv = (position + vec2(64.f)) / 128.f;
    height = enableTerrain == 1 ? texture(tex_grid, uv).r : 0.f;

    vec3 pos_3d = vec3(position.x, height, -position.y);
    vec4 vpoint_mv = MV * vec4(pos_3d, 1.0);
    gl_Position = P * vpoint_mv;

    vec3 off = vec3(1/ 1024.0, 1 / 1024.0, 0);
    float hL = texture(tex_grid, uv - off.xz).r;
    float hR = texture(tex_grid, uv + off.xz).r;
    float hD = texture(tex_grid, uv - off.zy).r;
    float hU = texture(tex_grid, uv + off.zy).r;

    vec3 N = vec3(0.0);
    N.x = hL - hR;
    N.y = hD - hU;
    N.z = 1.0 / 1024.0;
    N = normalize(N);

    vec4 tmp = MV * vec4(N, 0.f);
    N_mv = normalize(tmp.xyz);
    N_mv = N_mv;

    // compute the light direction light_dir.
    light_dir = normalize(vec4(light_pos, 1.0) - vpoint_mv).xyz;
    view_dir = normalize(vec4(0,0,0,1) - vpoint_mv).xyz;
}
