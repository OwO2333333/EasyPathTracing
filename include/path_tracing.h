//
// Created by Lenovo on 2026/10/8.
//

#ifndef PATHTRACING_PATH_TRACING_H
#define PATHTRACING_PATH_TRACING_H
#include <memory>
#include <random>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace PathTracing {
    class Triangle;
    class Object;


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
        glm::vec3 hitPos{};
        glm::vec3 hitNormal{};
        Ray ray{};
        float t{};
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
        float energy;
        float length;
        float width;

        AreaLight(const float& energy, const float& length, const float& width) : energy(energy),
            length(length), width(width) {
            objectType = ObjectType::AreaLight;
        }

        glm::vec3 GetEmission() const override {
            return glm::vec3(energy);
        }

    };

    class Camera {
        public:
        glm::mat4 modelMatrix;
        int width, height;
        float fov;
        float aspect;
        float near;
        float far;

        void GetBasis(glm::vec3& right, glm::vec3& up, glm::vec3& backward) const;

        glm::vec3 GetWorldPos() const;

    };

    class RenderServer {
        public:
        RenderServer(int maxDepth, int sampleCountPerPixel, float p_rr);
        RenderServer() = default;

        const int MAX_DEPTH = 12;
        const int SAMPLE_COUNT_PER_PIXEL = 1024;
        const float P_RR = 0.75f;
        std::vector<float>* PathTracing(const Camera& camera, const std::vector<AABB>& aabbs, std::vector<AreaLight>& lights);

        private:
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
        static bool CanReachLight(const AreaLight& light, const glm::vec3& hitPos, const glm::vec3& hitNormal, const std::vector<AABB>& aabbs, HitMessage& returnHitMessage);
        static glm::vec3 SampleCosineHemisphere(const glm::vec3& n);
        static glm::vec3 SampleUniformHemisphere(const glm::vec3& n);


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
            static std::unique_ptr<Object> GetDefaultCube();



    };







}

#endif //PATHTRACING_PATH_TRACING_H
