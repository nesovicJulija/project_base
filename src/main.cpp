#include <iostream>
#include<glad/glad.h> /* glad obavezno ukljuciti pre glfw!!! */
#include<GLFW/glfw3.h>
#include<cmath>
#include <learnopengl/shader.h>
#include<stb_image.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

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
float lastX =  SCR_WIDTH / 2.0; // na pocetku je kursor na sredini ekrana
float lastY =  SCR_HEIGHT / 2.0; // na pocetku je kursor na sredini ekrana
//float fov   =  45.0f;

float deltaTime = 0.0f;	// time between current frame and last frame
float lastFrame = 0.0f;

//Camera camera(glm::vec3(0.0f, 0.0f, 40.0f));

struct PointLight {
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

struct ProgramState {
    glm::vec3 clearColor = glm::vec3(0);
    bool ImGuiEnabled = false;
    Camera camera;
    bool CameraMouseMovementUpdateEnabled = true;
    glm::vec3 modelPosition = glm::vec3(0.0f);
    float modelScale = 1.0f;
    PointLight pointLight;
    ProgramState()
            : camera(glm::vec3(20.0f, 20.0f, -40.0f)) {}

    void SaveToFile(std::string filename);

    void LoadFromFile(std::string filename);
};

void ProgramState::SaveToFile(std::string filename) {
    std::ofstream out(filename);
    out << clearColor.r << '\n'
        << clearColor.g << '\n'
        << clearColor.b << '\n'
        << ImGuiEnabled << '\n'
        << camera.Position.x << '\n'
        << camera.Position.y << '\n'
        << camera.Position.z << '\n'
        << camera.Front.x << '\n'
        << camera.Front.y << '\n'
        << camera.Front.z << '\n';
}

void ProgramState::LoadFromFile(std::string filename) {
    std::ifstream in(filename);
    if (in) {
        in >> clearColor.r
           >> clearColor.g
           >> clearColor.b
           >> ImGuiEnabled
           >> camera.Position.x
           >> camera.Position.y
           >> camera.Position.z
           >> camera.Front.x
           >> camera.Front.y
           >> camera.Front.z;
    }
}

ProgramState *programState;
void DrawImGui(ProgramState *programState);
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

int main() {

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
    glfwSetKeyCallback(window, key_callback);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad biblioteka ucitava sve svoje fje
    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)){
        std::cout << "Failed to init GLAD\n";
        glfwTerminate();
        return EXIT_FAILURE;
    }

    programState = new ProgramState;
    programState->LoadFromFile("resources/program_state.txt");
    if (programState->ImGuiEnabled) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    // Init Imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    glEnable(GL_DEPTH_TEST); //moramo ovo da uradimo jer objekti koji su na sceni treba uvek da budu ispred skybox-a!

    glEnable(GL_CULL_FACE); // enejblujemo odsecanje stranica
    glEnable(GL_BACK); // necemo renderovati zadnje stranice

    // zbog blendinga
    glEnable(GL_BLEND);
    // nakon toga postavljamo blending fju
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // shader() -> konstruktor
    Shader shader("resources/shaders/helicopter.vs", "resources/shaders/helicopter.fs");
    Shader shader1("resources/shaders/helipad.vs", "resources/shaders/helipad.fs");
    Shader skyboxShader("resources/shaders/skybox.vs", "resources/shaders/skybox.fs");
    Shader shaderCrow("resources/shaders/birds.vs", "resources/shaders/birds.fs");
    Shader blendingShader("resources/shaders/clouds.vs", "resources/shaders/clouds.fs");
    Shader ourShader("resources/shaders/model_lighting.vs", "resources/shaders/model_lighting.fs");

    Model ourModel("resources/objects/backpack/Bell206.obj");
    ourModel.SetShaderTextureNamePrefix("material.");
    Model ourModel1("resources/objects/backpack/E6E4LR2BDR7I3VZPO4A4WJLKX.obj");
    ourModel1.SetShaderTextureNamePrefix("material.");
    Model ourModel2("resources/objects/backpack/AmericanCrow.obj");
    ourModel2.SetShaderTextureNamePrefix("material.");

    PointLight& pointLight = programState->pointLight;
    pointLight.position = glm::vec3(4.0f, 4.0, 0.0);
    pointLight.ambient = glm::vec3(0.1, 0.1, 0.1);
    pointLight.diffuse = glm::vec3(0.6, 0.6, 0.6);
    pointLight.specular = glm::vec3(1.0, 1.0, 1.0);

    pointLight.constant = 1.0f;
    pointLight.linear = 0.09f;
    pointLight.quadratic = 0.032f;

    float cubeVertices[] = {
            // positions          // texture Coords
            -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
            0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
            0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
            0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
            -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
            -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

            -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
            0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
            0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
            0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
            -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
            -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

            -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
            -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
            -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
            -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
            -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
            -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

            0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
            0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
            0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
            0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
            0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
            0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

            -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
            0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
            0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
            0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
            -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
            -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

            -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
            0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
            0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
            0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
            -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
            -0.5f,  0.5f, -0.5f,  0.0f, 1.0f
    };

    float transparentVertices[] = {
            // positions         // texture Coords (swapped y coordinates because texture is flipped upside down)
            0.0f,  0.5f,  0.0f,  0.0f,  0.0f,
            0.0f, -0.5f,  0.0f,  0.0f,  1.0f,
            1.0f, -0.5f,  0.0f,  1.0f,  1.0f,

            0.0f,  0.5f,  0.0f,  0.0f,  0.0f,
            1.0f, -0.5f,  0.0f,  1.0f,  1.0f,
            1.0f,  0.5f,  0.0f,  1.0f,  0.0f
    };

    // cube VAO
    unsigned int cubeVAO, cubeVBO;
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);
    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), &cubeVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    // transparent VAO
    unsigned int transparentVAO, transparentVBO;
    glGenVertexArrays(1, &transparentVAO);
    glGenBuffers(1, &transparentVBO);
    glBindVertexArray(transparentVAO);
    glBindBuffer(GL_ARRAY_BUFFER, transparentVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(transparentVertices), transparentVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glBindVertexArray(0);

    // load textures
    // -------------
    unsigned int cubeTexture = loadTexture(FileSystem::getPath("resources/textures/marble.jpg").c_str());
    unsigned int transparentTexture = loadTexture(FileSystem::getPath("resources/textures/Cloud.png").c_str());

    float skyboxVertices[] = { // vrednosti su uvek [-1,1] i ovako definisana kocka obuhvata ceo clip space
            // positions
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            -1.0f,  1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f,  1.0f
    };

    // skybox VAO
    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    // ovaj niz od 6 tekstura su teksture za strane kocke(moraju bas u ovom redosledu da budu navedene!)
    vector<std::string> faces
            {
                    FileSystem::getPath("resources/textures/right.jpg"),
                    FileSystem::getPath("resources/textures/left.jpg"),
                    FileSystem::getPath("resources/textures/top.jpg"),
                    FileSystem::getPath("resources/textures/bottom.jpg"),
                    FileSystem::getPath("resources/textures/front.jpg"),
                    FileSystem::getPath("resources/textures/back.jpg")
            };

    vector<glm::vec3> clouds
            {
                    //glm::vec3(-1.5f, 0.0f, -0.48f),
                    //glm::vec3( 1.5f, 0.0f, 0.51f),
                    //glm::vec3( 0.0f, 0.0f, 0.7f),
                    //glm::vec3(-0.3f, 0.0f, -2.3f),
                    glm::vec3 (0.5f, 0.0f, -0.6f)
            };

    blendingShader.use();
    blendingShader.setInt("texture1", 0);

    glm::vec3 lightPos(0, 0, 0);

    unsigned int cubemapTexture = loadCubemap(faces);

    skyboxShader.use();
    skyboxShader.setInt("skybox", 0);

    float lin = 0.14f;
    float kvad = 0.07f;

    while(!glfwWindowShouldClose(window)){

        /*kada frame rate nije zakljucan moramo raditi koliko je vremena proteklo
         * od kada se petlja renderovanja zavrsila do trenutnog vremena*/
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        proccessInput(window);

        // moramo da sortiramo pozicije transparentnih objekata koje hocemo da nacrtamo

        std::map<float, glm::vec3> sorted;
        for (unsigned int i = 0; i < clouds.size(); i++)
        {
            float distance = glm::length(programState->camera.Position - clouds[i]);
            sorted[distance] = clouds[i];
        }

        glClearColor(programState->clearColor.r, programState->clearColor.g, programState->clearColor.b, 1.0f);
       // glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // postoji i bit za dubinu

        /*OVO JE ZA SKYBOX!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/

        glm::mat4 view = programState->camera.GetViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);

        glDepthMask(GL_FALSE);
        skyboxShader.use();
        // zelimo da iz matrice pogleda(view) eliminisemo translaciju, kako bi samo rotacija uticala
        // na prikaz skyboxa(da kocka uvek izgleda beskonacno daleko)
        skyboxShader.setMat4("view", glm::mat4(glm::mat3 (view)));
        skyboxShader.setMat4("projection", projection);
        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glDepthMask(GL_TRUE); // ponovo ukljucujemo testiranje dubine(jer skybox treba da bude iza svega)

        /* OVO VRATI ZA MODEL (ODAVDE)!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
        ourShader.use();

        //Directional Lignt
        ourShader.setVec3("dirLight.direction", -20.0f, -20.0f, 0.0f);
        ourShader.setVec3("dirLight.ambient", 0.56, 0.56, 0.56);
        ourShader.setVec3("dirLight.diffuse",  0.97f,0.95f,0.96);
        ourShader.setVec3("dirLight.specular", 0.5, 0.5, 0.5);

        // Pointlight's
        //1
        ourShader.setVec3("pointLight[0].position", glm::vec3(-1.05f,2.4f,1.7f));
        ourShader.setVec3("pointLight[0].ambient", glm::vec3(0.8, 0.85, 0.45));
        ourShader.setVec3("pointLight[0].diffuse", glm::vec3(0.5f,0.5f,0.1f));
        ourShader.setVec3("pointLight[0].specular", glm::vec3(0.6, 0.6, 0.6));
        ourShader.setFloat("pointLight[0].constant", 1.0f);
        ourShader.setFloat("pointLight[0].linear", lin);
        ourShader.setFloat("pointLight[0].quadratic", kvad);
        //2
        ourShader.setVec3("pointLight[1].position", glm::vec3(-1.70f,(2.4f + sin(glfwGetTime())/6),-11.1f));
        ourShader.setVec3("pointLight[1].ambient", glm::vec3(0.45, 0.65, 0.75));
        ourShader.setVec3("pointLight[1].diffuse", glm::vec3(0.25f,0.25f,0.25f));
        ourShader.setVec3("pointLight[1].specular", glm::vec3(0.6, 0.6, 0.6));
        ourShader.setFloat("pointLight[1].constant", 1.0f);
        ourShader.setFloat("pointLight[1].linear", lin);
        ourShader.setFloat("pointLight[1].quadratic", kvad);
        //3
        ourShader.setVec3("pointLight[2].position", glm::vec3(-5.75f,(4.85f + sin(glfwGetTime())/6),8.95f));
        ourShader.setVec3("pointLight[2].ambient", glm::vec3(0.23, 0.23, 0.24));
        ourShader.setVec3("pointLight[2].diffuse", glm::vec3(0.25f,0.25f,0.25f));
        ourShader.setVec3("pointLight[2].specular", glm::vec3(0.6, 0.6, 0.6));
        ourShader.setFloat("pointLight[2].constant", 1.0f);
        ourShader.setFloat("pointLight[2].linear", lin);
        ourShader.setFloat("pointLight[2].quadratic", kvad);
        //4
        ourShader.setVec3("pointLight[3].position", glm::vec3(7.7f,(-0.4f + sin(glfwGetTime())/6),8.75f));
        ourShader.setVec3("pointLight[3].ambient", glm::vec3(0.15, 0.15, 0.15));
        ourShader.setVec3("pointLight[3].diffuse", glm::vec3(0.25f,0.25f,0.25f));
        ourShader.setVec3("pointLight[3].specular", glm::vec3(0.6, 0.6, 0.6));
        ourShader.setFloat("pointLight[3].constant", 1.0f);
        ourShader.setFloat("pointLight[3].linear", lin);
        ourShader.setFloat("pointLight[3].quadratic", kvad);

        ourShader.setVec3("viewPosition", programState->camera.Position);
        ourShader.setFloat("material.shininess", 64.0f);
        // view/projection transformations
        projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                                (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        view = programState->camera.GetViewMatrix();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);
        ourShader.setMat4("view", view);

        float time = glfwGetTime();

        glm::mat4 model = glm::mat4(1.0f);
        //model = glm::translate(model, glm::vec3(-1.5f, -25.35f, 7.0f)); // ovako stoji tacno na zemlji

        model = glm::translate(model, glm::vec3(-1.5f, -24.0f, 7.0f));
        model = glm::translate(model, glm::vec3(0,sin((glfwGetTime()*2) / 2.0+0.5),0));
        model = glm::scale(model, glm::vec3(1, 1, 1));
        ourShader.setMat4("model", model);

        // hocemo da nacrtamo model sa sejderom shader
        ourModel.Draw(ourShader);

        //drugi model
        //shader1.use();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);

        model = glm::mat4(1);
        model = glm::translate(model, glm::vec3(5.0f, -20.0f, 0.0f));
        model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0, 0.0,10.0));
        model = glm::scale(model, glm::vec3(55.0f, 55.0f, 55.0f));
        ourShader.setMat4("model", model);

        ourModel1.Draw(ourShader);

        // treci model
        //shaderCrow.use();
        projection = glm::perspective(glm::radians(programState->camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        ourShader.setMat4("projection", projection);

        // mozemo po jednacini kruga da pomeramo
        float radius = 40; // koliko zelimo da budemo udaljeni od koord pocetka
        float camX = sin(glfwGetTime()) * radius;
        float camZ = cos(glfwGetTime()) * radius;

        view = glm::lookAt(glm::vec3(2,2,-5), glm::vec3(camX,0,camZ), glm::vec3(4,4,4)); // vektor na gore je bas y!

        //view = camera.GetViewMatrix();
        ourShader.setMat4("view", view);

        for(int i = 0; i < 3; i++) {

            float angle = 1.0+i;
            glm::mat4 model = glm::mat4(1.0f);
            // hocemo svaku kocku da zarotiramo i treba da transliramo u poziciju zaq kocku
            model = glm::translate(model, glm::vec3(0.0, 0.0, -2.0 * 2*i));
            model = glm::translate(model, glm::vec3(5 * cos(time), -2.0 * i, 5 * sin(time)));
            //float angle = 20.0f * (i + 5);
            model = glm::rotate(model, glm::radians(angle), glm::vec3(10.0, 20.0, 8.0));
            model = glm::scale(model, glm::vec3(2.5f, 2.5f, 2.5f));
            // moramo da postavimo model matricu!!!
            ourShader.setMat4("model", model);
            ourModel2.Draw(ourShader);
        }

        blendingShader.use();
        projection = glm::perspective(glm::radians(programState->camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        view = programState->camera.GetViewMatrix();
        model = glm::mat4(1.0f);

        blendingShader.setMat4("projection", projection);
        blendingShader.setMat4("view", view);
        // cubes
        glBindVertexArray(cubeVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, cubeTexture);
        model = glm::translate(model, glm::vec3(2.0f, 2.0f, 2.0f));

        blendingShader.setMat4("model", model);

        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(2.0f, 0.0f, 0.0f));
        blendingShader.setMat4("model", model);

        // clouds
        glBindVertexArray(transparentVAO);
        glBindTexture(GL_TEXTURE_2D, transparentTexture);
        for (std::map<float, glm::vec3>::reverse_iterator it = sorted.rbegin(); it != sorted.rend(); ++it)
        {
            model = glm::mat4(1.0f);
            model = glm::translate(model, it->second);
            model = glm::translate(model, glm::vec3(0,4,0));
            model = glm::scale(model, glm::vec3(-9,7,4));
            blendingShader.setMat4("model", model);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
        // --------------------------------------------------------------
        // flags
        blendingShader.setMat4("projection", projection);
        blendingShader.setMat4("view", view);
        // cubes
        glBindVertexArray(cubeVAO);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, cubeTexture);
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));

        blendingShader.setMat4("model", model);

        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(2.0f, 0.0f, 0.0f));
        blendingShader.setMat4("model", model);

        // clouds
        glBindVertexArray(transparentVAO);
        glBindTexture(GL_TEXTURE_2D, transparentTexture);
        for (std::map<float, glm::vec3>::reverse_iterator it = sorted.rbegin(); it != sorted.rend(); ++it)
        {
            model = glm::mat4(1.0f);
            model = glm::translate(model, it->second);
            model = glm::translate(model, glm::vec3(12,7,2));
            model = glm::scale(model, glm::vec3(-9,7,4));
            blendingShader.setMat4("model", model);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        //update(window);

        if (programState->ImGuiEnabled)
            DrawImGui(programState);

        glfwSwapBuffers(window); //render();
        glfwPollEvents();
    }

    programState->SaveToFile("resources/program_state.txt");
    delete programState;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();
    return 0;
}

  void proccessInput(GLFWwindow *window){
      if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
          glfwSetWindowShouldClose(window, true);

      if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
          programState->camera.ProcessKeyboard(FORWARD, deltaTime);
      if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
          programState->camera.ProcessKeyboard(BACKWARD, deltaTime);
      if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
          programState->camera.ProcessKeyboard(LEFT, deltaTime);
      if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
          programState->camera.ProcessKeyboard(RIGHT, deltaTime);
  }

void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    if (programState->CameraMouseMovementUpdateEnabled)
        programState->camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    programState->camera.ProcessMouseScroll(yoffset);
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

void DrawImGui(ProgramState *programState) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    {
        static float f = 0.0f;
        ImGui::Begin("Hello window");
        ImGui::Text("Hello text");
        ImGui::SliderFloat("Float slider", &f, 0.0, 1.0);
        ImGui::ColorEdit3("Background color", (float *) &programState->clearColor);
        ImGui::DragFloat3("Model position", (float*)&programState->modelPosition);
        ImGui::DragFloat("Model scale", &programState->modelScale, 0.05, 0.1, 4.0);

        ImGui::DragFloat("pointLight.constant", &programState->pointLight.constant, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight.linear", &programState->pointLight.linear, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight.quadratic", &programState->pointLight.quadratic, 0.05, 0.0, 1.0);
        ImGui::End();
    }

    {
        ImGui::Begin("Camera info");
        const Camera& c = programState->camera;
        ImGui::Text("Camera position: (%f, %f, %f)", c.Position.x, c.Position.y, c.Position.z);
        ImGui::Text("(Yaw, Pitch): (%f, %f)", c.Yaw, c.Pitch);
        ImGui::Text("Camera front: (%f, %f, %f)", c.Front.x, c.Front.y, c.Front.z);
        ImGui::Checkbox("Camera mouse update", &programState->CameraMouseMovementUpdateEnabled);
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
        programState->ImGuiEnabled = !programState->ImGuiEnabled;
        if (programState->ImGuiEnabled) {
            programState->CameraMouseMovementUpdateEnabled = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }
}

void update(GLFWwindow *window){

}

void frameBufferSizeCallback(GLFWwindow *window, int width, int height){
    // dimenzije unutar prozora za renderovanje, posto se velicina prozora moze menjati necemo ga pozvati
    // iz main-a
    //glViewport(0,0,800,600);
    glViewport(0,0,width, height);
}
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
