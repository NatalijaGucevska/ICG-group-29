#version 330

in vec3 light_dir, N_mv, view_dir;
in float height;
in vec2 uv; // [0..1]^2
uniform float water_height;
uniform float is_reflected;

out vec4 color;
uniform float offset_x;
uniform float offset_y;
uniform vec3 kd, ka, La, Ld;
uniform sampler1D colormap;

uniform sampler2D tex_rock, tex_grass, tex_sand, tex_snow, tex_alpha;

void main() {
    if (height < water_height && is_reflected != 0){
        discard;
    }
    else {
        // normal
        //vec3 normal_mv = normalize(cross(dFdx(vpoint_mv.xyz), dFdy(vpoint_mv.xyz)));
        //vec3 ambient = ka*La;

        // compute diffuse term.
        //texture(colormap, max(dot(normal_mv, light_dir), 0.0f)).x;
        float nl = max(dot(N_mv, light_dir.xyz), 0.0f);
        vec3 diffuse = kd*Ld*nl;

        // color = vec4(diffuse, 1);//+ ambient;
        // color = vec4(mix(texture(colormap, height * 2.5  + 0.5).rgb, diffuse + ka * La, 0.25), 1);

        float alphaSnow, alphaSand, alphaRock, alphaGrass;
        float snowThresh = 1.3f;
        float snowRockGrass = 0.60;
        float rockGrassSand = 0.15;
        float sandThresh = 0.03;

        // TODO blend according to slope, grass on flat surfaces, rock sand on steep surface
        // or other...
        // TODO play with the thresholds or and add mixtures between 3 textures
        // maybe too much work in fragment shader....
        if (height > snowThresh) {
            alphaSnow = 1;
            alphaRock = 0;
            alphaGrass = 0;
            alphaSand = 0;
        } else if (height > snowRockGrass) {
            alphaSnow = 1-((snowThresh-height)/(snowThresh-snowRockGrass));
            alphaRock = 1-alphaSnow;
            alphaGrass = 0;
            alphaSand = 0;
        } else if (height > rockGrassSand) {
            alphaSnow = 0;
            alphaRock = 1-((snowRockGrass-height)/(snowRockGrass-rockGrassSand));
            alphaGrass = 1-alphaRock;
            alphaSand = 0;
        } else if (height > sandThresh) {
            alphaSnow = 0;
            alphaRock = 0;
            alphaGrass = 1-((rockGrassSand-height)/(rockGrassSand-sandThresh));
            alphaSand = 1-alphaGrass;
        } else {
            alphaSnow = 0;
            alphaRock = 0;
            alphaGrass = 0;
            alphaSand = 1;
        }

        color = (alphaGrass * vec4(texture(tex_grass, 32.f * uv + 32.f * vec2(offset_x, offset_y)).rgb, 1) +
                alphaRock * vec4(texture(tex_rock,    32.f * uv + 32.f * vec2(offset_x, offset_y)).rgb, 1) +
                alphaSnow * vec4(texture(tex_snow,    32.f * uv + 32.f * vec2(offset_x, offset_y)).rgb, 1) +
                alphaSand * vec4(texture(tex_sand,    32.f * uv + 32.f * vec2(offset_x, offset_y)).rgb, 1));
        vec4 light = vec4(ka * La + diffuse,1);
        color = color * light;
        color.a = 1.f - texture(tex_alpha, uv).r;



    }
}
