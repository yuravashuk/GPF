
#include "GPF.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "ThirdParty/stb_image.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "ThirdParty/tiny_obj_loader.h"

#include <algorithm>
#include <set>

namespace std
{
    template<>
    struct hash<GPF::Vertex1P1N1UV1T1BT>
    {
        size_t operator()(const GPF::Vertex1P1N1UV1T1BT &inVertex) const
        {
            return ((hash<glm::vec3>()(inVertex.Position) ^
                    (hash<glm::vec3>()(inVertex.Normal) << 1)) >> 1) ^
                    (hash<glm::vec2>()(inVertex.TexCoord ) << 1);
        }
    };
}

namespace GPF
{
    /* Profiling */

    void ProfilerManager::Store( const char *inTag, F64 inTime )
    {
        ProfilingMap.emplace( inTag, inTime );
    }

    void ProfilerManager::DumpToConsole()
    {
        // TODO: implememnt dump to std::cout
    }

    void ProfilerManager::DumpToCSV(const std::string &inFileName)
    {
        // TODO: implement export to csv code
    }

    Profile::Profile( const char *inTag, ProfilingType inType )
        : Tag( inTag )
        , Type( inType )
        , IsEnded( false )
    {
        if (inType == ProfilingType::CPU)
            StartTime = glfwGetTime();
        else 
            glBeginQuery(GL_TIME_ELAPSED, Query);
    }
    
    Profile::~Profile()
    {
        EndProfiling();
    }

    void Profile::EndProfiling()
    {
        if (!IsEnded)
        {
            IsEnded = true;
            F64 endTime = 0.0f;

            if (Type == ProfilingType::CPU)
            {
                endTime = glfwGetTime() - StartTime;
            }
            else 
            {
                glEndQuery(GL_TIME_ELAPSED);
                endTime = Query * 0.000001;
            }
            
            g_Profiler.Store(Tag, endTime);
        }
    }

    /* Converters */

    static constexpr GLenum BufferToGlBuffer(BufferType inBuffer)
    {
        switch (inBuffer)
        {
        case BufferType::Array: return GL_ARRAY_BUFFER;
        case BufferType::Index: return GL_ELEMENT_ARRAY_BUFFER;
        case BufferType::CopyRead: return GL_COPY_READ_BUFFER;
        case BufferType::CopyWrite: return GL_COPY_WRITE_BUFFER;
        case BufferType::PixelUnpack: return GL_PIXEL_UNPACK_BUFFER;
        case BufferType::PixelPack: return GL_PIXEL_PACK_BUFFER;
        //case PxBuffer::Query: return GL_QUERY_BUFFER;
        case BufferType::Texture: return GL_TEXTURE_BUFFER;
        case BufferType::TransformFeedback: return GL_TRANSFORM_FEEDBACK_BUFFER;
        case BufferType::Uniform: return GL_UNIFORM_BUFFER;
        case BufferType::DrawIndirect: return GL_DRAW_INDIRECT_BUFFER;
        case BufferType::AtomicCounter: return GL_ATOMIC_COUNTER_BUFFER;
        case BufferType::DispatchIndirect: return GL_DISPATCH_INDIRECT_BUFFER;
        case BufferType::ShaderStorage: return GL_SHADER_STORAGE_BUFFER;
        }
        return GL_ARRAY_BUFFER;
    }

    static constexpr GLenum BufferUsageToGlBufferUsage(BufferUsage inUsage)
    {
        switch (inUsage)
        {
        case BufferUsage::DynamicCopy: return GL_DYNAMIC_COPY;
        case BufferUsage::DynamicDraw: return GL_DYNAMIC_DRAW;
        case BufferUsage::DynamicRead: return GL_DYNAMIC_READ;
        case BufferUsage::StaticCopy: return GL_STATIC_COPY;
        case BufferUsage::StaticDraw: return GL_STATIC_DRAW;
        case BufferUsage::StaticRead: return GL_STATIC_READ;
        case BufferUsage::StreamCopy: return GL_STREAM_COPY;
        case BufferUsage::StreamDraw: return GL_STREAM_DRAW;
        case BufferUsage::StreamRead: return GL_STREAM_READ;
        }
        return GL_STATIC_DRAW;
    }

    /* IO */

    bool LoadFile(const std::string &inFileName, std::string &outData)
    {
        std::ifstream file(inFileName.c_str(), std::ios::in | std::ios::binary | std::ios::ate);

        if (file.bad())
        {
            return false;
        }

        file.seekg(0, std::ios::end);
        const auto fileSize = file.tellg();

        if (fileSize > 0)
        {
            outData.resize( static_cast<size_t>(fileSize) );
            file.seekg(0, std::ios::beg);
            file.read(&outData.front(), fileSize);
        }

        file.close();
        return true;
    }

    /* Images */

    bool LoadImage(const std::string &inFileName, Image &outImage)
    {
        I32 x = 0;
        I32 y = 0;
        I32 comp = 0;

        stbi_set_flip_vertically_on_load(true);
        outImage.Data = stbi_load(inFileName.c_str(), &x, &y, &comp, 0);

        if (!glm::isPowerOfTwo(x) || !glm::isPowerOfTwo(y))
        {
            std::cerr << "Warning: not vaid width or height - " << inFileName << "\n";
        }

        if (!outImage.Data)
        {
			std::cerr << "Error: failed to load image - " << inFileName << "\n";
            return false;
        }

        outImage.Width = x;
        outImage.Height = y;
        outImage.Components = comp;

        return true;
    }

    void FreeImage(Image &outImage)
    {
        if (!outImage.Data)
        {
            stbi_image_free(outImage.Data);

            outImage.Data = nullptr;
            outImage.Width = 0;
            outImage.Height = 0;
            outImage.Components = 0;
        }
    }

    /* Load OBJ */

    inline void FetchMaterials(const std::vector<tinyobj::material_t> &inMatreials, Geometry &outGeometry)
    {
        for (const auto &material : inMatreials) 
        {
            // TODO: Get parameters from material.ambient_texopt and other textures ...
            const auto founded = outGeometry.Materials.find(material.name);

            if (founded == std::end(outGeometry.Materials))
            {
                MaterialInfo materialInfo;

                materialInfo.Name = material.name;
                materialInfo.AmbientTextureName = material.ambient_texname;
                materialInfo.DiffuseTextureName = material.diffuse_texname;
                materialInfo.SpecularTextureName = material.specular_texname;
                materialInfo.HighlightTextureName = material.specular_highlight_texname;
                materialInfo.BumpTextureName = material.bump_texname;
                materialInfo.DisplacementTextureName = material.displacement_texname;
                materialInfo.AlphaTextureName = material.alpha_texname;
                materialInfo.ReflectionTextureName = material.reflection_texname;

                outGeometry.Materials.emplace( material.name, materialInfo );
            }
        }
    }

    bool LoadOBJ(const std::string &inFileName, Geometry &outGeometry)
    {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string errs;
        std::string warnings;
        std::unordered_map<Vertex1P1N1UV1T1BT, U32> uniqueVertices = {};

        if ( !tinyobj::LoadObj( &attrib, &shapes, &materials, &warnings, &errs, inFileName.c_str() ) ) 
        {
            std::cerr << "Failed load mesh - " << inFileName << std::endl;
            return false;
        }

        if (!warnings.empty())
        {
            std::cerr << "Mesh loading - " << inFileName << " warning - " << warnings << ".\n";
        }

        if (!errs.empty())
        {
            std::cerr << "Mesh loading - " << errs << std::endl;
            return false;
        }

        FetchMaterials( materials, outGeometry );

        for (const auto& shape : shapes) 
        {
            for (const auto& index : shape.mesh.indices) 
            {
                Vertex1P1N1UV1T1BT vertex = {};

                // position
                vertex.Position = {
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]
                };

                // texcoords
                vertex.TexCoord = { 1.0, 1.0 };

                if (index.texcoord_index > 0)
                {
                    vertex.TexCoord = {
                        attrib.texcoords[2 * index.texcoord_index + 0],
                        1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
                    };
                }

                // normals
                vertex.Normal = { 1.0f, 1.0f, 1.0f };

                if (!attrib.normals.empty())
                {
					if (index.normal_index >= 0)
					{
						vertex.Normal = {
							attrib.normals[3 * index.normal_index + 0],
							attrib.normals[3 * index.normal_index + 1],
							attrib.normals[3 * index.normal_index + 2]
						};
					}
                }

                if (uniqueVertices.count(vertex) == 0)
                {
                    uniqueVertices[vertex] = static_cast<U32>(outGeometry.Vertices_1P1N1UV1T1BT.size());
                    outGeometry.Vertices_1P1N1UV1T1BT.push_back(vertex);
                }

                outGeometry.Indices.push_back(uniqueVertices[vertex]);

                outGeometry.Bounds.Min = glm::min(outGeometry.Bounds.Min, vertex.Position);
			    outGeometry.Bounds.Max = glm::max(outGeometry.Bounds.Max, vertex.Position);
            }
        }

		// compute tangent / bitangent
		for (size_t i = 0; i < outGeometry.Vertices_1P1N1UV1T1BT.size() - 1; i += 3)
		{
			Vertex1P1N1UV1T1BT& v0 = outGeometry.Vertices_1P1N1UV1T1BT[i + 0];
			Vertex1P1N1UV1T1BT& v1 = outGeometry.Vertices_1P1N1UV1T1BT[i + 1];
			Vertex1P1N1UV1T1BT& v2 = outGeometry.Vertices_1P1N1UV1T1BT[i + 2];

			glm::vec3 edge1 = v1.Position - v0.Position;
			glm::vec3 edge2 = v2.Position - v1.Position;

			glm::vec2 deltaUV1 = v1.TexCoord - v0.TexCoord;
			glm::vec2 deltaUV2 = v2.TexCoord - v1.TexCoord;

			float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

			glm::vec3 tangent1;
			glm::vec3 bitangent1;

			tangent1.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
			tangent1.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
			tangent1.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
			tangent1 = glm::normalize(tangent1);

			bitangent1.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
			bitangent1.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
			bitangent1.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);
			bitangent1 = glm::normalize(bitangent1);

			v0.Tangent = tangent1;
			v1.Tangent = tangent1;
			v2.Tangent = tangent1;

			v0.Bitangent = bitangent1;
			v1.Bitangent = bitangent1;
			v2.Bitangent = bitangent1;
		}
		
        outGeometry.VertexCount = static_cast<U32>(outGeometry.Vertices_1P1N1UV1T1BT.size());
        outGeometry.IndexCount = static_cast<U32>(outGeometry.Indices.size());

        outGeometry.Bounds.Dimensions = outGeometry.Bounds.Max - outGeometry.Bounds.Min;
	    outGeometry.Bounds.Center = (outGeometry.Bounds.Max + outGeometry.Bounds.Min) / 2.0f;

        return true;
    }

    /* Primitives */

    Geometry Primitive_Plane()
    {
        Geometry result;

		Vertex1P1N1UV vertex;
        vertex.Position = glm::vec3(-1.0f, 0.0f, 0.f);
        vertex.TexCoord = glm::vec2(0.0f, 0.0f);
        result.Vertices_1P1N1UV.push_back( vertex );

        vertex.Position = glm::vec3(1.0f, 1.0f, 0.f);
        vertex.TexCoord = glm::vec2(1.0f, 0.0f);
        result.Vertices_1P1N1UV.push_back( vertex );

        vertex.Position = glm::vec3(1.0f,-1.0f, 0.f);
        vertex.TexCoord = glm::vec2(1.0f, 1.0f);
        result.Vertices_1P1N1UV.push_back( vertex );

        vertex.Position = glm::vec3(-1.0f,-1.0f, 0.f);
        vertex.TexCoord = glm::vec2(0.0f, 1.0f);
        result.Vertices_1P1N1UV.push_back( vertex );

        result.Indices = {
			0, 1, 2,
			2, 3, 0
		};

        result.IndexCount = static_cast<U32>(result.Indices.size());
        result.VertexCount = static_cast<U32>(result.Vertices_1P1N1UV.size());

        return result;
    }

    Geometry Primitive_ScreenQuad()
    {
        Geometry result;

		Vertex1P1N1UV topLeft;
        topLeft.Position = glm::vec3(-1.0f, -1.0f, 0.0f);
        topLeft.TexCoord = glm::vec2(0.0f, 0.0f);

		Vertex1P1N1UV topRight;
        topRight.Position = glm::vec3(1.0f, -1.0f, 0.0f);
        topRight.TexCoord = glm::vec2(1.0f, 0.0f);

		Vertex1P1N1UV bottomRight;
        bottomRight.Position = glm::vec3(1.0f, 1.0f, 0.0f);
        bottomRight.TexCoord = glm::vec2(1.0f, 1.0f);

		Vertex1P1N1UV bottomLeft;
        bottomLeft.Position = glm::vec3(-1.0f, 1.0f, 0.0f);
        bottomLeft.TexCoord = glm::vec2(0.0f, 1.0f);

        result.Vertices_1P1N1UV = {
            topLeft, topRight, bottomRight, bottomLeft
        };

        result.Indices = {
            0, 1, 2,
            2, 3, 0
		};

        result.IndexCount = static_cast<U32>(result.Indices.size());
        result.VertexCount = static_cast<U32>(result.Vertices_1P1N1UV.size());

        return result;
    }

     /* Camera */

    glm::mat4 GetProjectionMatrix(const Camera &inCamera)
    {
        return glm::perspective( glm::radians(inCamera.Fov) , inCamera.Aspect, inCamera.Near, inCamera.Far );
    }

    glm::mat4 GetViewMatrix(Camera &outCamera)
    {
        glm::vec3 front;
        front.x = glm::cos(glm::radians(outCamera.Yaw)) * glm::cos(glm::radians(outCamera.Pitch));
        front.y = glm::sin(glm::radians(outCamera.Pitch));
        front.z = glm::sin(glm::radians(outCamera.Yaw)) * glm::cos(glm::radians(outCamera.Pitch));
        outCamera.Front = glm::normalize(front);

        outCamera.Right = glm::normalize(glm::cross(outCamera.Front, outCamera.WorldUp));
        outCamera.Up = glm::normalize(glm::cross(outCamera.Right, outCamera.Front));

        return glm::lookAt( outCamera.Position, 
                            outCamera.Position + outCamera.Front, 
                            outCamera.Up );
    }

    void MoveForward(Camera &outCamera, F64 inDt)
    {
	    const F32 velocity = static_cast<F32>(inDt) * outCamera.MovementSpeed;
	    outCamera.Position += outCamera.Front * velocity;
    }

    void MoveBackward(Camera &outCamera, F64 inDt)
    {
	    const F32 velocity = static_cast<F32>(inDt) * outCamera.MovementSpeed;
	    outCamera.Position -= outCamera.Front * velocity;
    }

    void MoveLeft(Camera &outCamera, F64 inDt)
    {
	    const F32 velocity = static_cast<F32>(inDt) * outCamera.MovementSpeed;
	    outCamera.Position -= outCamera.Right * velocity;
    }

    void MoveRight(Camera &outCamera, F64 inDt)
    {
	    const F32 velocity = static_cast<F32>(inDt) * outCamera.MovementSpeed;
	    outCamera.Position += outCamera.Right * velocity;
    }

    void Rotate(Camera &outCamera, F64 inDeltaVertical, F64 inDeltaHorizontal)
    {
        F32 fX = static_cast<F32>(inDeltaVertical);
        F32 fY = static_cast<F32>(inDeltaHorizontal);

        F32 xOffset = fX - outCamera.LastX;
        F32 yOffset = outCamera.LastY - fY;

        outCamera.LastX = fX;
        outCamera.LastY = fY;

        xOffset *= outCamera.MouseSpeed;
        yOffset *= outCamera.MouseSpeed;

        outCamera.Yaw += xOffset;
        outCamera.Pitch += yOffset;
        outCamera.Pitch = glm::clamp(outCamera.Pitch, -89.0f, 89.0f);
    }

    /* Vertex Array Object */

    U32 GenerateVAO()
    {
        U32 vao = 0;
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        return vao;
    }

    void BindVAO(U32 inID)
    {
        glBindVertexArray(inID);
    }

    void DeleteVAO(U32& outVAO)
    {
        if (outVAO)
        {
            glDeleteVertexArrays(1, &outVAO);
            outVAO = 0;
        }
    }

    /* Buffer Object */

    U32 GenerateBuffer(BufferType inType)
    {
        U32 bufferID = 0;
        glGenBuffers(1, &bufferID);
        glBindBuffer(BufferToGlBuffer(inType), bufferID);
        return bufferID;
    }

    void BindBuffer(BufferType inType, U32 inID)
    {
        glBindBuffer(BufferToGlBuffer(inType), inID);
    }

    void UploadData(BufferType inType, BufferUsage inUsage, const void* inData, size_t inSize)
    {
        const auto target = BufferToGlBuffer(inType);
        const auto usage = BufferUsageToGlBufferUsage(inUsage);
        const auto size = static_cast<GLsizeiptr>( inSize );

        glBufferData(target, size, inData, usage);
    }

    void UploadDataImmutable(BufferType inType, const void* inData, size_t inSize)
    {
        const auto target = BufferToGlBuffer(inType);
        const auto size = static_cast<GLsizeiptr>( inSize );

        const auto flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT;
        glBufferStorage(target, size, NULL, flags);

        void* gpu = glMapBuffer(target, GL_WRITE_ONLY);
        memcpy(gpu, inData, size);
        glUnmapBuffer(target);
    }

    void InvalidateBuffer(U32 inBufferID)
    {
        glInvalidateBufferData(inBufferID);
    }

    void DeleteBuffer(U32& outBuffer)
    {
        if (outBuffer)
        {
            glDeleteBuffers(1, &outBuffer);
            outBuffer = 0;
        }
	}

    /* Textures */

    U32 GenerateTexture()
    {
        U32 textureID = 0;
        glGenTextures(1, &textureID);
        return textureID;
    }

    void UploadTextureData( const Image &inImage, U32 &outTextureID, bool inGenerateMipMaps )
    {
        GLenum format = GL_RGB;

	    if (inImage.Components == 1) format = GL_RED;
	    else if (inImage.Components == 3) format = GL_RGB;
	    else if (inImage.Components == 4) format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, outTextureID);
	    glTexImage2D(GL_TEXTURE_2D, 0, format, inImage.Width, inImage.Height, 0, format, GL_UNSIGNED_BYTE, inImage.Data);

		if (inGenerateMipMaps)
		{
			glGenerateMipmap(GL_TEXTURE_2D);
		}
    }

    void UploadTextureData( const void * inData,
                            U32 inWidth, 
                            U32 inHeight, 
                            U32 &outTextureID, 
                            GLenum inType,
                            GLenum inInternalFormat, 
                            GLenum inFormat )
    {
        glBindTexture(GL_TEXTURE_2D, outTextureID);
	    glTexImage2D(GL_TEXTURE_2D, 0, inInternalFormat, inWidth, inHeight, 0, inFormat, inType, inData);
    }

    void DeleteTexture(U32 &outID)
    {
        if (outID)
        {
            glDeleteTextures(1, &outID);
            outID = 0;
        }
    }

    /* Samplers */

    U32 GenerateSampler()
    {
        U32 samplerID = 0;
        glGenSamplers(1, &samplerID);
        return samplerID;
    }

    void SetupSamplerParameters(const SamplerParameters &inParameters, U32 inSamplerID)
    {
        glSamplerParameteri(inSamplerID, GL_TEXTURE_WRAP_S, inParameters.WrapS);
        glSamplerParameteri(inSamplerID, GL_TEXTURE_WRAP_T, inParameters.WrapT);
        glSamplerParameteri(inSamplerID, GL_TEXTURE_MAG_FILTER, inParameters.MagFilter);
        glSamplerParameteri(inSamplerID, GL_TEXTURE_MIN_FILTER, inParameters.MinFilter);
        glSamplerParameterf(inSamplerID, GL_TEXTURE_MAX_ANISOTROPY_EXT, inParameters.MaxAnisotropy);
    }

    void BindSampler( U32 inSamplerID, U32 inTextureUnit )
    {
        glBindSampler(inTextureUnit, inSamplerID);
    }

    void UnbindSampler( U32 inTextureUnit )
    {
        glBindSampler(inTextureUnit, 0);
    }

    void DeleteSampler(U32 &outSampler)
    {
        if (outSampler != 0)
        {
            glDeleteSamplers(1, &outSampler);
            outSampler = 0;
        }
    }

    /* Shaders */

    bool LoadShader(const std::string &inFileName, GLenum inType, ShaderList &outList)
    {
        std::string content;
        if (!LoadFile(inFileName, content))
        {
            std::cerr << "Failed to load shader - " << inFileName << "\n";
            return false;
        }

        if (!content.empty())
        {
            const char *shaderCode = content.c_str();
            U32 shaderID = glCreateShader(inType);
            glShaderSource(shaderID, 1, &shaderCode, NULL);
            glCompileShader(shaderID);

            I32 infoLogLen = 0;
            GLint result = GL_FALSE;

            glGetShaderiv(shaderID, GL_COMPILE_STATUS, &result);
            glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &infoLogLen);

            if (infoLogLen > 0 || result == GL_FALSE)
            {
                std::vector<char> shaderErrorMessages(infoLogLen + 1);
                glGetShaderInfoLog(shaderID, infoLogLen, NULL, &shaderErrorMessages[0]);
                std::cerr << "Failed compile shader - " << std::string((char*)&shaderErrorMessages[0]) << "\n";
                return false;
            }

			std::cout << "Loaded shader - " << inFileName << "\n";
            outList.push_back( shaderID );
            return true;
        }

        return false;
    }

    bool CompileShaderList( ShaderList &outShaders, U32 &outProgramID, bool inDeleteShaders )
    {
        outProgramID = glCreateProgram();

        for (U32 shader : outShaders)
        {
            if (shader != 0)
            {
                glAttachShader(outProgramID, shader);
            }
        }

        glLinkProgram(outProgramID);

        I32 infoLogLen = 0;
        GLint result = GL_FALSE;

        glGetProgramiv(outProgramID, GL_LINK_STATUS, &result);
        glGetProgramiv(outProgramID, GL_INFO_LOG_LENGTH, &infoLogLen);

        if (infoLogLen > 0 || result == GL_FALSE)
        {
            std::vector<char> errorMessages(infoLogLen + 1);
            glGetProgramInfoLog(outProgramID, infoLogLen, NULL, &errorMessages[0]);
            std::cerr << "Failed compile shader program - " << std::string((char*)&errorMessages[0]) << "\n";
            return false;
        }

        if (inDeleteShaders)
        {
            for (U32 shader : outShaders)
            {
                if (shader != 0)
                {
                    glDetachShader(outProgramID, shader);
                    glDeleteShader(shader);
                }
            }

            outShaders.clear();
        }
       
        return true;
    }

    void DeleteShaderProgram(U32 &outProgramID)
    {
        if (outProgramID != 0)
        {
            glDeleteProgram(outProgramID);
            outProgramID = 0;
        }
    }

    I32 GetUniform(const char* inUniformName, U32 inProgramID)
    {
        const I32 uniformID = glGetUniformLocation(inProgramID, inUniformName);

        if (uniformID == -1)
        {
            std::cerr << "Warning - invalid uniform ID - " << inUniformName << "\n";
        }

        return uniformID;
    }

    I32 GetCachedUniform(const char *inUniformName, U32 inProgramID, UniformCache &outCache)
    {
        const auto founded = outCache.find( inUniformName );
        I32 uniformID = -1;

        if (founded != std::end(outCache))
        {
            uniformID = founded->second;
        }
        else 
        {
            uniformID = GetUniform(inUniformName, inProgramID);
            outCache.emplace( inUniformName, uniformID );
        }

        return uniformID;
    }

	void SetVec3Array(const char* inUniformName, const std::vector< glm::vec3 >& inValues, U32 inProgramID, UniformCache& outCache)
	{
		const I32 uniformID = GetCachedUniform(inUniformName, inProgramID, outCache);
		glUniform3fv(uniformID, static_cast<I32>( inValues.size() ), &(inValues[0])[0] );
	}

	void SetVec3Array(const char* inUniformName, const std::vector< glm::vec3 >& inValues, U32 inProgramID)
	{
		const I32 uniformID = GetUniform(inUniformName, inProgramID);
		glUniform3fv(uniformID, static_cast<I32>(inValues.size()), &(inValues[0])[0]);
	}

    void SetMat4(const char * inUniformName, const glm::mat4 &inMat, U32 inProgramID, UniformCache &outCache )
    {
        const I32 uniformID = GetCachedUniform(inUniformName, inProgramID, outCache);
        glUniformMatrix4fv( uniformID, 1, GL_FALSE, &inMat[0][0] );
    }

    void SetMat4(const char * inUniformName, const glm::mat4 &inMat, U32 inProgramID)
    {
        const I32 uniformID = GetUniform(inUniformName, inProgramID);
        glUniformMatrix4fv( uniformID, 1, GL_FALSE, &inMat[0][0] );
    }

    void SetMat3(const char * inUniformName, const glm::mat3 &inMat, U32 inProgramID, UniformCache &outCache)
    {
        const I32 uniformID = GetCachedUniform(inUniformName, inProgramID, outCache);
        glUniformMatrix3fv( uniformID, 1, GL_FALSE, &inMat[0][0] );
    }

	void SetMat3(const char* inUniformName, const glm::mat3& inMat, U32 inProgramID)
	{
		const I32 uniformID = GetUniform(inUniformName, inProgramID);
		glUniformMatrix3fv(uniformID, 1, GL_FALSE, &inMat[0][0]);
	}

    void SetVec4(const char * inUniformName, const glm::vec4 &inVec, U32 inProgramID, UniformCache &outCache)
    {
        const I32 uniformID = GetCachedUniform(inUniformName, inProgramID, outCache);
        glUniform4fv(uniformID, 1, &inVec[0]);
    }

    void SetVec4(const char * inUniformName, const glm::vec4 &inVec, U32 inProgramID)
    {
        const I32 uniformID = GetUniform(inUniformName, inProgramID);
        glUniform4fv(uniformID, 1, &inVec[0]);
    }

    void SetVec3(const char * inUniformName, const glm::vec3 &inVec, U32 inProgramID, UniformCache &outCache)
    {
        const I32 uniformID = GetCachedUniform(inUniformName, inProgramID, outCache);
        glUniform3fv(uniformID, 1, &inVec[0]);
    }

    void SetVec3(const char * inUniformName, const glm::vec3 &inVec, U32 inProgramID)
    {
        const I32 uniformID = GetUniform(inUniformName, inProgramID);
        glUniform3fv(uniformID, 1, &inVec[0]);
    }

    void SetVec2(const char * inUniformName, const glm::vec2 &inVec, U32 inProgramID, UniformCache &outCache)
    {
        const I32 uniformID = GetCachedUniform(inUniformName, inProgramID, outCache);
        glUniform2fv(uniformID, 1, &inVec[0]);
    }

    void SetVec2(const char * inUniformName, const glm::vec2 &inVec, U32 inProgramID)
    {
        const I32 uniformID = GetUniform(inUniformName, inProgramID);
        glUniform2fv(uniformID, 1, &inVec[0]);
    }

    void SetDouble( const char * inUniformName, F64 inValue, U32 inProgramID, UniformCache &outCache )
    {
        const I32 uniformID = GetCachedUniform(inUniformName, inProgramID, outCache);
        glUniform1d(uniformID, inValue);
    }

    void SetDouble( const char * inUniformName, F64 inValue, U32 inProgramID)
    {
        const I32 uniformID = GetUniform( inUniformName, inProgramID );
        glUniform1d(uniformID, inValue);
    }

    void SetFloat( const char * inUniformName, F32 inValue, U32 inProgramID, UniformCache &outCache )
    {
        const I32 uniformID = GetCachedUniform(inUniformName, inProgramID, outCache);
        glUniform1f(uniformID, inValue);
    }

    void SetFloat( const char * inUniformName, F32 inValue, U32 inProgramID)
    {
        const I32 uniformID = GetUniform( inUniformName, inProgramID );
        glUniform1f(uniformID, inValue);
    }

    void SetInt( const char * inUniformName, I32 inValue, U32 inProgramID, UniformCache &outCache )
    {
        const I32 uniformID = GetCachedUniform(inUniformName, inProgramID, outCache);
        glUniform1i(uniformID, inValue);
    }

    void SetInt( const char * inUniformName, I32 inValue, U32 inProgramID )
    {
        const I32 uniformID = GetUniform(inUniformName, inProgramID);
        glUniform1i(uniformID, inValue);
    }

    void SetUInt( const char * inUniformName, U32 inValue, U32 inProgramID, UniformCache &outCache )
    {
        const I32 uniformID = GetCachedUniform(inUniformName, inProgramID, outCache);
        glUniform1ui(uniformID, inValue);
    }

    void SetUInt( const char * inUniformName, U32 inValue, U32 inProgramID )
    {
        const I32 uniformID = GetUniform(inUniformName, inProgramID);
        glUniform1ui(uniformID, inValue);
    }
}