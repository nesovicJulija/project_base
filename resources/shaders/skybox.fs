#version 330 core

out vec4 FragColor;
in vec3 TexCoords;

uniform samplerCube skybox;
// samplerCube ugradjen tip teksture

void main(){

    // skybox je kocka koju hocemo da semplujemo u direkcionom vektoru(racunamo ga u vertex shaderu
    // i fragment interpolacija radi ostalo)
    FragColor = texture(skybox, TexCoords);

}