//
// Created by Lenovo on 2026/10/9.
//

#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "path_tracing.h"

#include <atomic>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <thread>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"


namespace PathTracing {
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


    void PrintPicture(char const* filename, int w, int h, int comp, const void* data, int stride_in_bytes, PrintType type)
    {
        int ok = 0;
        if (type == PrintType::PNG)
        {
            // PNG: data 是 uint8_t 字节数组，stride = 每行字节数
            ok = stbi_write_png(
                filename,
                w,
                h,
                comp,
                data,
                stride_in_bytes
            );
        }
        else if (type == PrintType::HDR)
        {
            // HDR: data 是 float数组（线性HDR，每个通道float）
            ok = stbi_write_hdr(
                filename,
                w,
                h,
                comp,
                static_cast<const float*>(data)
            );
        }

        if (ok)
        {
            std::cout << "Saved image: " << filename << "\n";
        }
        else
        {
            std::cerr << "Failed to save image: " << filename << "\n";
        }
    }

    // 多线程测试专用的进度输出（只做统计与打印，不参与渲染模块）
    // 把秒数打成 HH:MM:SS
    void PrintClock(double seconds)
    {
        if (seconds < 0.0)
        {
            std::cout << "--:--:--";
            return;
        }
        const long long t = static_cast<long long>(seconds);
        std::cout << std::setfill('0') << std::setw(2) << (t / 3600) << ':'
                  << std::setw(2) << ((t % 3600) / 60) << ':'
                  << std::setw(2) << (t % 60) << std::setfill(' ');
    }

    // 每次打印一行带时间戳的日志：即使 doneRows 长时间不变，时间和 threadRow 也能看出在运行
    void PrintRenderProgress(int doneRows, int totalRows,
                                    const std::vector<std::atomic<int>>& threadRows,
                                    const std::vector<int>& endRowPerThread,
                                    std::chrono::steady_clock::time_point start)
    {
        const double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
        const double rate = (doneRows > 0 && elapsed > 0.0) ? doneRows / elapsed : 0.0;   // 行/秒
        const double eta = (rate > 0.0) ? (totalRows - doneRows) / rate : 0.0;

        const std::time_t now = std::time(nullptr);
        char ts[16] = {};
        std::strftime(ts, sizeof(ts), "%H:%M:%S", std::localtime(&now));

        std::cout << "[" << ts << "]"
                  << " "
                  << std::fixed << std::setprecision(1)<< (totalRows > 0 ? 100.0 * doneRows / totalRows : 0.0) << "%("
                  << doneRows << "/" << totalRows << "rows)"
                  << "  "
                  << std::setprecision(2) << rate * 60.0 << " rows/min"
                  << "  "
                  << "elapsed:" << std::setprecision(1) << elapsed << "s"
                  << "  "
                  << "eta" << " ";
                  PrintClock(eta);
        std::cout << std::endl;
        std::cout << "threadRow:";
        for (size_t t = 0; t < threadRows.size(); ++t)
        {
            const int row = threadRows[t].load(std::memory_order_relaxed);
            std::cout << " ";
            if (row < 0) std::cout << "-"; else std::cout << row;
            if (row < endRowPerThread[t]) {
                std::cout << "->" << endRowPerThread[t];
            }
        }
        std::cout << std::endl;
    }






    thread_local std::mt19937 gen{ std::random_device{}() };
    thread_local std::uniform_real_distribution<float> dist ( 0.0f , 1.0f );

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


    void AreaLight::BuildQuad() {
        const auto tri = ModelServer::GetAreaLightSquad();
        triangles = *tri;
    }

    glm::vec3 AreaLight::GetEmissiveNormal() const {
        return glm::normalize(normalMatrix * glm::vec3(0.0f, -1.0f, 0.0f));
    }

    float AreaLight::GetWorldArea() const {
        glm::vec3 right, up, backward;
        GetBasis(right, up, backward);                      // right=col0, up=col1, backward=col2
        return glm::length(glm::cross(right, backward));    // = 1.30 × 1.05
    }

    glm::vec3 AreaLight::GetPointOnEmissiveFace(float u, float v) const {
        glm::vec3 right, up, backward;
        GetBasis(right, up, backward);
        // 局部方形跨 ±0.5，modelMatrix 的列已含缩放 → 直接乘 (u-0.5) 就是世界空间半宽
        return GetWorldPos() + right * (u - 0.5f) + backward * (v - 0.5f);
        // 变成 quad 后不再需要 -up*0.5 那种"移到盒底"的偏移
    }



    RenderServer::RenderServer(int maxDepth, int spp, float p_rr) : MAX_DEPTH(maxDepth), SPP(spp), P_RR(p_rr) {

    }

    void RenderServer::PathTracing(const Camera& camera,
        const std::vector<AABB>& aabbs,
        const std::vector<AreaLight>& lights,
        std::string fileName,
        PrintType printType,
        unsigned int maxThreadsAllowed
        ) {
        switch (printType) {
            case PrintType::HDR:
                fileName += ".hdr";
                break;
            case PrintType::PNG:
                fileName += ".png";
        }
        const int w = camera.width;
        const int h = camera.height;
        const int comp = 3;

        // 线程数：硬件并发，至少 1
        const int threadCount = static_cast<int>(std::max(1u, std::min(maxThreadsAllowed, std::thread::hardware_concurrency())));
        const int rowsPerThread = (h + threadCount - 1) / threadCount; // 向上取整

        // 每个线程的结果缓冲
        std::vector<std::unique_ptr<std::vector<float>>> partials(threadCount);

        // 进度统计（只有主线程读、各线程写，用 atomic 汇总）
        std::atomic<int> doneRows{0}; // 完成的行数
        std::atomic<int> finishedThreads{0}; // 完成的线程数
        std::vector<std::atomic<int>> threadRows(threadCount); // 各线程当前渲染到第几行（-1 = 未开始）
        std::vector<int> endRowPerThread(threadCount); // 每个线程需要计算到的最终行数
        for (auto& row : threadRows) row.store(-1, std::memory_order_relaxed);
        const auto renderStart = std::chrono::steady_clock::now();

        // 启动线程
        std::vector<std::thread> workers;
        workers.reserve(threadCount);

        for (int t = 0; t < threadCount; ++t) {
            const int y0 = t * rowsPerThread;
            const int y1 = std::min(y0 + rowsPerThread, h);
            if (y0 >= y1) break; // 开始行超过渲染最大行 该线程无效
            endRowPerThread[t] = y1;

            workers.emplace_back([&, t, y0, y1]() {
                // 每个线程独立 RenderServer，避免随机数 gen 竞争
                auto rs = PathTracing::RenderServer(30, 1, 0.9f);
                auto buffer = std::make_unique<std::vector<float>>();
                buffer->reserve(static_cast<size_t>(y1 - y0) * w * comp);

                // 逐行渲染：每完成一行就上报一次，进度粒度最细
                for (int y = y0; y < y1; ++y) {
                    threadRows[t].store(y, std::memory_order_relaxed);
                    auto part = std::unique_ptr<std::vector<float>>(
                        rs.Tracing(camera, aabbs, lights, y, y + 1));
                    buffer->insert(buffer->end(), part->begin(), part->end());
                    doneRows.fetch_add(1, std::memory_order_relaxed);
                }
                threadRows[t].store(y1, std::memory_order_relaxed);

                partials[t] = std::move(buffer);
                finishedThreads.fetch_add(1, std::memory_order_relaxed);
            });
        }

        // 主线程定时打印进度：每 5 秒一行（带时间戳），保证任何时候都能看出它还在跑
        auto nextReport = renderStart;
        while (finishedThreads.load(std::memory_order_relaxed) < static_cast<int>(workers.size())) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            if (std::chrono::steady_clock::now() >= nextReport) {
                PrintRenderProgress(doneRows.load(std::memory_order_relaxed), h, threadRows, endRowPerThread, renderStart);
                nextReport += std::chrono::seconds(5);
            }
        }
        PrintRenderProgress(h, h, threadRows, endRowPerThread, renderStart);

        // 等待所有线程完成
        for (auto& th : workers) th.join();

        // 合并到主缓冲
        auto data = std::make_unique<std::vector<float>>(
            static_cast<size_t>(w) * h * comp, 0.0f);

        for (int t = 0; t < threadCount; ++t) {
            const int y0 = t * rowsPerThread;
            const int y1 = std::min(y0 + rowsPerThread, h);
            if (y0 >= y1 || !partials[t]) continue; // 超过渲染行数或者访问到空指针 continue

            const size_t offset = static_cast<size_t>(y0) * w * comp;
            std::copy(partials[t]->begin(), partials[t]->end(),
                      data->begin() + offset);
        }

        // 保存
        PrintPicture(fileName.c_str(), w, h, comp, data->data(), w * comp, PathTracing::PrintType::HDR);
    }

    std::vector<float>* RenderServer::Tracing(const Camera& camera, const std::vector<AABB>& aabbs, const std::vector<AreaLight>& lights) {
        auto data = new std::vector<float>();
        auto startTime = std::chrono::steady_clock::now();
        auto totalPixel = camera.width * camera.height;
        auto processed = 0;
        for ( int y = 0 ; y < camera.height; ++y) {
            for ( int x = 0 ; x < camera.width; ++x) {
                float a = 1.0f / static_cast<float>(SPP);
                auto pixel = glm::vec3(0.0f);
                for (int i = 0; i < SPP; ++i) {
                    auto randomX = dist(gen);
                    auto randomY = dist(gen);
                    auto ray = CastRay(camera, x, y, randomX, randomY);
                    auto hitMessage = RayTest(ray, aabbs);
                    auto result = Shade(hitMessage, aabbs, lights, 1);
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


    std::vector<float>* RenderServer::Tracing(const Camera& camera,
                                              const std::vector<AABB>& aabbs,
                                              const std::vector<AreaLight>& lights,
                                              int startY, int endY) {
        // 边界保护
        startY = std::max(0, startY);
        endY = std::min(endY, camera.height);
        if (startY >= endY) return new std::vector<float>();

        auto data = new std::vector<float>();
        const int totalPixel = camera.width * (endY - startY);


        // 预分配，避免 push_back 反复扩容
        data->reserve(static_cast<size_t>(totalPixel) * 3);

        for (int y = startY; y < endY; ++y) {
            for (int x = 0; x < camera.width; ++x) {
                float a = 1.0f / static_cast<float>(SPP);
                auto pixel = glm::vec3(0.0f);
                for (int i = 0; i < SPP; ++i) {
                    auto randomX = dist(gen);
                    auto randomY = dist(gen);
                    auto ray = CastRay(camera, x, y, randomX, randomY);
                    auto hitMessage = RayTest(ray, aabbs);
                    auto result = Shade(hitMessage, aabbs, lights, 1);
                    pixel += result * a;
                }

                data->push_back(pixel.x);
                data->push_back(pixel.y);
                data->push_back(pixel.z);
            }
        }
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


        // 自发光
        glm::vec3 L_e(0.0f);
        if (hitMessage.object && hitMessage.object->objectType == ObjectType::AreaLight) {
            // 单面光源: 只有打到发光面才有能量
            if (glm::dot(hitMessage.ray.direction, hitMessage.hitNormal) < 0.0f) {
                return hitMessage.object-> GetEmission ();
            }
            return L_e;
        }

        auto L_dir = glm::vec3(0.0f);
        auto L_indir = glm::vec3(0.0f);
        // 对光源
        for (const AreaLight& light : lights) {
            // (1) 发光面上均匀采一点
            const glm::vec3 lightPos = light.GetPointOnEmissiveFace(dist(gen), dist(gen));
            const glm::vec3 toLight  = lightPos - hitMessage.hitPos;
            const float distance     = glm::length(toLight);
            const glm::vec3 wi       = toLight / distance;

            // (2) 两侧余弦：cosB 用光源定义的发光面法线，而不是"刚好被打中的那个面"的法线
            const float cosA = glm::max(0.0f, glm::dot(hitMessage.hitNormal, wi));
            const float cosB = glm::max(0.0f, glm::dot(light.GetEmissiveNormal(), -wi));
            if (cosA <= 0.0f || cosB <= 0.0f) continue;

            // (3) 可见性
            const Ray shadowRay = CastRay(hitMessage.hitPos + kRayEpsilon * hitMessage.hitNormal, wi);
            if (IsOccluded(shadowRay, aabbs, distance)) continue;

            // (4) 面积采样的估计量
            const glm::vec3 f_r = Get_fr(hitMessage.triangle->brdf, hitMessage.hitPos,
                                         hitMessage.hitNormal, wi, -hitMessage.ray.direction);
            const float pdfArea = 1.0f / light.GetWorldArea();
            L_dir += light.emission * f_r * cosA * cosB / (distance * distance) / pdfArea;
        }

        // 对一般物体
        if (depth > 2) {
            float random = dist(gen);
            if (P_RR < random) L_indir = glm::vec3(0.0f);
            else {
                auto sampleResult = SampleCosineHemisphere(hitMessage.hitNormal);
                Ray ray = CastRay(hitMessage.hitPos + kRayEpsilon * hitMessage.hitNormal, sampleResult.randomDir);
                auto newHitMessage = RayTest(ray, aabbs); // 反射光线命中信息
                if (newHitMessage.triangle && newHitMessage.object && newHitMessage.object->objectType != ObjectType::AreaLight) {
                    glm::vec3 f_r = Get_fr(hitMessage.triangle->brdf, hitMessage.hitPos, hitMessage.hitNormal, sampleResult.randomDir, -hitMessage.ray.direction);
                    float pdf = sampleResult.pdf;
                    float cosA = glm::max(0.0f, GetCosValue(hitMessage.hitNormal, sampleResult.randomDir));
                    L_indir = Shade(newHitMessage, aabbs, lights, depth + 1) * f_r * cosA / pdf / P_RR;
                }
            }

        }
        else {
            auto sampleResult = SampleCosineHemisphere(hitMessage.hitNormal);
            Ray ray = CastRay(hitMessage.hitPos + kRayEpsilon * hitMessage.hitNormal, sampleResult.randomDir);
            auto newHitMessage = RayTest(ray, aabbs); // 反射光线命中信息
            if (newHitMessage.triangle && newHitMessage.object && newHitMessage.object->objectType != ObjectType::AreaLight) {
                glm::vec3 f_r = Get_fr(hitMessage.triangle->brdf, hitMessage.hitPos, hitMessage.hitNormal, sampleResult.randomDir, -hitMessage.ray.direction);
                float pdf = sampleResult.pdf;
                float cosA = glm::max(0.0f, GetCosValue(hitMessage.hitNormal, sampleResult.randomDir));
                L_indir = Shade(newHitMessage, aabbs, lights, depth + 1) * f_r * cosA / pdf;
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
     * 获取BRDF
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


        glm::vec3 f_Lambert = brdf.albedo / glm::pi<glm::vec3>();
        glm::vec3 f_CookTorrance{};

        auto N = hitNormal; // N 表面法线
        auto V = out; // V 交点→相机 out
        auto L = in; // L 交点→光源 in
        auto H = glm::normalize(L + V); // H 半向量
        float D = BRDF_D_GGX(glm::dot(N, H), brdf.roughness);
        // 计算正确的F0 垂直入射时的菲涅尔反射率
        glm::vec3 F0 = glm::mix(glm::vec3(0.04f), // 电介质的 F0
                                brdf.albedo,            // 金属的 F0
                                brdf.metallicity);      // 插值权重
        glm::vec3 F = BRDF_F_Fresnel_Schlick(glm::dot(V, H), F0);
        float G = BRDF_G_Smith(glm::dot(N, V), glm::dot(N, L), brdf.roughness);

        f_CookTorrance = D * F * G / (4 * (glm::dot(V, N) * (glm::dot(L, N))));

        // 扣除 镜面反射的能量 + 金属的漫反射
        // 1 - F: 透过表面进入介质的能量 漫反射可用能量
        glm::vec3 kd = (glm::vec3(1.0f) - F) * (1.0f - brdf.metallicity);
        return kd * f_Lambert + f_CookTorrance;
    }


    bool RenderServer::IsOccluded(const Ray& ray, const std::vector<AABB>& aabbs, float maxDistance) {
        const HitMessage blocker = RayTest(ray, aabbs); // aabbs 里含光源自身
        if (!blocker.triangle) return false; // 畅通
        // 射线正指向发光面上一点 → 命中光源自身时 t ≈ maxDistance，不算遮挡
        return blocker.t < maxDistance - 0.001f;
    }


    /**
     * 余弦加权半球采样
     * @param n
     * @return
     */
    HemisphereSampleResult RenderServer::SampleCosineHemisphere(const glm::vec3& n) {
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

        HemisphereSampleResult result;
        result.randomDir = glm::normalize(localDir.x * u + localDir.y * v + localDir.z * w);
        result.pdf = GetCosValue(result.randomDir, n) / glm::pi<float>();
        return result;
    }

    /**
     * 均匀半球采样
     * @param n
     * @return
     */
    HemisphereSampleResult RenderServer::SampleUniformHemisphere(const glm::vec3& n) {
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

        HemisphereSampleResult result;
        result.randomDir = glm::normalize(localDir.x * u + localDir.y * v + localDir.z * w);
        result.pdf = 1.0f / (2 * glm::pi<float>());
        return result;
    }


    std::unique_ptr<Object> ModelServer::GetDefaultCube() {
        auto triangles = std::vector<Triangle>();
        auto brdf = BRDF {
            .albedo = glm::vec3(1.0f, 1.0f, 1.0f),
            .roughness = 1.0f,
            .metallicity = 0.0f
        };
        // ==================== 前面 z = -0.5，法线 (0,0,-1) ====================
        Triangle triFront0 = {};
        triFront0.brdf = brdf;
        triFront0.t0 = glm::vec3(-0.5f, -0.5f, -0.5f);
        triFront0.t1 = glm::vec3(-0.5f,  0.5f, -0.5f);
        triFront0.t2 = glm::vec3( 0.5f,  0.5f, -0.5f);
        triFront0.n0 = glm::vec3(0.0f, 0.0f, -1.0f);
        triFront0.n1 = glm::vec3(0.0f, 0.0f, -1.0f);
        triFront0.n2 = glm::vec3(0.0f, 0.0f, -1.0f);
        triangles.push_back(triFront0);

        Triangle triFront1 = {};
        triFront1.brdf = brdf;
        triFront1.t0 = glm::vec3(-0.5f, -0.5f, -0.5f);
        triFront1.t1 = glm::vec3( 0.5f,  0.5f, -0.5f);
        triFront1.t2 = glm::vec3( 0.5f, -0.5f, -0.5f);
        triFront1.n0 = glm::vec3(0.0f, 0.0f, -1.0f);
        triFront1.n1 = glm::vec3(0.0f, 0.0f, -1.0f);
        triFront1.n2 = glm::vec3(0.0f, 0.0f, -1.0f);
        triangles.push_back(triFront1);

        // ==================== 后面 z = +0.5，法线 (0,0,+1) ====================
        Triangle triBack0 = {};
        triBack0.brdf = brdf;
        triBack0.t0 = glm::vec3(-0.5f, -0.5f,  0.5f);
        triBack0.t1 = glm::vec3( 0.5f, -0.5f,  0.5f);
        triBack0.t2 = glm::vec3( 0.5f,  0.5f,  0.5f);
        triBack0.n0 = glm::vec3(0.0f, 0.0f, 1.0f);
        triBack0.n1 = glm::vec3(0.0f, 0.0f, 1.0f);
        triBack0.n2 = glm::vec3(0.0f, 0.0f, 1.0f);
        triangles.push_back(triBack0);

        Triangle triBack1 = {};
        triBack1.brdf = brdf;
        triBack1.t0 = glm::vec3(-0.5f, -0.5f,  0.5f);
        triBack1.t1 = glm::vec3( 0.5f,  0.5f,  0.5f);
        triBack1.t2 = glm::vec3(-0.5f,  0.5f,  0.5f);
        triBack1.n0 = glm::vec3(0.0f, 0.0f, 1.0f);
        triBack1.n1 = glm::vec3(0.0f, 0.0f, 1.0f);
        triBack1.n2 = glm::vec3(0.0f, 0.0f, 1.0f);
        triangles.push_back(triBack1);

        // ==================== 右面 x = +0.5，法线 (+1,0,0) ====================
        Triangle triRight0 = {};
        triRight0.brdf = brdf;
        triRight0.t0 = glm::vec3( 0.5f, -0.5f, -0.5f);
        triRight0.t1 = glm::vec3( 0.5f,  0.5f, -0.5f);
        triRight0.t2 = glm::vec3( 0.5f,  0.5f,  0.5f);
        triRight0.n0 = glm::vec3(1.0f, 0.0f, 0.0f);
        triRight0.n1 = glm::vec3(1.0f, 0.0f, 0.0f);
        triRight0.n2 = glm::vec3(1.0f, 0.0f, 0.0f);
        triangles.push_back(triRight0);

        Triangle triRight1 = {};
        triRight1.brdf = brdf;
        triRight1.t0 = glm::vec3( 0.5f, -0.5f, -0.5f);
        triRight1.t1 = glm::vec3( 0.5f,  0.5f,  0.5f);
        triRight1.t2 = glm::vec3( 0.5f, -0.5f,  0.5f);
        triRight1.n0 = glm::vec3(1.0f, 0.0f, 0.0f);
        triRight1.n1 = glm::vec3(1.0f, 0.0f, 0.0f);
        triRight1.n2 = glm::vec3(1.0f, 0.0f, 0.0f);
        triangles.push_back(triRight1);

        // ==================== 左面 x = -0.5，法线 (-1,0,0) ====================
        Triangle triLeft0 = {};
        triLeft0.brdf = brdf;
        triLeft0.t0 = glm::vec3(-0.5f, -0.5f, -0.5f);
        triLeft0.t1 = glm::vec3(-0.5f, -0.5f,  0.5f);
        triLeft0.t2 = glm::vec3(-0.5f,  0.5f,  0.5f);
        triLeft0.n0 = glm::vec3(-1.0f, 0.0f, 0.0f);
        triLeft0.n1 = glm::vec3(-1.0f, 0.0f, 0.0f);
        triLeft0.n2 = glm::vec3(-1.0f, 0.0f, 0.0f);
        triangles.push_back(triLeft0);

        Triangle triLeft1 = {};
        triLeft1.brdf = brdf;
        triLeft1.t0 = glm::vec3(-0.5f, -0.5f, -0.5f);
        triLeft1.t1 = glm::vec3(-0.5f,  0.5f,  0.5f);
        triLeft1.t2 = glm::vec3(-0.5f,  0.5f, -0.5f);
        triLeft1.n0 = glm::vec3(-1.0f, 0.0f, 0.0f);
        triLeft1.n1 = glm::vec3(-1.0f, 0.0f, 0.0f);
        triLeft1.n2 = glm::vec3(-1.0f, 0.0f, 0.0f);
        triangles.push_back(triLeft1);


        // ==================== 顶面 y = +0.5，法线 (0,+1,0) ====================
        Triangle triTop0 = {};
        triTop0.brdf = brdf;
        triTop0.t0 = glm::vec3(-0.5f,  0.5f, -0.5f);
        triTop0.t1 = glm::vec3(-0.5f,  0.5f,  0.5f);
        triTop0.t2 = glm::vec3( 0.5f,  0.5f,  0.5f);
        triTop0.n0 = glm::vec3(0.0f, 1.0f, 0.0f);
        triTop0.n1 = glm::vec3(0.0f, 1.0f, 0.0f);
        triTop0.n2 = glm::vec3(0.0f, 1.0f, 0.0f);
        triangles.push_back(triTop0);

        Triangle triTop1 = {};
        triTop1.brdf = brdf;
        triTop1.t0 = glm::vec3(-0.5f,  0.5f, -0.5f);
        triTop1.t1 = glm::vec3( 0.5f,  0.5f,  0.5f);
        triTop1.t2 = glm::vec3( 0.5f,  0.5f, -0.5f);
        triTop1.n0 = glm::vec3(0.0f, 1.0f, 0.0f);
        triTop1.n1 = glm::vec3(0.0f, 1.0f, 0.0f);
        triTop1.n2 = glm::vec3(0.0f, 1.0f, 0.0f);
        triangles.push_back(triTop1);


        // ==================== 底面 y = -0.5，法线 (0,-1,0) ====================
        Triangle triBottom0 = {};
        triBottom0.brdf = brdf;
        triBottom0.t0 = glm::vec3(-0.5f, -0.5f, -0.5f);
        triBottom0.t1 = glm::vec3( 0.5f, -0.5f, -0.5f);
        triBottom0.t2 = glm::vec3( 0.5f, -0.5f,  0.5f);
        triBottom0.n0 = glm::vec3(0.0f, -1.0f, 0.0f);
        triBottom0.n1 = glm::vec3(0.0f, -1.0f, 0.0f);
        triBottom0.n2 = glm::vec3(0.0f, -1.0f, 0.0f);
        triangles.push_back(triBottom0);

        Triangle triBottom1 = {};
        triBottom1.brdf = brdf;
        triBottom1.t0 = glm::vec3(-0.5f, -0.5f, -0.5f);
        triBottom1.t1 = glm::vec3( 0.5f, -0.5f,  0.5f);
        triBottom1.t2 = glm::vec3(-0.5f, -0.5f,  0.5f);
        triBottom1.n0 = glm::vec3(0.0f, -1.0f, 0.0f);
        triBottom1.n1 = glm::vec3(0.0f, -1.0f, 0.0f);
        triBottom1.n2 = glm::vec3(0.0f, -1.0f, 0.0f);
        triangles.push_back(triBottom1);






        auto cube = new Object();
        cube->objectType = ObjectType::TriangleMesh;
        cube->triangles = triangles;
        cube->modelMatrix = glm::mat4(1.0f);
        cube->invModelMatrix = glm::mat4(1.0f);
        cube->normalMatrix = glm::mat4(1.0f);

        return std::unique_ptr<Object>(cube);
    }

    std::unique_ptr<std::vector<Triangle>> ModelServer::GetAreaLightSquad() {
        const glm::vec3 n(0.0f, -1.0f, 0.0f);
        const BRDF brdf{ glm::vec3(1.0f), 1.0f, 0.0f };
        auto squad = new std::vector<Triangle>();
        Triangle a{};
        a.t0 = glm::vec3(-0.5f, 0.0f, -0.5f);
        a.t1 = glm::vec3(-0.5f, 0.0f,  0.5f);
        a.t2 = glm::vec3( 0.5f, 0.0f,  0.5f);
        a.n0 = a.n1 = a.n2 = n;
        a.brdf = brdf;

        Triangle b{};
        b.t0 = glm::vec3(-0.5f, 0.0f, -0.5f);
        b.t1 = glm::vec3( 0.5f, 0.0f,  0.5f);
        b.t2 = glm::vec3( 0.5f, 0.0f, -0.5f);
        b.n0 = b.n1 = b.n2 = n;
        b.brdf = brdf;

        squad->push_back(a);
        squad->push_back(b);
        return std::make_unique<std::vector<Triangle>>(*squad);
    }




}
