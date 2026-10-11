//
// Created by Lenovo on 2026/10/9.
//

#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "path_tracing.h"

#include <chrono>
#include <iomanip>
#include <iostream>

static void PrintProgress(int processed, int total, std::chrono::steady_clock::time_point start)
{
    // 控制刷新频率：每1%或者每64像素才刷新，避免疯狂IO
    const int updateStep = 64;
    if (processed % updateStep != 0 && processed != total)
        return;

    float progress = static_cast<float>(processed) / total;
    auto now = std::chrono::steady_clock::now();
    double elapsedSec = std::chrono::duration<double>(now - start).count();
    double etaSec = (progress <= 0) ? 0 : elapsedSec / progress * (1 - progress);

    const int barWidth = 30;
    int filled = static_cast<int>(progress * barWidth);

    std::cout << "\rProgress: "
              << std::setw(6) << processed << " / " << std::setw(6) << total
              << " [";
    for (int i = 0; i < barWidth; i++)
    {
        std::cout << (i < filled ? "#" : " ");
    }
    std::cout << "] " << std::fixed << std::setprecision(1) << progress * 100.0f << "%"
              << " Elapsed:" << std::setw(6) << elapsedSec << "s"
              << " Remaining:" << std::setw(6) << etaSec << "s";
    std::cout.flush();
}


namespace PathTracing {

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    AABB::AABB(const Object& object) : object(&object) {
        // 获取世界空间中最小的AABB盒
        float a_x0 = FLT_MAX;
        float a_y0 = FLT_MAX;
        float a_z0 = FLT_MAX;
        float a_x1 = -FLT_MAX;
        float a_y1 = -FLT_MAX;
        float a_z1 = -FLT_MAX;
        this->object = &object;
        const glm::mat4 modelMatrix = object.modelMatrix;
        for (const auto & triangle : object.triangles) {
            glm::vec3 t0 = modelMatrix * glm::vec4(triangle.t0, 1.0f);
            glm::vec3 t1 = modelMatrix * glm::vec4(triangle.t1, 1.0f);
            glm::vec3 t2 = modelMatrix * glm::vec4(triangle.t2, 1.0f);

            float min_X = std::min(t0.x, std::min(t1.x, t2.x));
            float min_Y = std::min(t0.y, std::min(t1.y, t2.y));
            float min_Z = std::min(t0.z, std::min(t1.z, t2.z));
            float max_X = std::max(t0.x, std::max(t1.x, t2.x));
            float max_Y = std::max(t0.y, std::max(t1.y, t2.y));
            float max_Z = std::max(t0.z, std::max(t1.z, t2.z));

            a_x0 = min_X < a_x0 ? min_X : a_x0;
            a_y0 = min_Y < a_y0 ? min_Y : a_y0;
            a_z0 = min_Z < a_z0 ? min_Z : a_z0;
            a_x1 = max_X > a_x1 ? max_X : a_x1;
            a_y1 = max_Y > a_y1 ? max_Y : a_y1;
            a_z1 = max_Z > a_z1 ? max_Z : a_z1;
        }

        this->x0 = a_x0;
        this->y0 = a_y0;
        this->z0 = a_z0;
        this->x1 = a_x1;
        this->y1 = a_y1;
        this->z1 = a_z1;
    }






    void Object::GetBasis(glm::vec3& right, glm::vec3& up, glm::vec3& backward) const {
        right = glm::vec3(modelMatrix[0]);
        up = glm::vec3(modelMatrix[1]);
        backward = glm::vec3(modelMatrix[2]);
    }

    glm::vec3 Object::GetWorldPos() const {
        return glm::vec3(modelMatrix[3]);
    }

    void Camera::GetBasis(glm::vec3& right, glm::vec3& up, glm::vec3& backward) const {
        right = glm::vec3(modelMatrix[0]);
        up = glm::vec3(modelMatrix[1]);
        backward = glm::vec3(modelMatrix[2]);
    }

    glm::vec3 Camera::GetWorldPos() const {
        return glm::vec3(modelMatrix[3]);
    }


    std::vector<float>* RenderServer::PathTracing(const Camera& camera, const std::vector<AABB>& aabbs, std::vector<AreaLight>& lights) {
        auto data = new std::vector<float>();
        auto startTime = std::chrono::steady_clock::now();
        auto totalPixel = camera.width * camera.height;
        auto processed = 0;
        for ( int y = 0 ; y < camera.height; ++y) {
            for ( int x = 0 ; x < camera.width; ++x) {
                float a = 1.0f / SAMPLE_COUNT_PER_PIXEL;
                auto pixel = glm::vec3(0.0f);
                for (int i = 0; i < SAMPLE_COUNT_PER_PIXEL; ++i) {
                    auto randomX = dist(gen);
                    auto randomY = dist(gen);
                    auto ray = CastRay(camera, x, y, randomX, randomY);
                    auto hitMessage = RayTest(ray, aabbs);
                    auto result = Shade(hitMessage, aabbs, lights, 0);
                    pixel += result * a;
                }

                data->push_back(pixel.x);
                data->push_back(pixel.y);
                data->push_back(pixel.z);
                PrintProgress(processed++, totalPixel, startTime);
            }
        }
        std::cout << "\nRender Complete\n";
        return data;
    }


    /**
     *
     * @param camera
     * @param x 像素横坐标
     * @param y 像素纵坐标
     * @param randomX 范围[0, 1]的随机偏移
     * @param randomY 范围[0, 1]的随机偏移
     * @return
     */
    Ray RenderServer::CastRay(const Camera& camera, int x, int y, float randomX, float randomY) {
        float halfHeight = tanf(glm::radians(camera.fov) / 2.0f);
        float halfWidth = halfHeight * camera.aspect;
        float ndc_x = 2.0 * (x + randomX) / camera.width - 1;
        float ndc_y = -(2.0 * (y + randomY) / camera.height - 1);

        glm::vec3 front, right, up;
        camera.GetBasis(right, up, front);
        front = -front;
        glm::vec3 worldDir = glm::normalize(front + ndc_x * halfWidth * right + ndc_y * halfHeight * up);
        Ray ray;
        ray.origin = camera.GetWorldPos();
        ray.direction = worldDir;
        return ray;
    }

    Ray RenderServer::CastRay(const glm::vec3& origin, const glm::vec3& direction) {
        Ray ray;
        ray.origin = origin;
        ray.direction = direction;
        return ray;
    }


    HitMessage RenderServer::RayTest(const Ray& ray, const std::vector<AABB>& aabbs) {
        HitMessage hitMessage;
        const Object* hitObj = nullptr;
        const Triangle* hitTriangle = nullptr;
        glm::vec3 hitPos;
        glm::vec3 hitNormal;
        float min_t = FLT_MAX;


        float t = 0;
        float b1 = 0;
        float b2 = 0;
        for (const auto & aabb : aabbs) {
            if (!IsIntersectionWithAABB(ray, aabb)) continue;
            const Object* obj = aabb.object;
            if (!obj) continue;
            for (const auto & triangle : obj->triangles) {

                if (!IsIntersectionWithTriangle(ray, obj->modelMatrix, triangle, t, b1, b2)) continue;
                if (t > 0.0f && t < min_t) {
                    hitObj = obj;
                    hitTriangle = &triangle;
                    min_t = t;
                    hitPos = InterpHitPos(b1, b2, triangle, obj->modelMatrix);
                    hitNormal = InterpNormal(b1, b2, triangle, obj->normalMatrix);
                }
            }


        }

        if (hitTriangle) {

            hitMessage.object = hitObj;
            hitMessage.triangle = hitTriangle;
            hitMessage.hitPos = hitPos;
            hitMessage.hitNormal = hitNormal;
            hitMessage.ray = ray;
            hitMessage.t = min_t;
        }

        return hitMessage;
    }

    HitMessage RenderServer::RayTest(const Ray& ray, const AABB& aabb) {
        HitMessage hitMessage;
        const Object* hitObj = nullptr;
        const Triangle* hitTriangle = nullptr;
        glm::vec3 hitPos;
        glm::vec3 hitNormal;
        float min_t = FLT_MAX;


        float t = 0;
        float b1 = 0;
        float b2 = 0;

        if (IsIntersectionWithAABB(ray, aabb)) {
            const Object* obj = aabb.object;
            if (!obj) return hitMessage;
            for (const auto & triangle : obj->triangles) {

                if (!IsIntersectionWithTriangle(ray, obj->modelMatrix, triangle, t, b1, b2)) continue;
                if (t > 0.0f && t < min_t) {
                    hitObj = obj;
                    hitTriangle = &triangle;
                    min_t = t;
                    hitPos = InterpHitPos(b1, b2, triangle, obj->modelMatrix);
                    hitNormal = InterpNormal(b1, b2, triangle, obj->normalMatrix);
                }
            }

        }



        if (hitTriangle) {
            hitMessage.object = hitObj;
            hitMessage.triangle = hitTriangle;
            hitMessage.hitPos = hitPos;
            hitMessage.hitNormal = hitNormal;
            hitMessage.ray = ray;
            hitMessage.t = min_t;
        }

        return hitMessage;
    }

    HitMessage RenderServer::RayTest(const Ray& ray, const Object& object) {
        HitMessage hitMessage;
        const Object* hitObj = nullptr;
        const Triangle* hitTriangle = nullptr;
        glm::vec3 hitPos;
        glm::vec3 hitNormal;
        float min_t = FLT_MAX;


        float t = 0;
        float b1 = 0;
        float b2 = 0;

        for (const auto & triangle : object.triangles) {

            if (!IsIntersectionWithTriangle(ray, object.modelMatrix, triangle, t, b1, b2)) continue;
            if (t > 0.0f && t < min_t) {
                hitObj = &object;
                hitTriangle = &triangle;
                min_t = t;
                hitPos = InterpHitPos(b1, b2, triangle, object.modelMatrix);
                hitNormal = InterpNormal(b1, b2, triangle, object.normalMatrix);
            }
        }




        if (hitTriangle) {

            hitMessage.object = hitObj;
            hitMessage.triangle = hitTriangle;
            hitMessage.hitPos = hitPos;
            hitMessage.hitNormal = hitNormal;
            hitMessage.ray = ray;
            hitMessage.t = min_t;
        }

        return hitMessage;
    }


    glm::vec3 RenderServer::Shade(const HitMessage& hitMessage, const std::vector<AABB>& aabbs, const std::vector<AreaLight>& lights, int depth) {
        if (depth >= MAX_DEPTH) return glm::vec3(0.0f);
        if (!hitMessage.triangle) return glm::vec3(0.0f);

        const float P_RR = 0.75f; // 光线继续传播的概率
        const float kEps = 1e-4f;



        glm::vec3 L_e(0.0f);
        if (hitMessage.object && hitMessage.object->objectType == ObjectType::AreaLight) {
            L_e = hitMessage.object->GetEmission();
            return L_e;
        }

        auto L_dir = glm::vec3(0.0f);
        auto L_indir = glm::vec3(0.0f);
        for (const AreaLight& light : lights) {
            HitMessage lightHitMessage{};
            if (CanReachLight(light, hitMessage.hitPos, hitMessage.hitNormal, aabbs, lightHitMessage)) {
                float L_i = light.energy;
                glm::vec3 f_r = Get_fr(hitMessage.triangle->brdf, hitMessage.hitPos, hitMessage.hitNormal, lightHitMessage.hitPos - hitMessage.hitPos, -hitMessage.ray.direction);
                float area = light.length * light.width;
                float pdfLight = 1.0f / area;
                float cosA = glm::max(0.0f, GetCosValue(hitMessage.hitNormal, lightHitMessage.hitPos - hitMessage.hitPos));
                float cosB =  glm::max(0.0f, GetCosValue(lightHitMessage.hitNormal, hitMessage.hitPos - lightHitMessage.hitPos));
                if (cosA <= 0.0f || cosB <= 0.0f) continue;
                float distance = glm::length(hitMessage.hitPos - lightHitMessage.hitPos);
                L_dir += L_i * f_r * cosA * cosB / (distance * distance) / pdfLight;
            }
        }

        float random = dist(gen);
        if (P_RR < random) L_indir = glm::vec3(0.0f);
        else {
            glm::vec3 randomDir = SampleUniformHemisphere(hitMessage.hitNormal);
            Ray ray = CastRay(hitMessage.hitPos + kEps * hitMessage.hitNormal, randomDir);
            auto newHitMessage = RayTest(ray, aabbs); // 反射光线命中信息
            if (newHitMessage.triangle && newHitMessage.object && newHitMessage.object->objectType != ObjectType::AreaLight) {
                glm::vec3 f_r = Get_fr(hitMessage.triangle->brdf, hitMessage.hitPos, hitMessage.hitNormal, randomDir, -hitMessage.ray.direction);
                float pdf = 1.0f / (glm::pi<float>() * 2.0f);
                float cosA = glm::max(0.0f, GetCosValue(hitMessage.hitNormal, randomDir));
                L_indir = Shade(newHitMessage, aabbs, lights, depth + 1) * f_r * cosA / pdf / P_RR;
            }
        }

            return L_dir + L_indir;
    }






    bool RenderServer::IsIntersectionWithAABB(const Ray& ray, const AABB& aabb) {
        // 标准 slab test（轴对齐包围盒射线相交判定）
        float minX = std::min(aabb.x0, aabb.x1);
        float maxX = std::max(aabb.x0, aabb.x1);
        float minY = std::min(aabb.y0, aabb.y1);
        float maxY = std::max(aabb.y0, aabb.y1);
        float minZ = std::min(aabb.z0, aabb.z1);
        float maxZ = std::max(aabb.z0, aabb.z1);

        float t_min = -FLT_MAX;
        float t_max =  FLT_MAX;

        float dir[3] = {ray.direction.x, ray.direction.y, ray.direction.z};
        float ori[3] = {ray.origin.x,   ray.origin.y,   ray.origin.z  };
        float minB[3] = {minX, minY, minZ};
        float maxB[3] = {maxX, maxY, maxZ};

        for (int i = 0; i < 3; ++i) {
            if (glm::abs(dir[i]) < 1e-8f) {
                // 射线与该轴的 slab 平行：原点必须在 slab 内
                if (ori[i] < minB[i] || ori[i] > maxB[i]) return false;
            } else {
                float invD = 1.0f / dir[i];
                float t1 = (minB[i] - ori[i]) * invD;
                float t2 = (maxB[i] - ori[i]) * invD;
                float tNear = std::min(t1, t2);
                float tFar  = std::max(t1, t2);
                t_min = std::max(t_min, tNear);
                t_max = std::min(t_max, tFar);
                if (t_min > t_max) return false;
            }
        }
        return t_max > 0.0f;
    }



    bool RenderServer::IsIntersectionWithTriangle(const Ray& ray, const glm::mat4& modelMatrix,const Triangle& tri, float& t, float& b1, float& b2) {
        glm::vec3 v0 = glm::vec3(modelMatrix * glm::vec4(tri.t0, 1.0f));
        glm::vec3 v1 = glm::vec3(modelMatrix * glm::vec4(tri.t1, 1.0f));
        glm::vec3 v2 = glm::vec3(modelMatrix * glm::vec4(tri.t2, 1.0f));

        glm::vec3 E1 = v1 - v0;
        glm::vec3 E2 = v2 - v0;
        glm::vec3 S = ray.origin - v0;
        glm::vec3 S1 = glm::cross(ray.direction, E2);
        glm::vec3 S2 = glm::cross(S, E1);
        float det = glm::dot(S1, E1);
        if (glm::abs(det) < 1e-10f) return false;
        float inv_det = 1.0f / det;
        t = glm::dot(S2, E2) * inv_det;
        b1 = glm::dot(S1, S) * inv_det;
        b2 = glm::dot(S2, ray.direction) * inv_det;

        return t > 0.0f && b1 > 0.0f && b2 > 0.0f && (1.0f - b1 - b2) > 0.0f;
    }




    glm::vec3 RenderServer::InterpNormal(float b1, float b2,
                                   const Triangle& tri,
                                   const glm::mat3& normalMatrix) {
        float w = 1.0f - b1 - b2;

        // b1 -> n1, b2 -> n2, w -> n0
        glm::vec3 normal = b1 * tri.n1 + b2 * tri.n2 + w * tri.n0;
        normal = normalMatrix * normal;
        return glm::normalize(normal);
    }



    glm::vec3 RenderServer::InterpHitPos(float b1, float b2,
                                       const Triangle& tri,
                                       const glm::mat4& modelMatrix) {
        float w = 1.0f - b1 - b2;

        // b1 -> t1, b2 -> t2, w -> t0
        glm::vec3 localPos = b1 * tri.t1 + b2 * tri.t2 + w * tri.t0;
        glm::vec3 worldPos = glm::vec3(modelMatrix * glm::vec4(localPos, 1.0f));
        return worldPos;
    }



    /**
     *
     * @param brdf
     * @param hitPos
     * @param hitNormal
     * @param out 入射方向 从表面交点指向光源
     * @param in 出射方向 从表面交点指向相机/观测点
     * @return
     */
    glm::vec3 RenderServer::Get_fr(const BRDF& brdf, const glm::vec3& hitPos, const glm::vec3& hitNormal,glm::vec3 out, glm::vec3 in) {
        out = glm::normalize(out);
        in = glm::normalize(in);

        float kd = 1.0f - brdf.metallicity;
        glm::vec3 f_Lambert = brdf.albedo / glm::pi<glm::vec3>();
        glm::vec3 f_CookTorrance{};

        auto N = hitNormal; // N 表面法线
        auto V = out; // V 交点→相机 out
        auto L = in; // L 交点→光源 in
        auto H = glm::normalize(L + V); // H 半向量
        float D = BRDF_D_GGX(glm::dot(N, H), brdf.roughness);
        glm::vec3 F = BRDF_F_Fresnel_Schlick(glm::dot(V, H), brdf.albedo);
        float G = BRDF_G_Smith(glm::dot(N, V), glm::dot(N, L), brdf.roughness);

        f_CookTorrance = D * F * G / (4 * (glm::dot(V, N) * (glm::dot(L, N))));

        return kd * f_Lambert + f_CookTorrance;
    }

    /**
     * 判断物体与光源之间有没有遮挡 返回本次命中信息
     * @param light
     * @param hitPos
     * @param aabbs
     * @param returnHitMessage
     * @return
     */
    bool RenderServer::CanReachLight(const AreaLight& light, const glm::vec3& hitPos, const glm::vec3& hitNormal, const std::vector<AABB>& aabbs, HitMessage& returnHitMessage) {
        const float kEps = 1e-4f;
        glm::vec3 dir = glm::normalize(light.GetWorldPos() - hitPos);
        Ray ray = CastRay(hitPos + kEps * hitNormal, dir);
        auto hitMessage = RayTest(ray, aabbs);
        auto lightHitMessage = RayTest(ray, light);
        if (glm::abs(hitMessage.t - lightHitMessage.t) <= 0.01f) {
            returnHitMessage = lightHitMessage;
            return true;
        }
        return false;
    }


    /**
     * 余弦加权半球采样
     * @param n
     * @return
     */
    glm::vec3 RenderServer::SampleCosineHemisphere(const glm::vec3& n) {
        float r1 = dist(gen);
        float r2 = dist(gen);

        float phi = 2.0f * glm::pi<float>() * r1;
        float cosTheta = std::sqrt(1.0f - r2);
        float sinTheta = std::sqrt(r2);


        glm::vec3 localDir(sinTheta * std::cos(phi),
                           sinTheta * std::sin(phi),
                           cosTheta);


        glm::vec3 w = glm::normalize(n);
        glm::vec3 a = (std::abs(w.x) > 0.9f) ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0);
        glm::vec3 u = glm::normalize(glm::cross(a, w));
        glm::vec3 v = glm::cross(w, u);

        return glm::normalize(localDir.x * u + localDir.y * v + localDir.z * w);
    }

    /**
     * 均匀半球采样
     * @param n
     * @return
     */
    glm::vec3 RenderServer::SampleUniformHemisphere(const glm::vec3& n) {
        float r1 = dist(gen);
        float r2 = dist(gen);

        float phi = 2.0f * glm::pi<float>() * r1;
        float cosTheta = r2;
        float sinTheta = std::sqrt(1.0f - r2 * r2);


        glm::vec3 localDir(sinTheta * std::cos(phi),
                           sinTheta * std::sin(phi),
                           cosTheta);


        glm::vec3 w = glm::normalize(n);
        glm::vec3 a = (std::abs(w.x) > 0.9f) ? glm::vec3(0.0f, 1.0f, 0.0f)
                                             : glm::vec3(1.0f, 0.0f, 0.0f);
        glm::vec3 u = glm::normalize(glm::cross(a, w));
        glm::vec3 v = glm::cross(w, u);


        return glm::normalize(localDir.x * u + localDir.y * v + localDir.z * w);
    }


}
