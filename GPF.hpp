
#pragma once

#ifndef _GPF_HPP_
#define _GPF_HPP_

#include <vector>
#include <unordered_map>
#include <string>
#include <chrono>
#include <iostream>
#include <fstream>
#include <sstream>
#include <limits>
#include <cstdint>
#include <cassert>

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <GLFW/glfw3.h>

// Include GLM
#ifndef _WIN32
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-register"
#endif //_WIN32

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#ifndef _WIN32
#pragma clang diagnostic pop
#endif //_WIN32

// data types
using I8 = std::int8_t;
using I16 = std::int16_t;
using I32 = std::int32_t;
using I64 = std::int64_t;

using U8 = std::uint8_t;
using U16 = std::uint16_t;
using U32 = std::uint32_t;
using U64 = std::uint64_t;

using F32 = float;
using F64 = double;

namespace GPF
{
    /* Enums */

    enum class BufferType : U32
	{
		Array,
		Index, 
		CopyRead,
		CopyWrite,
		PixelUnpack,
		PixelPack,
		//Query,
		Texture,
		TransformFeedback,
		Uniform,
		DrawIndirect,
		AtomicCounter,
		DispatchIndirect,
		ShaderStorage
	};

	enum class BufferUsage
	{
		StreamDraw,
		StreamRead,
		StreamCopy,

		StaticDraw,
		StaticRead,
		StaticCopy,

		DynamicDraw,
		DynamicRead,
		DynamicCopy
	};

    /* Types */

    struct Image
    {
        unsigned char * Data;
        I32 Width;
        I32 Height;
        I32 Components;
    };
    
    struct AABB
    {
        glm::vec3 Min = glm::vec3( std::numeric_limits<F32>::max() );
        glm::vec3 Max = glm::vec3( -std::numeric_limits<F32>::max() );
        glm::vec3 Dimensions = glm::vec3(0.0f);
        glm::vec3 Center = glm::vec3(0.0f);
    };

    struct Vertex1P1UV
    {
        glm::vec3 Position;
        glm::vec2 TexCoord;
    };

	struct Vertex1P1N1UV
	{
		glm::vec3 Position;
		glm::vec3 Normal;
		glm::vec2 TexCoord;

		bool operator==(const Vertex1P1N1UV& inOther) const
		{
			return Position == inOther.Position &&
				Normal == inOther.Normal &&
				TexCoord == inOther.TexCoord;
		}
	};

    struct Vertex1P1N1UV1T1BT
    {
        glm::vec3 Position;
        glm::vec3 Normal;
        glm::vec2 TexCoord;
		glm::vec3 Tangent;
		glm::vec3 Bitangent;

        bool operator==(const Vertex1P1N1UV1T1BT& inOther) const
        {
            return Position == inOther.Position && 
                   Normal == inOther.Normal &&
                   TexCoord == inOther.TexCoord &&
				   Tangent == inOther.Tangent &&
				   Bitangent == inOther.Bitangent;
        }
    };

    // TODO: add internal texture cache and load materials 

    struct MaterialInfo
    {
        std::string Name;

        std::string AmbientTextureName;
        std::string DiffuseTextureName;
        std::string SpecularTextureName;
        std::string HighlightTextureName;
        std::string BumpTextureName;
        std::string DisplacementTextureName;
        std::string AlphaTextureName;
        std::string ReflectionTextureName;
    };

    inline bool operator==(const MaterialInfo &inLhs, const MaterialInfo &inRhs)
    {
        return inLhs.Name == inRhs.Name;
    }

    struct Geometry
    {
        std::vector< Vertex1P1N1UV1T1BT > Vertices_1P1N1UV1T1BT;
		std::vector< Vertex1P1N1UV > Vertices_1P1N1UV;
        std::vector< Vertex1P1UV > Vertices_1P1UV;
        std::vector< U32 > Indices;

        std::unordered_map< std::string, MaterialInfo > Materials;
        AABB Bounds;

        U32 VAO;
        U32 VBO;
        U32 IBO;

        U32 IndexCount;
        U32 VertexCount;
    };

    struct SamplerParameters
    {
        F32 MaxAnisotropy = 1.0f;

        GLenum MinFilter = GL_NEAREST;
        GLenum MagFilter = GL_NEAREST;
        GLenum WrapS = GL_REPEAT;
        GLenum WrapT = GL_REPEAT;
    };

    /* Profiling */

    static struct ProfilerManager
    {
        std::unordered_map< const char*, F64 > ProfilingMap;

        void Store( const char *inTag, F64 inTime );
        void DumpToConsole();
        void DumpToCSV(const std::string &inFileName);

    } g_Profiler;

    struct Profile
    {
        enum ProfilingType 
        { 
            CPU, GPU 
        } Type;

        const char *Tag;
        U32 Query; 
        F64 StartTime;
        bool IsEnded;

        explicit Profile( const char *inTag, ProfilingType inType );
        ~Profile();

        void EndProfiling();
    };

    /* Converters */

    static constexpr GLenum BufferToGlBuffer(BufferType inBuffer);
    static constexpr GLenum BufferUsageToGlBufferUsage(BufferUsage inUsage);

    template<typename T>
    static const GLenum ElementToGL()
    {
        if (std::is_same<float, typename std::remove_cv<T>::type>())
        {
            return GL_FLOAT;
        }

        if (std::is_same<double, typename std::remove_cv<T>::type>())
        {
            return GL_DOUBLE;
        }

        if (std::is_same<unsigned int, typename std::remove_cv<T>::type>())
        {
            return GL_UNSIGNED_INT;
        }

        if (std::is_same<unsigned char, typename std::remove_cv<T>::type>())
        {
            return GL_UNSIGNED_BYTE;
        }

        return GL_FLOAT;
    }

    /* IO */

    bool LoadFile(const std::string &inFileName, std::string &outData);

    /* Images */

    bool LoadImage(const std::string &inFileName, Image &outImage);
    void FreeImage(Image &outImage);

    /* Models */

    bool LoadOBJ(const std::string &inFileName, Geometry &outGeometry);

    /* Primitives */

    Geometry Primitive_Plane();
    Geometry Primitive_ScreenQuad();

    /* Camera */

    struct Camera
    {
        glm::vec3 Position;
        glm::vec3 Front = glm::vec3(0.0f, 0.0f, -1.0f);
        glm::vec3 Right;
        glm::vec3 Up = glm::vec3(0.0f, 1.0f, 0.0f);
        glm::vec3 WorldUp = glm::vec3( 0.0f, 1.0f, 0.0f );

        F32 LastX;
        F32 LastY;
        F32 Yaw;
        F32 mRoll;
        F32 Pitch;

        F32 MouseSpeed = 0.6f;
        F32 MovementSpeed = 6.0f;

        F32 Fov;
        F32 Aspect;
        F32 Near;
        F32 Far;
    };

    glm::mat4 GetProjectionMatrix(const Camera &inCamera);
    glm::mat4 GetViewMatrix(Camera &outCamera);
    void MoveForward(Camera &outCamera, F64 inDt);
    void MoveBackward(Camera &outCamera, F64 inDt);
    void MoveLeft(Camera &outCamera, F64 inDt);
    void MoveRight(Camera &outCamera, F64 inDt);
    void Rotate(Camera &outCamera, F64 inDeltaVertical, F64 inDeltaHorizontal);

    /* Vertex Array Object */

    U32 GenerateVAO();

    void BindVAO(U32 inID);

    template<typename T>
    static inline U32 ElementLayout(
        U32 inSlot,
		U32 inCount,
		U32 inVertexSize,
		U32 inOffset,
        bool inIsNormalized = false,
        bool inUseInts = false)
    {
        const auto normalized = inIsNormalized ? GL_TRUE : GL_FALSE;
        const auto type = ElementToGL<T>();
		const GLvoid* offset = static_cast<const char*>(0) + inOffset;

        glEnableVertexAttribArray(inSlot);

        if (inUseInts)
        {
            glVertexAttribIPointer(inSlot, inCount, type, inVertexSize, offset);
        }
        else
        {
            glVertexAttribPointer(inSlot, inCount, type, normalized, inVertexSize, offset);
        }
        
        const auto newOffset = inOffset + (static_cast<U32>(sizeof(T) * inCount));

        return newOffset;
    }

    void DeleteVAO(U32& outVAO);

    /* Buffer Object */

    U32 GenerateBuffer(BufferType inType);

    void BindBuffer(BufferType inType, U32 inID);

    void UploadData(BufferType inType, BufferUsage inUsage, const void* inData, size_t inSize);

    template<typename T>
    static inline void UploadData(BufferType inType, BufferUsage inUsage, const std::vector<T> &inData)
    {
        const auto size = sizeof(T) * inData.size();
        const auto data = inData.data();

        UploadData( inType, inUsage, data, size);
    }

    void UploadDataImmutable(BufferType inType, const void* inData, size_t inSize);

    template<typename T>
    static inline void UploadDataImmutable(BufferType inType, const std::vector<T>& inData)
    {
        #ifdef __APPLE__
            UploadData<T>(inType, BufferUsage::StaticDraw, inData);
        #else 
            const auto size = sizeof(T) * inData.size();
            const auto data = inData.data();

            UploadDataImmutable( inType, data, size );
        #endif
    }

    void InvalidateBuffer(U32 inBufferID);

    void DeleteBuffer(U32& outBuffer);

    /* Textures */

    U32 GenerateTexture();

    void UploadTextureData( const Image &inImage, U32 &outTextureID, bool inGenerateMipMaps = false);

    void UploadTextureData( const void * inData,
                            U32 inWidth, 
                            U32 inHeight, 
                            U32 &outTextureID, 
                            GLenum inType = GL_UNSIGNED_BYTE,
                            GLenum inInternalFormat = GL_RGB, 
                            GLenum inFormat = GL_RGB );

	template<typename T>
	static inline void UploadTextureData(const void* inData,
		U32 inWidth,
		U32 inHeight,
		U32& outTextureID,
		GLenum inInternalFormat = GL_RGB,
		GLenum inFormat = GL_RGB )
	{
		const auto type = ElementToGL<T>();

		glBindTexture(GL_TEXTURE_2D, outTextureID);
		glTexImage2D(GL_TEXTURE_2D, 0, inInternalFormat, inWidth, inHeight, 0, inFormat, type, inData);
	}

    void DeleteTexture(U32 &outID);

    /* Samplers */

    U32 GenerateSampler();

    void SetupSamplerParameters(const SamplerParameters &inParameters, U32 inSamplerID);

    void BindSampler( U32 inSamplerID, U32 inTextureUnit = 0 );

    void UnbindSampler( U32 inTextureUnit = 0 );

    void DeleteSampler(U32 &outSampler);

    /* Framebuffers */

	static inline void BlitFramebuffers(U32 inBufferFrom, U32 inBufferTo, U32 inWidth, U32 inHeight)
	{
		glBindFramebuffer(GL_READ_FRAMEBUFFER, inBufferFrom);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, inBufferTo);
		glBlitFramebuffer(0, 0, inWidth, inHeight, 0, 0, inWidth, inHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST);
	}

    static inline U32 GenerateFramebuffer()
    {
        U32 bufferID = 0;
        glGenFramebuffers(1, &bufferID);
        glBindFramebuffer(GL_FRAMEBUFFER, bufferID);
        return bufferID;
    }

    static inline U32 GenerateRenderbuffer(U32 inWidth, U32 inHeight, GLenum inComponents = GL_DEPTH_COMPONENT)
    {
        U32 bufferID = 0;
        glGenRenderbuffers(1, &bufferID);
        glBindRenderbuffer(GL_RENDERBUFFER, bufferID);
        glRenderbufferStorage(GL_RENDERBUFFER, inComponents, inWidth, inHeight);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, bufferID);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            std::cerr << "Framebuffer not complete!\n";
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        return bufferID;
    }

    template<typename T>
    static inline U32 GenerateRenderTarget_Color(
        U32 inIndex,
        U32 inWidth,
        U32 inHeight,
        GLenum inInternalFormat = GL_RGB16F, 
        GLenum inFormat = GL_RGB,
        GLenum inMinFilter = GL_NEAREST,
        GLenum inMagFilter = GL_NEAREST,
		GLenum inWrapS = GL_REPEAT,
		GLenum inWrapT = GL_REPEAT)
    {
        const auto type = ElementToGL<T>();

        U32 textureID = 0;

        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, inInternalFormat, inWidth, inHeight, 0, inFormat, type, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, inMinFilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, inMagFilter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, inWrapS);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, inWrapT);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + inIndex, GL_TEXTURE_2D, textureID, 0);

        return textureID;
    }

	template<typename T>
	static inline U32 GenerateRenderTarget_Depth(
		U32 inWidth,
		U32 inHeight,
		GLenum inMinFilter = GL_NEAREST,
		GLenum inMagFilter = GL_NEAREST,
		GLenum inWrapS = GL_REPEAT,
		GLenum inWrapT = GL_REPEAT)
	{
		const auto type = ElementToGL<T>();

		U32 textureID = 0;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, inWidth, inHeight, 0, GL_DEPTH_COMPONENT, type, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, inMinFilter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, inMagFilter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, inWrapS);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, inWrapT);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, textureID, 0);

		return textureID;
	}

    static inline void DeleteFramebuffer(U32 &outFramebufferID)
    {
        if (outFramebufferID != 0)
        {
            glDeleteFramebuffers(1, &outFramebufferID);
            outFramebufferID = 0;
        }
    }

    /* Shaders */

    using ShaderList = std::vector< U32 >;
    using UniformCache = std::unordered_map< const char*, I32 >;

    bool LoadShader(const std::string &inFileName, GLenum inType, ShaderList &outList);

    bool CompileShaderList( ShaderList &outShaders, U32 &outProgramID, bool inDeleteShaders = true );

    void DeleteShaderProgram(U32 &outProgramID);

    I32 GetUniform(const char* inUniformName, U32 inProgramID);

    I32 GetCachedUniform(const char *inUniformName, U32 inProgramID, UniformCache &outCache);

	void SetVec3Array(const char* inUniformName, const std::vector< glm::vec3 >& inValues, U32 inProgrmID, UniformCache& outCache);
	void SetVec3Array(const char* inUniformName, const std::vector< glm::vec3 >& inValues, U32 inProgrmID);

    void SetMat4(const char * inUniformName, const glm::mat4 &inMat, U32 inProgramID, UniformCache &outCache );
    void SetMat4(const char * inUniformName, const glm::mat4 &inMat, U32 inProgramID);

    void SetMat3(const char * inUniformName, const glm::mat3 &inMat, U32 inProgramID, UniformCache &outCache);
	void SetMat3(const char* inUniformName, const glm::mat3& inMat, U32 inProgramID);

    void SetVec4(const char * inUniformName, const glm::vec4 &inVec, U32 inProgramID, UniformCache &outCache);
    void SetVec4(const char * inUniformName, const glm::vec4 &inVec, U32 inProgramID);

    void SetVec3(const char * inUniformName, const glm::vec3 &inVec, U32 inProgramID, UniformCache &outCache);
    void SetVec3(const char * inUniformName, const glm::vec3 &inVec, U32 inProgramID);

    void SetVec2(const char * inUniformName, const glm::vec2 &inVec, U32 inProgramID, UniformCache &outCache);
    void SetVec2(const char * inUniformName, const glm::vec2 &inVec, U32 inProgramID);

    void SetDouble( const char * inUniformName, F64 inValue, U32 inProgramID, UniformCache &outCache );
    void SetDouble( const char * inUniformName, F64 inValue, U32 inProgramID);

    void SetFloat( const char * inUniformName, F32 inValue, U32 inProgramID, UniformCache &outCache );
    void SetFloat( const char * inUniformName, F32 inValue, U32 inProgramID);

    void SetInt( const char * inUniformName, I32 inValue, U32 inProgramID, UniformCache &outCache );
    void SetInt( const char * inUniformName, I32 inValue, U32 inProgramID );

    void SetUInt( const char * inUniformName, U32 inValue, U32 inProgramID, UniformCache &outCache );
    void SetUInt( const char * inUniformName, U32 inValue, U32 inProgramID );
}

#endif //_GPF_HPP_