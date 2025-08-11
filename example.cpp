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

/*
================== geom.vert ==================
#version 450 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in vec3 aBitangent;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;

void main()
{
    FragPos = vec3(uModel * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(uModel))) * aNormal;
    TexCoord = aTexCoord;
    gl_Position = uProjection * uView * vec4(FragPos, 1.0);
}
================================================
*/

/*
================== geom.frag ==================
#version 450 core
layout(location = 0) out vec3 gPosition;
layout(location = 1) out vec3 gNormal;
layout(location = 2) out vec4 gAlbedoMetallic;
layout(location = 3) out vec2 gRoughAO; // optional

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

uniform sampler2D uAlbedoMap;
uniform sampler2D uMetallicMap;
uniform sampler2D uRoughnessMap;
uniform sampler2D uAOMap;

void main()
{
    gPosition = FragPos;
    gNormal = normalize(Normal);
    vec3 albedo = texture(uAlbedoMap, TexCoord).rgb;
    float metallic = texture(uMetallicMap, TexCoord).r;
    float roughness = texture(uRoughnessMap, TexCoord).r;
    float ao = texture(uAOMap, TexCoord).r;
    gAlbedoMetallic = vec4(albedo, metallic);
    gRoughAO = vec2(roughness, ao);
}
================================================
*/

/*
================== light.vert ==================
#version 450 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoord;
out vec2 TexCoord;
void main()
{
    TexCoord = aTexCoord;
    gl_Position = vec4(aPos, 1.0);
}
================================================
*/

/*
================== light.frag ==================
#version 450 core
out vec4 FragColor;
in vec2 TexCoord;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoMetallic;
uniform sampler2D gRoughAO;

uniform vec3 camPos;
uniform vec3 lightPos;
uniform vec3 lightColor;

// PBR helpers
const float PI = 3.14159265359;

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    return a2 / (PI * denom * denom);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx1 = GeometrySchlickGGX(NdotV, roughness);
    float ggx2 = GeometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

void main()
{
    vec3 FragPos = texture(gPosition, TexCoord).rgb;
    vec3 Normal = normalize(texture(gNormal, TexCoord).rgb);
    vec3 albedo = pow(texture(gAlbedoMetallic, TexCoord).rgb, vec3(2.2)); // gamma->linear
    float metallic = texture(gAlbedoMetallic, TexCoord).a;
    float roughness = texture(gRoughAO, TexCoord).r;
    float ao = texture(gRoughAO, TexCoord).g;

    vec3 V = normalize(camPos - FragPos);
    vec3 L = normalize(lightPos - FragPos);
    vec3 H = normalize(V + L);
    float distance = length(lightPos - FragPos);
    float attenuation = 1.0 / (distance*distance);
    vec3 radiance = lightColor * attenuation;

    // base reflectivity
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    float NDF = DistributionGGX(Normal, H, roughness);
    float G   = GeometrySmith(Normal, V, L, roughness);
    vec3  F   = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 numerator = NDF * G * F;
    float denom = 4.0 * max(dot(Normal, V), 0.0) * max(dot(Normal, L), 0.0) + 0.001;
    vec3 specular = numerator / denom;

    vec3 kS = F;
    vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);

    float NdotL = max(dot(Normal, L), 0.0);
    vec3 Lo = (kD * albedo / PI + specular) * radiance * NdotL;

    vec3 ambient = vec3(0.03) * albedo * ao;
    vec3 color = ambient + Lo;

    color = color / (color + vec3(1.0)); // tone map
    color = pow(color, vec3(1.0/2.2));   // gamma correct

    FragColor = vec4(color, 1.0);
}
================================================
*/
