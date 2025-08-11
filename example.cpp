#include "GPF.hpp"

using namespace GPF;

int main()
{
    // Init GLFW
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Deferred PBR", NULL, NULL);
    glfwMakeContextCurrent(window);
    glewInit();

    // Camera setup
    Camera cam;
    cam.Position = {0.0f, 0.0f, 3.0f};
    cam.Fov = 60.0f; cam.Aspect = 1280.0f/720.0f;
    cam.Near = 0.1f; cam.Far = 100.0f;
    cam.Yaw = -90.0f; cam.Pitch = 0.0f;

    // Load a test mesh
    Geometry mesh;
    LoadOBJ("model.obj", mesh);

    // Create VAO/VBO
    mesh.VAO = GenerateVAO();
    BindVAO(mesh.VAO);
    mesh.VBO = GenerateBuffer(BufferType::Array);
    BindBuffer(BufferType::Array, mesh.VBO);
    UploadDataImmutable(BufferType::Array, mesh.Vertices_1P1N1UV1T1BT);
    mesh.IBO = GenerateBuffer(BufferType::Index);
    BindBuffer(BufferType::Index, mesh.IBO);
    UploadDataImmutable(BufferType::Index, mesh.Indices);
    U32 offset = 0;
    offset = ElementLayout<float>(0, 3, sizeof(Vertex1P1N1UV1T1BT), offset); // pos
    offset = ElementLayout<float>(1, 3, sizeof(Vertex1P1N1UV1T1BT), offset); // normal
    offset = ElementLayout<float>(2, 2, sizeof(Vertex1P1N1UV1T1BT), offset); // uv
    offset = ElementLayout<float>(3, 3, sizeof(Vertex1P1N1UV1T1BT), offset); // tangent
    offset = ElementLayout<float>(4, 3, sizeof(Vertex1P1N1UV1T1BT), offset); // bitangent

    // Create screen quad
    auto quad = Primitive_ScreenQuad();
    U32 quadVAO = GenerateVAO();
    BindVAO(quadVAO);
    U32 quadVBO = GenerateBuffer(BufferType::Array);
    BindBuffer(BufferType::Array, quadVBO);
    UploadDataImmutable(BufferType::Array, quad.Vertices_1P1N1UV);
    U32 quadIBO = GenerateBuffer(BufferType::Index);
    BindBuffer(BufferType::Index, quadIBO);
    UploadDataImmutable(BufferType::Index, quad.Indices);
    offset = 0;
    offset = ElementLayout<float>(0, 3, sizeof(Vertex1P1N1UV), offset);
    offset = ElementLayout<float>(1, 2, sizeof(Vertex1P1N1UV), offset);

    // Create G-buffer
    U32 gBuffer = GenerateFramebuffer();
    U32 gPosition = GenerateRenderTarget_Color<float>(0, 1280, 720, GL_RGB16F, GL_RGB);
    U32 gNormal   = GenerateRenderTarget_Color<float>(1, 1280, 720, GL_RGB16F, GL_RGB);
    U32 gAlbedo   = GenerateRenderTarget_Color<unsigned char>(2, 1280, 720, GL_RGBA, GL_RGBA);
    U32 rboDepth  = GenerateRenderTarget_Depth<float>(1280, 720);
    GLenum attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
    glDrawBuffers(3, attachments);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Geometry pass shader
    ShaderList geomShaders;
    LoadShader("geom.vert", GL_VERTEX_SHADER, geomShaders);
    LoadShader("geom.frag", GL_FRAGMENT_SHADER, geomShaders);
    U32 geomProg; CompileShaderList(geomShaders, geomProg);

    // Lighting pass shader (PBR)
    ShaderList lightShaders;
    LoadShader("light.vert", GL_VERTEX_SHADER, lightShaders);
    LoadShader("light.frag", GL_FRAGMENT_SHADER, lightShaders);
    U32 lightProg; CompileShaderList(lightShaders, lightProg);

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        // Geometry pass
        glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(geomProg);
        SetMat4("uProjection", GetProjectionMatrix(cam), geomProg);
        SetMat4("uView", GetViewMatrix(cam), geomProg);
        glm::mat4 model(1.0f);
        SetMat4("uModel", model, geomProg);
        BindVAO(mesh.VAO);
        glDrawElements(GL_TRIANGLES, mesh.IndexCount, GL_UNSIGNED_INT, 0);

        // Lighting pass
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(lightProg);
        glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, gPosition);
        glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, gNormal);
        glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, gAlbedo);
        SetVec3("lightPos", glm::vec3(10.0, 10.0, 10.0), lightProg);
        SetVec3("lightColor", glm::vec3(300.0, 300.0, 300.0), lightProg);
        SetVec3("camPos", cam.Position, lightProg);
        BindVAO(quadVAO);
        glDrawElements(GL_TRIANGLES, quad.IndexCount, GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
