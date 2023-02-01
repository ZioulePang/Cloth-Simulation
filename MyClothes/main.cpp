#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "shader.h"
#include "Camera.h"
#include "model.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <iostream>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
unsigned int loadTexture(const char* path);
void processInput(GLFWwindow* window);

// settings
const unsigned int SCR_WIDTH = 1600;
const unsigned int SCR_HEIGHT = 1200;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

float nodesSimTime;
double gravitySimTime;
float springTime;
float holderSimTime;
float windSimTime;
float ballTime;
float frictionTime;
float cornerTime;


float radian = 0.8f;
// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    bool show_demo_window = true;
    bool show_another_window = false;

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    stbi_set_flip_vertically_on_load(true);

    glEnable(GL_DEPTH_TEST);

    Shader planeShader("./Shader/light_Vertex.glsl", "./Shader/light_Fragment.glsl");
    Shader clothShader("./Shader/test.vs", "./Shader/test.fs");

    Model ourModel("./resources/nanosuit/10x10.obj");
    ourModel.read();
    ourModel.vertexSort();
    ourModel.CreateSpring();

    bool useWind = false;
    bool useCorner = false;
    bool useBallTest = false;
    bool useFriction = false;

    float planeVertices[] = {
        // positions          // texture 
         5.0f, -0.5f,  5.0f,  1.0f, 0.0f,
        -5.0f, -0.5f,  5.0f,  0.0f, 0.0f,
        -5.0f, -0.5f, -5.0f,  0.0f, 1.0f,

         5.0f, -0.5f,  5.0f,  1.0f, 0.0f,
        -5.0f, -0.5f, -5.0f,  0.0f, 1.0f,
         5.0f, -0.5f, -5.0f,  1.0f, 1.0f
    };

    // plane VAO
    unsigned int planeVAO, planeVBO;
    glGenVertexArrays(1, &planeVAO);
    glGenBuffers(1, &planeVBO);
    glBindVertexArray(planeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), &planeVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glBindVertexArray(0);

    // load textures
    // -------------
    unsigned int planeMap = loadTexture("./Image/container.jpg");
    unsigned int diffuseMap = loadTexture("./Image/disco.jpg");

    planeShader.use();
    planeShader.setInt("material.diffuse", 0);
    planeShader.setInt("material.specular", 1);

    clothShader.use();
    clothShader.setInt("material.diffuse", 0);
    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        double prev = glfwGetTime();
        double now;
        // per-frame time logic
        // --------------------
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);

        // render
        // ------
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (useWind) 
        {
            ourModel.SimulateWind(glm::vec3(0, 10, 0));
            now = glfwGetTime();
            windSimTime = now - prev;
            prev = now;
        }

        ourModel.SimulateInternalForce(0.0016);
        now = glfwGetTime();
        springTime = now - prev;
        prev = now;

        ourModel.SimulateGravity();
        now = glfwGetTime();
        gravitySimTime = now - prev;
        prev = now;

        ourModel.CollisionTest(glm::vec3(-5.0f, -10.5f, 5.0f), glm::vec3(0, 1, 0));
        now = glfwGetTime();
        holderSimTime = now - prev;
        prev = now;

        if (useFriction)
        {
            ourModel.SimulateFriction(glm::vec3(0.0f, -9.5f, 0.0f), radian);
            now = glfwGetTime();
            frictionTime = now - prev;
            prev = now;
        }


        if (useBallTest) 
        {
            ourModel.CollisionBallTest(glm::vec3(0.0f, -9.5f, 0.0f), radian);
            now = glfwGetTime();
            ballTime = now - prev;
            prev = now;
        }

        if (useCorner) 
        {
            ourModel.SimulateCorners(100, 120);
            now = glfwGetTime();
            cornerTime = now - prev;
            prev = now;
        }

        ourModel.SimulateNodes(0.0016);
        now = glfwGetTime();
        nodesSimTime = now - prev;
        prev = now;


        ourModel.updatePosition();

        clothShader.use();

        clothShader.setVec3("viewPos", camera.Position);
        clothShader.setFloat("material.shininess", 32.0f);
        clothShader.setVec3("light.direction", -0.2f, -1.0f, -0.3f);
        clothShader.setVec3("light.ambient", 0.25f, 0.25f, 0.25f);
        clothShader.setVec3("light.diffuse", 0.6f, 0.6f, 0.6f);
        clothShader.setVec3("light.specular", 0.8f, 0.8f, 0.8f);
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        clothShader.setMat4("projection", projection);
        clothShader.setMat4("view", view);

        // render the loaded model
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f)); // translate it down so it's at the center of the scene
        //model = glm::rotate(model, (float)glfwGetTime(), glm::vec3(0.0f, 2.0f, 0.0f));
        model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));	// it's a bit too big for our scene, so scale it down
        clothShader.setMat4("model", model);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseMap);
        ourModel.Draw(clothShader);

        planeShader.use();
        planeShader.setVec3("viewPos", camera.Position);
        planeShader.setFloat("material.shininess", 32.0f);
        planeShader.setVec3("light.direction", -0.2f, -1.0f, -0.3f);
        planeShader.setVec3("light.ambient", 0.25f, 0.25f, 0.25f);
        planeShader.setVec3("light.diffuse", 0.6f, 0.6f, 0.6f);
        planeShader.setVec3("light.specular", 0.8f, 0.8f, 0.8f);
        projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        view = camera.GetViewMatrix();
        planeShader.setMat4("projection", projection);
        planeShader.setMat4("view", view);
        glm::mat4 clothmodel = glm::mat4(1.0f);
        clothmodel = glm::translate(clothmodel, glm::vec3(0.0f, -10.0f, 0.0f));
        planeShader.setMat4("model", clothmodel);
        glBindVertexArray(planeVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseMap);
        glBindVertexArray(planeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        {
            static float f = 0.0f;
            static int counter = 0;

            ImGui::Begin("Test"); // Create a window called "Hello, world!" and append into it.
            ImGui::Text("Wind");
            ImGui::Checkbox("Use Wind", &useWind);
            ImGui::Checkbox("Use Corner", &useCorner);
            ImGui::Checkbox("Use Ball Test", &useBallTest);
            ImGui::Checkbox("Use Friction Test", &useFriction);

            if (ImGui::Button("Write new file"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
                ourModel.Write("New10x10.txt");

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            ImGui::End();
        }

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

unsigned int loadTexture(char const* path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
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
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}