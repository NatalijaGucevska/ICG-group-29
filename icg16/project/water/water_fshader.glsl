#version 330

in vec2 uv;
in vec3 light_dir;
in vec3 view_pos;
in float time;
in vec4 pos;

uniform float offset_x;
uniform float offset_y;
uniform vec3 kd, ka, La, Ld;

uniform sampler2D reflectionTexture;
uniform sampler2D normalMapTexture;
uniform sampler2D dudvTexture;
uniform sampler2D tex_alpha;

out vec4 color;


// Constants //
float kDistortion = 0.01;
float kReflection = 0.0325;
vec4 baseColor = vec4(0, 0, 1, 1);

vec4 tangent = vec4(1.0, 0.0, 0.0, 0.0);
vec4 lightNormal = vec4(0.0, 1.0, 0.0, 0.0);
vec4 biTangent = vec4(0.0, 0.0, 1.0, 0.0);

vec3 rgb2hsv(vec3 c)
{
    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));

    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void main(void)
{
    ivec2 texture_size = textureSize(reflectionTexture, 0);
    int window_width = texture_size.x;
    int window_height = texture_size.y;
    vec2 _u_v = vec2(gl_FragCoord.x/window_width, 1 - gl_FragCoord.y / window_height);

    // Light Tangent Space //
    vec4 lightDirection = normalize(vec4(light_dir.xyz, 1.0));
    vec4 lightTanSpace = normalize(vec4(dot(lightDirection, tangent), dot(lightDirection, biTangent), dot(lightDirection, lightNormal), 1.0));

    // Fresnel Term //
    vec4 distOffset = texture(dudvTexture, uv * 128.f + vec2(time)) * kDistortion;
    vec4 normal = texture(normalMapTexture, vec2(uv * 128.f  + distOffset.xy));

    normal = normalize(normal * 2.0 - 1.0);
    normal.a = 0.21;

    vec4 lightReflection = normalize(reflect(-1 * lightTanSpace, normal));
    vec4 invertedFresnel = vec4(dot(normal, lightReflection));
    vec4 fresnelTerm = 1.0 - invertedFresnel;

    // Reflection //
    vec4 dudvColor = texture(dudvTexture, vec2(uv * 128.f + distOffset.xy));
    dudvColor = normalize(dudvColor * 2.0 - 1.0) * kReflection;

    // Projection Coordinates from http://www.bonzaisoftware.com/tnp/gl-water-tutorial/
    vec4 projCoord = vec4(_u_v.x, _u_v.y, 0.0f, 1.0f);
    projCoord += dudvColor;
    projCoord = clamp(projCoord, 0.001, 0.999);

    vec4 reflectionColor = texture(reflectionTexture, projCoord.xy);
    vec3 hsv = rgb2hsv(reflectionColor.xyz);
    hsv.z *= 0.9;
    reflectionColor = vec4(hsv2rgb(hsv), reflectionColor.a);
    reflectionColor *= fresnelTerm;

    //reflectionColor.a = texture(tex_alpha, uv).r;
    color = reflectionColor;
    //color = vec4(0.f, 0.f, 0.5f, 1.f);
    color.a = 0.8f * (1.f - texture(tex_alpha, uv).r);

 }
/*

void main() {

    // normal
    //vec3 normal_mv = normalize(cross(dFdx(vpoint_mv.xyz), dFdy(vpoint_mv.xyz)));
    //vec3 ambient = ka*La;

    // compute diffuse term.
    //texture(colormap, max(dot(normal_mv, light_dir), 0.0f)).x;
    float nl = max(dot(view_pos, light_dir.xyz), 0.0f);
    vec3 diffuse = kd*Ld*nl;;

    color = vec4(texture(reflectionTexture, uv + vec2(offset_x, offset_y)).rgb, 0.5); //+ vec4(diffuse, 0.0f);
}*/
