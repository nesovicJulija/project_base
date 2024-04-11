#version 330 core

layout (location = 0) in vec3 aPos;

out vec3 TexCoords;

// transformacija koja nam treba je kroz view i projection matricu zbog kamere koja gleda u kocku

uniform mat4 projection;
uniform mat4 view;

void main(){

    TexCoords = aPos; // kada se bude radila fragment interpolacija(jedna strana kocke == dva trougla)
    // pa ne moramo sami da racunamo direkcioni vektor
    gl_Position = projection * view * vec4(aPos, 1.0);
    // ne moramo da radimo i model jer cemo ga wrapovati oko cele scene i obuhvatace sve sto se nalazi
    // na sceni
}