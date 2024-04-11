#include <iostream>
#include<glad/glad.h> /* glad obavezno ukljuciti pre glfw!!! */
#include<GLFW/glfw3.h>
#include<cmath>
#include <learnopengl/shader.h>
#include<stb_image.h>

//#include<rg/Shader.h>
// OpenGL-ove biblioteke za matematiku
#include <glm/glm.hpp>
#include<glm/gtc/matrix_transform.hpp>
#include<glm/gtc/type_ptr.hpp>
#include<learnopengl/camera.h>
#include <learnopengl/filesystem.h>
#include<learnopengl/model.h>
#include<learnopengl/mesh.h>
#include<rg/Error.h>

void frameBufferSizeCallback(GLFWwindow *window, int width, int height);
void update(GLFWwindow *window);
void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);
void proccessInput(GLFWwindow *window);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
unsigned int loadTexture(char const *path);
unsigned int loadCubemap(std::vector<std::string> faces);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

bool firstMouse = true;
//float yaw   = -90.0f;	// yaw is initialized to -90.0 degrees since a yaw of 0.0 results in a direction vector pointing to the right so we initially rotate a bit to the left.
//float pitch =  0.0f;
float lastX =  SCR_WIDTH / 2.0; // na pocetku je kursor na sredini ekrana
float lastY =  SCR_HEIGHT / 2.0; // na pocetku je kursor na sredini ekrana
//float fov   =  45.0f;

float deltaTime = 0.0f;	// time between current frame and last frame
float lastFrame = 0.0f;

glm::vec3 lightPos(1.2f, 1.0f, 2.0f);

Camera camera(glm::vec3(0.0f, 0.0f, 40.0f));

int main() {
    //std::cout << "Hello, World!" << std::endl;
    /*pravimo nov prozor i registrujemo dogadjaje*/
    // inicijalizacija glfw
    glfwInit();
    // OpenGL Core 3.3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // kreiramo prozora
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "project", nullptr, nullptr);
    if(window == nullptr){
        // nije uspesno kreiran
        std::cout << "Failed to create window!\n";
        glfwTerminate();
        return EXIT_FAILURE;
    }
    // opengl-u zelimo da kazemo da crta u prozoru
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, frameBufferSizeCallback); // sta se desi kada resize-ujemo prozor
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad biblioteka ucitava sve svoje fje
    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)){
        std::cout << "Failed to init GLAD\n";
        glfwTerminate();
        return EXIT_FAILURE;
    }

    // treba ovde da testiramo dubinu
    glEnable(GL_DEPTH_TEST);

    // shader() -> konstruktor
    Shader shader("resources/shaders/helicopter.vs", "resources/shaders/helicopter.fs");

    Model ourModel("resources/objects/backpack/Bell206.obj");

    while(!glfwWindowShouldClose(window)){

        /*kada frame rate nije zakljucan moramo raditi koliko je vremena proteklo
         * od kada se petlja renderovanja zavrsila do trenutnog vremena*/
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        proccessInput(window);

        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // postoji i bit za dubinu

        shader.use();

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        shader.setMat4("projection", projection);

        //glm::mat4 view = glm::mat4(1.0f);
        /* cameraFront nije smer gledanja vec je tacka u prostoru(moramo i nju da pomerimo) */
        //view = glm::lookAt(cameraPos, cameraFront + cameraPos, cameraUp);
        glm::mat4 view = camera.GetViewMatrix(); // za trenutno stanje kamere vraca view matricu
        shader.setMat4("view", view);

        float time = glfwGetTime();

        glm::mat4 model = glm::mat4(1.0f);
        //model = glm::translate(model, glm::vec3(-1.5f, -25.35f, 7.0f)); // ovako stoji tacno na zemlji

        model = glm::translate(model, glm::vec3(-1.5f, -24.0f, 7.0f));
        model = glm::translate(model, glm::vec3(0,sin((glfwGetTime()*2) / 2.0+0.5),0));
        model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));
        shader.setMat4("model", model);

        // hocemo da nacrtamo model sa sejderom shader
        ourModel.Draw(shader);

        //update(window);
        glfwSwapBuffers(window); //render();
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

void proccessInput(GLFWwindow *window){
    // obradjuje ulaz
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS){
        glfwSetWindowShouldClose(window, true);
    }

    // brzinu skaliramo sa deltaTime da bi bilo ravnomernije
    const float cameraSpeed = 2.5*deltaTime; // brzina kojom se kamera pomera
    // definisemo promene ulaza td se pomeramo na w,a,s,d
    if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS){
        camera.ProcessKeyboard(FORWARD, deltaTime);
    }

    if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS){
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    }

    if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS){
        /* pomeramo se u desno pa uzimamo vektorski proizvod
         * uvek treba da normalizujemo vektore*/
        camera.ProcessKeyboard(RIGHT, deltaTime);
    }

    if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS){
        camera.ProcessKeyboard(LEFT, deltaTime);
    }
}

void mouse_callback(GLFWwindow *window, double xpos, double ypos){
    /*moramo ponovo da izracunamo vektore x i y*/
    if(firstMouse){
        lastX = xpos;
        lastY = ypos;
        firstMouse = false; // mis je pomeren
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; /*obrnuto je jer nama koordinate po ekranu idu drugacije*/

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset){

    /* kada gledamo za koliko je skrolovano posmatramo yoffset */
    camera.ProcessMouseScroll(yoffset);
}

void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods){
    // da se prozor zatvori pritiskom na escape
    if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS){
        glfwSetWindowShouldClose(window, true);
    }
    // R na crveno, G na zelenu, B na plavu
    if(key == GLFW_KEY_R && action == GLFW_PRESS){
        glClearColor(1.0, 0.0, 0.0, 1.0);
    }

    if(key == GLFW_KEY_G && action == GLFW_PRESS){
        glClearColor(0.0, 1.0, 0.0, 1.0);
    }

    if(key == GLFW_KEY_B && action == GLFW_PRESS){
        glClearColor(0.0, 0.0, 1.0, 1.0);
    }

}

unsigned int loadTexture(char const *path){
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);

    if(data){
        GLenum format;

        if(nrComponents == 1)
            format = GL_RED;

        else if(nrComponents == 3)
            format = GL_RGB;

        else if(nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }

    else{
        std::cout << "Texture failed to load path : " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

void update(GLFWwindow *window){

}

void frameBufferSizeCallback(GLFWwindow *window, int width, int height){
    // dimenzije unutar prozora za renderovanje, posto se velicina prozora moze menjati necemo ga pozvati
    // iz main-a
    //glViewport(0,0,800,600);
    glViewport(0,0,width, height);
}
// loads a cubemap texture from 6 individual texture faces
// order:
// +X (right)
// -X (left)
// +Y (top)
// -Y (bottom)
// +Z (front)
// -Z (back)
// -------------------------------------------------------
unsigned int loadCubemap(std::vector<std::string> faces){
    // fja za ucitavanje Cubemape
    /*Cubemap je tekstura objekat kao i svaki drugi i pravi se na analogan nacin*/
    unsigned int textureID;
    glGenTextures(1, &textureID);
    // ne aktiviramo ga kao 2D teksturu
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    unsigned char* data;

    for(int i = 0; i < faces.size(); i++){
        // za svaku pozivamo stbi load
        data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);

        if(data){
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0,
                         GL_RGB, GL_UNSIGNED_BYTE, data);
        }else{
            ASSERT(false, "Failed to load cube map texture face");
            return -1;
        }
        stbi_image_free(data);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE); // r odgovara z koordinati
    // stavljamo clamp to edge da bi OpenGL vratio granice teksture

    return textureID;

}
