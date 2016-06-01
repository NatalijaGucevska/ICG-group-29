#version 330 core
in vec2 uv;
uniform int subdivision;
uniform float sub_multiplier;
uniform float amplitude;
uniform float lacunarity;
uniform int octaves;

uniform float offset_x;
uniform float offset_y;

out vec4 color;
in vec4 gl_FragCoord ;

uniform sampler1D x;
uniform sampler1D y;

float smooth_custom(float t) {
    return 6.f * t*t*t*t*t - 15.f * t*t*t*t + 10.f * t*t*t;
}

float mix_custom(float x, float y, float a) {
    return (1.f - a) * x + a * y;
}

float rand(vec2 co){
  return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

float random_i(int x, int y, int subdivision) {
    return ((y * subdivision + x) % 441) / 441.f;
}

float p_noise(float subdivision, float amplitude) {
    int subdivision_i = int(subdivision);

    vec2 uv = uv + vec2(offset_x, offset_y);
    // 18
    int x0i = int(floor(uv.x * subdivision));
    // 11
    int y1i = int(floor(uv.y * subdivision));

    // 19
    int x1i = int(ceil(uv.x * subdivision));
    // 10
    int y0i = int(ceil(uv.y * subdivision));

    float x0 = float(x0i)/subdivision;
    float y1 = float(y1i)/subdivision;
    float x1 = float(x1i)/subdivision;
    float y0 = float(y0i)/subdivision;

    vec2 gx0y1 = vec2(texture(x, random_i(x0i, y1i, subdivision_i)).r, texture(y, random_i(x0i, y1i, subdivision_i)).r);
    vec2 gx1y1 = vec2(texture(x, random_i(x1i, y1i, subdivision_i)).r, texture(y, random_i(x1i, y1i, subdivision_i)).r);
    vec2 gx0y0 = vec2(texture(x, random_i(x0i, y0i, subdivision_i)).r, texture(y, random_i(x0i, y0i, subdivision_i)).r);
    vec2 gx1y0 = vec2(texture(x, random_i(x1i, y0i, subdivision_i)).r, texture(y, random_i(x1i, y0i, subdivision_i)).r);
    //vec2 gx0y1 = vec2(rand(vec2(x0i, y1i)), rand(vec2(y1i, x0i)));
    //vec2 gx1y1 = vec2(rand(vec2(x1i, y1i)), rand(vec2(y1i, x1i)));
    //vec2 gx0y0 = vec2(rand(vec2(x0i, y0i)), rand(vec2(y0i, x0i)));
    //vec2 gx1y0 = vec2(rand(vec2(x1i, y0i)), rand(vec2(y0i, x1i)));


    vec2 c = (uv - vec2(x0, y1)) * subdivision;
    vec2 d = (uv - vec2(x1, y1)) * subdivision;
    vec2 a = (uv - vec2(x0, y0)) * subdivision;
    vec2 b = (uv - vec2(x1, y0)) * subdivision;

    float s = dot(gx0y0, a);
    float t = dot(gx1y0, b);
    float u = dot(gx0y1, c);
    float v = dot(gx1y1, d);

    float st = mix_custom(s, t, smooth_custom((uv.x - x0)*subdivision));
    float uv2 = mix_custom(u, v, smooth_custom((uv.x - x0)*subdivision));
    float noise = mix_custom(st, uv2, smooth_custom(1-(uv.y - y1)*subdivision));

    return noise * amplitude;
}

float fBm(float H, float lacunarity, int octaves) {
    float value = 0.f;
    vec2 point = vec2(gl_FragCoord / 2048.f * 100.f);
    float sub = subdivision;

    for(int i = 0; i < octaves; i++) {
        sub *= sub_multiplier;
        value += p_noise(sub, amplitude) * pow(lacunarity, -H * i);
    }

    return value;
}

void main() {
    float  r = fBm(1, lacunarity, octaves);
    // x[random_i(int(floor(uv.x * subdivision)), int(floor(uv.y * subdivision)))]
    color = vec4(r, 0.f, 0.f, 1.f);

}
