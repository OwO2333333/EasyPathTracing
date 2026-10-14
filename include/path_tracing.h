//
// Created by Lenovo on 2026/10/8.
//

#ifndef PATHTRACING_PATH_TRACING_H
#define PATHTRACING_PATH_TRACING_H
#include <algorithm>
#include <chrono>
#include <memory>
#include <random>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <atomic>
#include <iostream>

namespace PathTracing {
    class Triangle;
    class Object;
    class ModelServer;

    enum class PrintType {
        HDR,
        PNG,
    };

    static void PrintProgress(int processed, int total, std::chrono::steady_clock::time_point start);
    void PrintPicture(char const* filename, int w, int h, int comp, const void* data, int stride_in_bytes, PrintType type);
    void PrintClock(double seconds);
    void PrintRenderProgress(int doneRows, int totalRows, const std::vector<std::atomic<int>>& threadRows, const std::vector<int>& endRowPerThread, std::chrono::steady_clock::time_point start);


    struct Pixel {
        glm::vec3 rgb;
    };

    struct BRDF {
        glm::vec3 albedo;
        float roughness;
        float metallicity;
    };


    struct Ray {
        glm::vec3 origin{};
        glm::vec3 direction{};
    };

    struct HitMessage {
        const Object* object = nullptr;
        const Triangle* triangle = nullptr;
        glm::vec3 hitPos = glm::vec3(FLT_MAX);
        glm::vec3 hitNormal{};
        Ray ray{};
        float t{};
    };

    struct HemisphereSampleResult {
        glm::vec3 randomDir = glm::vec3(1.0f);
        float pdf = 1.0f;
    };


    enum class ObjectType
    {
        TriangleMesh,
        AreaLight,
        // Sphere
    };


    class AABB {
        public:
        float x0 = FLT_MAX;
        float y0 = FLT_MAX;
        float z0 = FLT_MAX;
        float x1 = FLT_MIN;
        float y1 = FLT_MIN;
        float z1 = FLT_MIN;
        const Object* object = nullptr;

        AABB(const Object& object);

    };

    class Triangle {
        public:
        // Obj局部空间数据
        glm::vec3 t0, t1, t2;
        glm::vec3 n0, n1, n2;
        BRDF brdf;
    };


    class Object {
        public:
        ObjectType objectType{ObjectType::TriangleMesh};
        std::vector<Triangle> triangles{};
        glm::mat4 modelMatrix{};
        glm::mat4 invModelMatrix{};
        glm::mat3 normalMatrix{};

        void GetBasis(glm::vec3& right, glm::vec3& up, glm::vec3& backward) const;
        glm::vec3 GetWorldPos() const;
        virtual ~Object() = default;
        virtual glm::vec3 GetEmission() const { return glm::vec3(0.0f); }
    };


    class AreaLight : public Object {
    public:
        glm::vec3 emission{glm::vec3(1.0f)};   // 发光面辐亮度 L_e

        explicit AreaLight(const glm::vec3& emission = glm::vec3(1.0f)) : emission(emission) {
            objectType = ObjectType::AreaLight;
            BuildQuad(); // 局部 XZ 平面上的单位方形，法线朝下
        }

        glm::vec3 GetEmissiveNormal() const;                       // 世界空间发光面法线
        float     GetWorldArea() const;                            // 世界空间面积
        glm::vec3 GetPointOnEmissiveFace(float u, float v) const;  // 均匀采样一点

        glm::vec3 GetEmission() const override { return emission; }

    private:
        void BuildQuad();
    };

    class Camera {
        public:
        glm::mat4 modelMatrix = glm::mat4(1.0f);
        int width = 256;
        int height = 256;
        float fov = 40;
        float aspect = static_cast<float>(width) / height;
        float near = 0.01f;
        float far = 1000.0f;
        Camera(const glm::mat4 &modelMatrix, const int width, const int height, const float fov, const float near, const float far)
            : modelMatrix(modelMatrix), width(width), height(height), fov(fov), near(near), far(far) {
            if (height <= 0 && width <= 0) {
                std::cout << "ERROR: width width数据不合法" << std::endl;
                this->width = 256;
                this->height = 256;
            }
            this->aspect = static_cast<float>(width) / height;
        }

        void GetBasis(glm::vec3& right, glm::vec3& up, glm::vec3& backward) const;
        glm::vec3 GetWorldPos() const;
    };

    class Scene {
        public:
        std::vector<std::unique_ptr<Object>> objects;
        std::vector<AABB> aabbs;
        std::vector<AreaLight> lights; // 注意这里light在aabbs和lights是两份独立数据
    };

    class RenderServer {
        public:
        RenderServer(int maxDepth, int spp, float p_rr);

        const int MAX_DEPTH;
        const int SPP;
        const float P_RR;
        static constexpr float kRayEpsilon = 1e-4f;
        static void PathTracing(const Camera& camera,
                                const std::vector<AABB>& aabbs,
                                const std::vector<AreaLight>& lights,
                                std::string fileName,
                                PrintType printType,
                                unsigned int threadsAllowed,
                                int maxDepth,
                                float p_rr,
                                int spp);


        private:
        std::vector<float>* Tracing(const Camera& camera, const std::vector<AABB>& aabbs, const std::vector<AreaLight>& lights);
        std::vector<float>* Tracing(const Camera& camera, const std::vector<AABB>& aabbs, const std::vector<AreaLight>& lights, int startY, int endY);
        static Ray CastRay(const Camera& camera, int x, int y, float randomX, float randomY);
        static HitMessage RayTest(const Ray& ray, const std::vector<AABB>& aabbs);
        static HitMessage RayTest(const Ray& ray, const AABB& aabb);
        static HitMessage RayTest(const Ray& ray, const Object& object);
        glm::vec3 Shade(const HitMessage& hitMessage, const std::vector<AABB>& aabbs, const std::vector<AreaLight>& lights, int depth);
        static Ray CastRay(const glm::vec3& origin, const glm::vec3& direction);
        static bool IsIntersectionWithAABB(const Ray& ray, const AABB& aabb);
        static bool IsIntersectionWithTriangle(const Ray& ray, const glm::mat4& modelMatrix, const Triangle& tri, float& t, float& b1, float& b2);
        static glm::vec3 InterpNormal(float b1, float b2, const Triangle& tri, const glm::mat3& normalMatrix);
        static glm::vec3 InterpHitPos(float b1, float b2, const Triangle& tri, const glm::mat4& modelMatrix);
        static glm::vec3 Get_fr(const BRDF& brdf, const glm::vec3& hitPos, const glm::vec3& hitNormal,glm::vec3 out, glm::vec3 in);
        static bool IsOccluded ( const Ray& ray, const std::vector<AABB>& aabbs, float maxDistance);
        static HemisphereSampleResult SampleCosineHemisphere(const glm::vec3& n);
        static HemisphereSampleResult SampleUniformHemisphere(const glm::vec3& n);


        static float GetCosValue(const glm::vec3& a, const glm::vec3& b) {
            return glm::dot( glm::normalize(a), glm::normalize(b));
        }
        static float BRDF_D_GGX(float NoH, float roughness) {
            float a2 = roughness * roughness;
            float f = (NoH * NoH) * (a2 - 1.0f) + 1.0f;
            return a2 / (glm::pi<float>() * f * f);
        }
        static float BRDF_G_Smith(float NdotV, float NdotL, float roughness) {
            float k = glm::pow(roughness + 1.0f, 2.0f) / 8.0f;
            return BRDF_GGX(NdotL, k) * BRDF_GGX(NdotV, k);
        }
        static glm::vec3 BRDF_F_Fresnel_Schlick(float VoH, const glm::vec3& f0) {
            float f = glm::pow(1.0f - VoH, 5.0f);
            return f0 + (1.0f - f0) * f;
        }
        static float BRDF_GGX(float NdotV, float k) {
            return NdotV / (NdotV * (1.0f - k) + k);
        }


    };

    class ModelServer {
        public:
            static std::unique_ptr<Scene> GetCornellBox();
            static std::unique_ptr<Object> GetDefaultCube();
            static std::unique_ptr<std::vector<Triangle>> GetAreaLightSquad();
    };







}

#endif //PATHTRACING_PATH_TRACING_H
