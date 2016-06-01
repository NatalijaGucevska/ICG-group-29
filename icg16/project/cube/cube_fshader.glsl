#version 330

in vec3 texcoords;
uniform samplerCube tex;
out vec4 color;

void main () {
    float y = texcoords.y / 256.f;
    if(y > 0.75) {
        color = texture(tex, texcoords);
    }
    else {
        color = vec4(mix(texture(tex, texcoords).rgb, vec3(1.f), 1.f - texcoords.y / 192.f), 1.f);
    }
}
