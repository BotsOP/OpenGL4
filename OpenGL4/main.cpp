
#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "Shader.h"
#include "Model.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "Utils.h"


//forward declarations
void renderModel(glm::mat4 view, glm::mat4 projection, glm::vec3 position, Model model, Shader shader);
void renderTerrain(glm::mat4 view, glm::mat4 projection);
void renderSkyBox(glm::mat4 view, glm::mat4 projection);
void renderWater(glm::mat4 view, glm::mat4 projection, float t, int depth_renderbuffer);
void handleInput(GLFWwindow* window, float deltaTime);
void setupSkyBox();

glm::vec3 cameraPosition(0, 100, 0), cameraForward(0, 0, 1), cameraUp(0, 1, 0), cameraSideward(1, 0, 0);
float movementSpeed = 1.0f;

unsigned int plane, planeSize, VAO, cubeSize, water;
unsigned int terrainShaderID, skyboxShaderID, waterShaderID, standardShaderID, postProcessingShaderID;
unsigned int heightmapID, heightNormalID, heightmapID2, waterHeightmap;
unsigned int dirtID, sandID, grassID, bayerID, ditherRampID;

int main()
{
    std::cout << "Hello World23!\n";

    static double previousT = 0;

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    int width = 1600;
    int height = 1200;
    GLFWwindow* window = glfwCreateWindow(width, height, "Hello OpenGL!", nullptr, nullptr);
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glViewport(0, 0, width, height);

    setupSkyBox();

    plane = GeneratePlane("Assets/Textures/displacement.png", GL_RGBA, 4, 1.0f, 1.0f, planeSize, heightmapID);
    water = GeneratePlane("Assets/Textures/displacement.png", GL_RGBA, 4, 1.0f, 1.0f, planeSize, heightmapID);

    //Models
    Model tree("Assets/lowpolytree.obj");

    //Textures
    heightmapID2 = loadTexture("Assets/Textures/displacement.png", GL_RGBA, 4);
    waterHeightmap = loadTexture("Assets/Textures/waterDisplacement.png", GL_RGBA, 4);

    heightNormalID = loadTexture("Assets/Textures/NormalMap.png", GL_RGBA);
    dirtID = loadTexture("Assets/Textures/dirt.jpg", GL_RGB);
    sandID = loadTexture("Assets/Textures/sand.jpg", GL_RGB);
    grassID = loadTexture("Assets/Textures/grass.png", GL_RGBA, 4);

    bayerID = loadTexture("Assets/Textures/BayerMatrix.png", GL_RGB);
    ditherRampID = loadTexture("Assets/Textures/ditherRamp.png", GL_RGB);


    ///SETUP SHADER PROGRAM///
    //Shader displacementShader("displacementFragShader.shader", "displacementVertexShader.shader");

    Shader terrainShader("terrainVertex.shader", "terrainFragment.shader");
    terrainShaderID = terrainShader.ID;
    
    Shader skyShader("skyboxVertex.shader", "skyboxFragment.shader");
    skyboxShaderID = skyShader.ID;

    Shader waterShader("waterVertex.shader", "waterFragment.shader");
    waterShaderID = waterShader.ID;
    waterShader.setInt("firstDepthPass", 0);

    Shader standardShader("standardVertex.shader", "standardFragment.shader");
    standardShaderID = standardShader.ID;

    Shader postProcessingShader("postProcessingVertex.shader", "postProcessingFragment.shader");
    postProcessingShaderID = postProcessingShader.ID;

    terrainShader.use();
    glUniform1i(glGetUniformLocation(terrainShader.ID, "heightMap"), 0);
    glUniform1i(glGetUniformLocation(terrainShader.ID, "normalMap"), 1);
    glUniform1i(glGetUniformLocation(terrainShader.ID, "dirt"), 2);
    glUniform1i(glGetUniformLocation(terrainShader.ID, "sand"), 3);
    glUniform1i(glGetUniformLocation(terrainShader.ID, "grass"), 4);

    postProcessingShader.use();
    glUniform1i(glGetUniformLocation(postProcessingShader.ID, "screenTexture"), 0);
    glUniform1i(glGetUniformLocation(postProcessingShader.ID, "noiseTexture"), 1);
    glUniform1i(glGetUniformLocation(postProcessingShader.ID, "ditherRampTexture"), 2);


    stbi_set_flip_vertically_on_load(true);

    // GENERATE QUAD
    float quadVertices[] = { // vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
        // positions   // texCoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };

    unsigned int quadVAO, quadVBO;
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));


    // GENERATE FRAMEBUFFER

    unsigned int framebuffer;
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    GLuint depth_renderbuffer;
    glGenRenderbuffers(1, &depth_renderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, (GLuint)depth_renderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_renderbuffer);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);



    unsigned int postProcessingBuffer;
    glGenFramebuffers(1, &postProcessingBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, postProcessingBuffer);
    // create a color attachment texture
    unsigned int textureColorbuffer;
    glGenTextures(1, &textureColorbuffer);
    glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColorbuffer, 0);
    // create a renderbuffer object for depth and stencil attachment (we won't be sampling these)
    unsigned int rbo;
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height); // use a single renderbuffer object for both a depth AND stencil buffer.
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo); // now actually attach it
    // now that we actually created the framebuffer and added all attachments we want to check if it is actually complete now
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    std::cout << glGetError() << endl;
    
    // OPENGL SETTINGS //

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    while (!glfwWindowShouldClose(window)) {
        terrainShader.use();

        float t = glfwGetTime();
        float deltaTime = t - previousT;
        previousT = t;

        handleInput(window, deltaTime);

        glm::mat4 view = glm::lookAt(cameraPosition, cameraPosition + cameraForward, cameraUp);
        glm::mat4 projection = glm::perspective(glm::radians(90.0f), width / (float)height, 0.1f, 1000.0f);

        // FIRST RENDER

        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        renderTerrain(view, projection);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth_renderbuffer, 0);

        // SECOND RENDER

        glBindFramebuffer(GL_FRAMEBUFFER, postProcessingBuffer);
        glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        renderSkyBox(view, projection);
        renderTerrain(view, projection);
        renderModel(view, projection, glm::vec3(0, 50, 0), tree, standardShader);


        renderWater(view, projection, t, depth_renderbuffer);
        

        glBindFramebuffer(GL_FRAMEBUFFER, 0); // back to default
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        postProcessingShader.use();
        glBindVertexArray(quadVAO);
        glDisable(GL_DEPTH_TEST);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, bayerID);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, ditherRampID);

        glDrawArrays(GL_TRIANGLES, 0, 6);

        glfwSwapBuffers(window);
        glfwPollEvents();     
    }

    glfwTerminate();
    return 0;
}

void renderModel(glm::mat4 view, glm::mat4 projection, glm::vec3 position, Model model, Shader shader)
{
    glUseProgram(standardShaderID);
    glCullFace(GL_BACK);
    glEnable(GL_DEPTH_TEST);

    glm::mat4 world = glm::mat4(1.f);
    world = glm::translate(world, position);
    world = glm::scale(world, glm::vec3(1, 1, 1));
    world = glm::translate(world, glm::vec3(0, 0, 1));

    glUniformMatrix4fv(glGetUniformLocation(standardShaderID, "world"), 1, GL_FALSE, glm::value_ptr(world));
    glUniformMatrix4fv(glGetUniformLocation(standardShaderID, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(standardShaderID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniform3fv(glGetUniformLocation(standardShaderID, "cameraPosition"), 1, glm::value_ptr(cameraPosition));

    model.Draw(shader);
}

void renderSkyBox(glm::mat4 view, glm::mat4 projection)
{
    // SKY BOX
    glUseProgram(skyboxShaderID);
    glCullFace(GL_FRONT);
    glDisable(GL_DEPTH_TEST);

    glm::mat4 world = glm::mat4(1.f);
    world = glm::translate(world, cameraPosition);

    glUniformMatrix4fv(glGetUniformLocation(skyboxShaderID, "world"), 1, GL_FALSE, glm::value_ptr(world));
    glUniformMatrix4fv(glGetUniformLocation(skyboxShaderID, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(skyboxShaderID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniform3fv(glGetUniformLocation(skyboxShaderID, "cameraPosition"), 1, glm::value_ptr(cameraPosition));

    float t = glfwGetTime();

    glUniform3f(glGetUniformLocation(skyboxShaderID, "lightDirection"), glm::cos(t), -0.5f, glm::sin(t));

    glBindVertexArray(VAO);
    //glDrawArrays(GL_TRIANGLES, 0, 6);
    glDrawElements(GL_TRIANGLES, cubeSize, GL_UNSIGNED_INT, 0);
}

void renderTerrain(glm::mat4 view, glm::mat4 projection)
{
    glUseProgram(terrainShaderID);
    glCullFace(GL_BACK);
    glEnable(GL_DEPTH_TEST);

    float t = glfwGetTime();

    glm::mat4 world = glm::mat4(1.f);
    world = glm::translate(world, glm::vec3(0, 0, 0));

    glUniformMatrix4fv(glGetUniformLocation(terrainShaderID, "world"), 1, GL_FALSE, glm::value_ptr(world));
    glUniformMatrix4fv(glGetUniformLocation(terrainShaderID, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(terrainShaderID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniform3fv(glGetUniformLocation(terrainShaderID, "cameraPosition"), 1, glm::value_ptr(cameraPosition));

    glUniform3f(glGetUniformLocation(terrainShaderID, "lightDirection"), glm::cos(t), -0.5f, glm::sin(t));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, heightmapID2);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, heightNormalID);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, dirtID);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, sandID);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, grassID);

    glBindVertexArray(plane);
    glDrawElements(GL_TRIANGLES, planeSize, GL_UNSIGNED_INT, 0);
}

void renderWater(glm::mat4 view, glm::mat4 projection, float t, int depth_renderbuffer)
{
    glUseProgram(waterShaderID);
    glDisable(GL_CULL_FACE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);

    glm::mat4 world = glm::mat4(1.f);
    world = glm::translate(world, glm::vec3(0, 40, 0));

    glUniformMatrix4fv(glGetUniformLocation(waterShaderID, "world"), 1, GL_FALSE, glm::value_ptr(world));
    glUniformMatrix4fv(glGetUniformLocation(waterShaderID, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(waterShaderID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniform3fv(glGetUniformLocation(waterShaderID, "cameraPosition"), 1, glm::value_ptr(cameraPosition));

    glUniform1f(glGetUniformLocation(waterShaderID, "t"), t);
    glUniform1i(glGetUniformLocation(waterShaderID, "heightMap"), waterHeightmap);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, depth_renderbuffer);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, waterHeightmap);

    glBindVertexArray(water);
    glDrawElements(GL_TRIANGLES, planeSize, GL_UNSIGNED_INT, 0);
}


void setupSkyBox()
{
    // Vertices of our triangle!
    // need 24 vertices for normal/uv-mapped Cube
    float vertices[] = {
        // positions            //colors            // tex coords   // normals
        0.5f, -0.5f, -0.5f,     1.0f, 0.0f, 1.0f,   1.f, 0.f,       0.f, -1.f, 0.f,
        0.5f, -0.5f, 0.5f,      1.0f, 1.0f, 0.0f,   1.f, 1.f,       0.f, -1.f, 0.f,
        -0.5f, -0.5f, 0.5f,     0.0f, 1.0f, 1.0f,   0.f, 1.f,       0.f, -1.f, 0.f,
        -0.5f, -0.5f, -.5f,     1.0f, 1.0f, 1.0f,   0.f, 0.f,       0.f, -1.f, 0.f,

        0.5f, 0.5f, -0.5f,      1.0f, 1.0f, 1.0f,   2.f, 0.f,       1.f, 0.f, 0.f,
        0.5f, 0.5f, 0.5f,       1.0f, 1.0f, 1.0f,   2.f, 1.f,       1.f, 0.f, 0.f,

        0.5f, 0.5f, 0.5f,       1.0f, 1.0f, 1.0f,   1.f, 2.f,       0.f, 0.f, 1.f,
        -0.5f, 0.5f, 0.5f,      1.0f, 1.0f, 1.0f,   0.f, 2.f,       0.f, 0.f, 1.f,

        -0.5f, 0.5f, 0.5f,      1.0f, 1.0f, 1.0f,   -1.f, 1.f,      -1.f, 0.f, 0.f,
        -0.5f, 0.5f, -.5f,      1.0f, 1.0f, 1.0f,   -1.f, 0.f,      -1.f, 0.f, 0.f,

        -0.5f, 0.5f, -.5f,      1.0f, 1.0f, 1.0f,   0.f, -1.f,      0.f, 0.f, -1.f,
        0.5f, 0.5f, -0.5f,      1.0f, 1.0f, 1.0f,   1.f, -1.f,      0.f, 0.f, -1.f,

        -0.5f, 0.5f, -.5f,      1.0f, 1.0f, 1.0f,   3.f, 0.f,       0.f, 1.f, 0.f,
        -0.5f, 0.5f, 0.5f,      1.0f, 1.0f, 1.0f,   3.f, 1.f,       0.f, 1.f, 0.f,

        0.5f, -0.5f, 0.5f,      1.0f, 1.0f, 1.0f,   1.f, 1.f,       0.f, 0.f, 1.f,
        -0.5f, -0.5f, 0.5f,     1.0f, 1.0f, 1.0f,   0.f, 1.f,       0.f, 0.f, 1.f,

        -0.5f, -0.5f, 0.5f,     1.0f, 1.0f, 1.0f,   0.f, 1.f,       -1.f, 0.f, 0.f,
        -0.5f, -0.5f, -.5f,     1.0f, 1.0f, 1.0f,   0.f, 0.f,       -1.f, 0.f, 0.f,

        -0.5f, -0.5f, -.5f,     1.0f, 1.0f, 1.0f,   0.f, 0.f,       0.f, 0.f, -1.f,
        0.5f, -0.5f, -0.5f,     1.0f, 1.0f, 1.0f,   1.f, 0.f,       0.f, 0.f, -1.f,

        0.5f, -0.5f, -0.5f,     1.0f, 1.0f, 1.0f,   1.f, 0.f,       1.f, 0.f, 0.f,
        0.5f, -0.5f, 0.5f,      1.0f, 1.0f, 1.0f,   1.f, 1.f,       1.f, 0.f, 0.f,

        0.5f, 0.5f, -0.5f,      1.0f, 1.0f, 1.0f,   2.f, 0.f,       0.f, 1.f, 0.f,
        0.5f, 0.5f, 0.5f,       1.0f, 1.0f, 1.0f,   2.f, 1.f,       0.f, 1.f, 0.f
    };

    unsigned int indices[] = {  // note that we start from 0!
        // DOWN
        0, 1, 2,   // first triangle
        0, 2, 3,    // second triangle
        // BACK
        14, 6, 7,   // first triangle
        14, 7, 15,    // second triangle
        // RIGHT
        20, 4, 5,   // first triangle
        20, 5, 21,    // second triangle
        // LEFT
        16, 8, 9,   // first triangle
        16, 9, 17,    // second triangle
        // FRONT
        18, 10, 11,   // first triangle
        18, 11, 19,    // second triangle
        // UP
        22, 12, 13,   // first triangle
        22, 13, 23,    // second triangle
    };

    cubeSize = sizeof(indices);

    glGenVertexArrays(1, &VAO);
    unsigned int VBO;
    glGenBuffers(1, &VBO);
    unsigned int EBO;
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    int stride = sizeof(float) * 11;

    // position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);
    // color
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * 3));
    glEnableVertexAttribArray(1);
    // uv
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * 6));
    glEnableVertexAttribArray(2);
    // normal
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * 8));
    glEnableVertexAttribArray(3);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    ///END SETUP OBJECT///
}

void handleInput(GLFWwindow* window, float deltaTime) 
{
    static bool w, s, a, d, space, ctrl, shift;
    static double cursorX = -1, cursorY = -1, lastCursorX, lastCursorY;
    static float pitch, yaw;
    static float speed = 100.0f;

    float sensitivity = 100.0f * deltaTime;
    float step = speed * deltaTime;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)				w = true;
    else if (glfwGetKey(window, GLFW_KEY_W) == GLFW_RELEASE)		w = false;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)				s = true;
    else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_RELEASE)		s = false;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)				a = true;
    else if (glfwGetKey(window, GLFW_KEY_A) == GLFW_RELEASE)		a = false;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)				d = true;
    else if (glfwGetKey(window, GLFW_KEY_D) == GLFW_RELEASE)		d = false;

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)				space = true;
    else if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE)		space = false;
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)		ctrl = true;
    else if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_RELEASE)	ctrl = false;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)          shift = true;
    else if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_RELEASE)   shift = false;

    if (cursorX == -1) {
        glfwGetCursorPos(window, &cursorX, &cursorY);
    }

    lastCursorX = cursorX;
    lastCursorY = cursorY;
    glfwGetCursorPos(window, &cursorX, &cursorY);

    glm::vec2 mouseDelta(cursorX - lastCursorX, cursorY - lastCursorY);

    // TODO: calculate rotation & movement
    yaw -= mouseDelta.x * sensitivity;
    pitch += mouseDelta.y * sensitivity;

    if (pitch < -90.0f) pitch = -90.0f;
    else if (pitch > 90.0f) pitch = 90.0f;

    if (yaw < -180.0f) yaw += 360;
    else if (yaw > 180.0f) yaw -= 360;

    glm::vec3 euler(glm::radians(pitch), glm::radians(yaw), 0);
    glm::quat q(euler);

    // update camera position / forward & up
    glm::vec3 translation(0, 0, 0);
    cameraPosition += q * translation;

    cameraUp = q * glm::vec3(0, 1, 0);
    cameraForward = q * glm::vec3(0, 0, 1);
    cameraSideward = q * glm::vec3(1, 0, 0);

    glm::vec3 movement = glm::vec3(0, 0, 0);

    if (w)
    {
        movement += glm::vec3(cameraForward.x, 0, cameraForward.z);
    }
    else if (s)
    {
        movement -= glm::vec3(cameraForward.x, 0, cameraForward.z);
    }
    if (a)
    {
        movement += glm::vec3(cameraSideward.x, 0, cameraSideward.z);
    }
    else if (d)
    {
        movement -= glm::vec3(cameraSideward.x, 0, cameraSideward.z);
    }
    if (space)
    {
        movement += glm::vec3(0, 1, 0);
    }
    else if (ctrl)
    {
        movement -= glm::vec3(0, 1, 0);
    }

    float movementSpeedMultiplier = 1;
    if (shift)
    {
        movementSpeedMultiplier = 2;
    }

    if (w || s || a || d || space || ctrl)
    {
        cameraPosition += normalize(movement) * movementSpeed * movementSpeedMultiplier;
    }
}

//Model setup
    //Model backpack("backpack/pillar.obj");

    //Image setup
    //unsigned int heightMapID = loadTexture("backpack/displacementPillar.jpg", GL_RGB);

    //displacementShader.use();

    //glUniform1i(glGetUniformLocation(displacementShader.ID, "heightMap"), heightMapID);
    //glActiveTexture(GL_TEXTURE0 + 5);
    //glBindTexture(GL_TEXTURE_2D, heightMapID);

//glActiveTexture(GL_TEXTURE0 + 2); // Texture unit 0
        //glBindTexture(GL_TEXTURE_2D, heightMapID);

        //glm::mat4 model = glm::mat4(1.0f);
        //model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f)); // translate it down so it's at the center of the scene
        //model = glm::scale(model, glm::vec3(0.1f, 0.1f, 0.1f));	// it's a bit too big for our scene, so scale it down
        //shader.setMat4("model", model);
        //backpack.Draw(shader);
