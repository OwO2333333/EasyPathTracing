#include <iostream>
#include <vector>
#include "path_tracing.h"



int main() {

    auto triangles = std::vector<PathTracing::Triangle>();
    auto brdf = PathTracing::BRDF {
        .albedo = glm::vec3(1.0f, 1.0f, 1.0f),
        .roughness = 1.0f,
        .metallicity = 0.0f
    };
    // ==================== 前面 z = -0.5，法线 (0,0,-1) ====================
    PathTracing::Triangle triFront0 = {};
    triFront0.brdf = brdf;
    triFront0.t0 = glm::vec3(-0.5f, -0.5f, -0.5f);
    triFront0.t1 = glm::vec3(-0.5f,  0.5f, -0.5f);
    triFront0.t2 = glm::vec3( 0.5f,  0.5f, -0.5f);
    triFront0.n0 = glm::vec3(0.0f, 0.0f, -1.0f);
    triFront0.n1 = glm::vec3(0.0f, 0.0f, -1.0f);
    triFront0.n2 = glm::vec3(0.0f, 0.0f, -1.0f);
    triangles.push_back(triFront0);

    PathTracing::Triangle triFront1 = {};
    triFront1.brdf = brdf;
    triFront1.t0 = glm::vec3(-0.5f, -0.5f, -0.5f);
    triFront1.t1 = glm::vec3( 0.5f,  0.5f, -0.5f);
    triFront1.t2 = glm::vec3( 0.5f, -0.5f, -0.5f);
    triFront1.n0 = glm::vec3(0.0f, 0.0f, -1.0f);
    triFront1.n1 = glm::vec3(0.0f, 0.0f, -1.0f);
    triFront1.n2 = glm::vec3(0.0f, 0.0f, -1.0f);
    triangles.push_back(triFront1);

    // ==================== 后面 z = +0.5，法线 (0,0,+1) ====================
    PathTracing::Triangle triBack0 = {};
    triBack0.brdf = brdf;
    triBack0.t0 = glm::vec3(-0.5f, -0.5f,  0.5f);
    triBack0.t1 = glm::vec3( 0.5f, -0.5f,  0.5f);
    triBack0.t2 = glm::vec3( 0.5f,  0.5f,  0.5f);
    triBack0.n0 = glm::vec3(0.0f, 0.0f, 1.0f);
    triBack0.n1 = glm::vec3(0.0f, 0.0f, 1.0f);
    triBack0.n2 = glm::vec3(0.0f, 0.0f, 1.0f);
    triangles.push_back(triBack0);

    PathTracing::Triangle triBack1 = {};
    triBack1.brdf = brdf;
    triBack1.t0 = glm::vec3(-0.5f, -0.5f,  0.5f);
    triBack1.t1 = glm::vec3( 0.5f,  0.5f,  0.5f);
    triBack1.t2 = glm::vec3(-0.5f,  0.5f,  0.5f);
    triBack1.n0 = glm::vec3(0.0f, 0.0f, 1.0f);
    triBack1.n1 = glm::vec3(0.0f, 0.0f, 1.0f);
    triBack1.n2 = glm::vec3(0.0f, 0.0f, 1.0f);
    triangles.push_back(triBack1);

    // ==================== 右面 x = +0.5，法线 (+1,0,0) ====================
    PathTracing::Triangle triRight0 = {};
    triRight0.brdf = brdf;
    triRight0.t0 = glm::vec3( 0.5f, -0.5f, -0.5f);
    triRight0.t1 = glm::vec3( 0.5f,  0.5f, -0.5f);
    triRight0.t2 = glm::vec3( 0.5f,  0.5f,  0.5f);
    triRight0.n0 = glm::vec3(1.0f, 0.0f, 0.0f);
    triRight0.n1 = glm::vec3(1.0f, 0.0f, 0.0f);
    triRight0.n2 = glm::vec3(1.0f, 0.0f, 0.0f);
    triangles.push_back(triRight0);

    PathTracing::Triangle triRight1 = {};
    triRight1.brdf = brdf;
    triRight1.t0 = glm::vec3( 0.5f, -0.5f, -0.5f);
    triRight1.t1 = glm::vec3( 0.5f,  0.5f,  0.5f);
    triRight1.t2 = glm::vec3( 0.5f, -0.5f,  0.5f);
    triRight1.n0 = glm::vec3(1.0f, 0.0f, 0.0f);
    triRight1.n1 = glm::vec3(1.0f, 0.0f, 0.0f);
    triRight1.n2 = glm::vec3(1.0f, 0.0f, 0.0f);
    triangles.push_back(triRight1);

    // ==================== 左面 x = -0.5，法线 (-1,0,0) ====================
    PathTracing::Triangle triLeft0 = {};
    triLeft0.brdf = brdf;
    triLeft0.t0 = glm::vec3(-0.5f, -0.5f, -0.5f);
    triLeft0.t1 = glm::vec3(-0.5f, -0.5f,  0.5f);
    triLeft0.t2 = glm::vec3(-0.5f,  0.5f,  0.5f);
    triLeft0.n0 = glm::vec3(-1.0f, 0.0f, 0.0f);
    triLeft0.n1 = glm::vec3(-1.0f, 0.0f, 0.0f);
    triLeft0.n2 = glm::vec3(-1.0f, 0.0f, 0.0f);
    triangles.push_back(triLeft0);

    PathTracing::Triangle triLeft1 = {};
    triLeft1.brdf = brdf;
    triLeft1.t0 = glm::vec3(-0.5f, -0.5f, -0.5f);
    triLeft1.t1 = glm::vec3(-0.5f,  0.5f,  0.5f);
    triLeft1.t2 = glm::vec3(-0.5f,  0.5f, -0.5f);
    triLeft1.n0 = glm::vec3(-1.0f, 0.0f, 0.0f);
    triLeft1.n1 = glm::vec3(-1.0f, 0.0f, 0.0f);
    triLeft1.n2 = glm::vec3(-1.0f, 0.0f, 0.0f);
    triangles.push_back(triLeft1);


    // ==================== 顶面 y = +0.5，法线 (0,+1,0) ====================
    PathTracing::Triangle triTop0 = {};
    triTop0.brdf = brdf;
    triTop0.t0 = glm::vec3(-0.5f,  0.5f, -0.5f);
    triTop0.t1 = glm::vec3(-0.5f,  0.5f,  0.5f);
    triTop0.t2 = glm::vec3( 0.5f,  0.5f,  0.5f);
    triTop0.n0 = glm::vec3(0.0f, 1.0f, 0.0f);
    triTop0.n1 = glm::vec3(0.0f, 1.0f, 0.0f);
    triTop0.n2 = glm::vec3(0.0f, 1.0f, 0.0f);
    triangles.push_back(triTop0);

    PathTracing::Triangle triTop1 = {};
    triTop1.brdf = brdf;
    triTop1.t0 = glm::vec3(-0.5f,  0.5f, -0.5f);
    triTop1.t1 = glm::vec3( 0.5f,  0.5f,  0.5f);
    triTop1.t2 = glm::vec3( 0.5f,  0.5f, -0.5f);
    triTop1.n0 = glm::vec3(0.0f, 1.0f, 0.0f);
    triTop1.n1 = glm::vec3(0.0f, 1.0f, 0.0f);
    triTop1.n2 = glm::vec3(0.0f, 1.0f, 0.0f);
    triangles.push_back(triTop1);


    // ==================== 底面 y = -0.5，法线 (0,-1,0) ====================
    PathTracing::Triangle triBottom0 = {};
    triBottom0.brdf = brdf;
    triBottom0.t0 = glm::vec3(-0.5f, -0.5f, -0.5f);
    triBottom0.t1 = glm::vec3( 0.5f, -0.5f, -0.5f);
    triBottom0.t2 = glm::vec3( 0.5f, -0.5f,  0.5f);
    triBottom0.n0 = glm::vec3(0.0f, -1.0f, 0.0f);
    triBottom0.n1 = glm::vec3(0.0f, -1.0f, 0.0f);
    triBottom0.n2 = glm::vec3(0.0f, -1.0f, 0.0f);
    triangles.push_back(triBottom0);

    PathTracing::Triangle triBottom1 = {};
    triBottom1.brdf = brdf;
    triBottom1.t0 = glm::vec3(-0.5f, -0.5f, -0.5f);
    triBottom1.t1 = glm::vec3( 0.5f, -0.5f,  0.5f);
    triBottom1.t2 = glm::vec3(-0.5f, -0.5f,  0.5f);
    triBottom1.n0 = glm::vec3(0.0f, -1.0f, 0.0f);
    triBottom1.n1 = glm::vec3(0.0f, -1.0f, 0.0f);
    triBottom1.n2 = glm::vec3(0.0f, -1.0f, 0.0f);
    triangles.push_back(triBottom1);









    auto cube = PathTracing::Object {};
    cube.objectType = PathTracing::ObjectType::TriangleMesh;
    cube.triangles = triangles;
    cube.modelMatrix = glm::mat4(1.0f);
    cube.invModelMatrix = glm::mat4(1.0f);
    cube.normalMatrix = glm::mat4(1.0f);


    // auto modelServer = PathTracing::ModelServer();
    // auto cube = modelServer.GetDefaultCube();








    auto camera = PathTracing::Camera();
    glm::vec3 camPos(2.78f, 2.78f, -8.0f);
    glm::vec3 lookTarget(2.78f, 2.78f, 2.78f);
    glm::mat4 viewMatrix = glm::lookAt(camPos, lookTarget, glm::vec3(0,1,0));
    glm::mat4 camWorld = glm::inverse(viewMatrix);
    camera.modelMatrix = camWorld;
    camera.width = 256;
    camera.height = 256;
    camera.aspect = 1.0f;
    camera.fov = 40.0f;
    camera.near = 0.01f;
    camera.far = 20.0f;



    auto aabbs = std::vector<PathTracing::AABB>();
    auto lights = std::vector<PathTracing::AreaLight>();

    // TODO: 把数据改到堆里 优化camera方便创建


    auto modelMatrix = glm::mat4(1.0f);
    auto floor_wall = cube;
    modelMatrix = glm::translate(modelMatrix, glm::vec3(2.775f, -0.005f, 2.775f));
    modelMatrix = glm::scale(modelMatrix, glm::vec3(5.55f, 0.01f, 5.55f));
    floor_wall.modelMatrix = modelMatrix;
    floor_wall.invModelMatrix = glm::inverse(modelMatrix);
    floor_wall.normalMatrix = glm::transpose(glm::inverse(modelMatrix));
    for (PathTracing::Triangle& tri : floor_wall.triangles) {
        tri.brdf.albedo = glm::vec3(0.725, 0.71, 0.68);
    }
    aabbs.emplace_back(floor_wall);

    modelMatrix = glm::mat4(1.0f);
    auto ceil_wall = cube;
    modelMatrix = glm::translate(modelMatrix, glm::vec3(2.775, 5.555, 2.775));
    modelMatrix = glm::scale(modelMatrix, glm::vec3(5.55, 0.01, 5.55));
    ceil_wall.modelMatrix = modelMatrix;
    ceil_wall.invModelMatrix = glm::inverse(modelMatrix);
    ceil_wall.normalMatrix = glm::transpose(glm::inverse(modelMatrix));
    for (PathTracing::Triangle& tri : ceil_wall.triangles) {
        tri.brdf.albedo = glm::vec3(0.725, 0.71, 0.68);
    }
    aabbs.emplace_back(ceil_wall);

    modelMatrix = glm::mat4(1.0f);
    auto back_wall = cube;
    modelMatrix = glm::translate(modelMatrix, glm::vec3(2.775, 2.775, 5.555));
    modelMatrix = glm::scale(modelMatrix, glm::vec3(5.55, 5.55, 0.01));
    back_wall.modelMatrix = modelMatrix;
    back_wall.invModelMatrix = glm::inverse(modelMatrix);
    back_wall.normalMatrix = glm::transpose(glm::inverse(modelMatrix));
    for (PathTracing::Triangle& tri : back_wall.triangles) {
        tri.brdf.albedo = glm::vec3(0.725, 0.71, 0.68);
    }
    aabbs.emplace_back(back_wall);

    modelMatrix = glm::mat4(1.0f);
    auto left_wall = cube;
    modelMatrix = glm::translate(modelMatrix, glm::vec3(-0.005, 2.775, 2.775));
    modelMatrix = glm::scale(modelMatrix, glm::vec3(0.01, 5.55, 5.55));
    left_wall.modelMatrix = modelMatrix;
    left_wall.invModelMatrix = glm::inverse(modelMatrix);
    left_wall.normalMatrix = glm::transpose(glm::inverse(modelMatrix));
    for (PathTracing::Triangle& tri : left_wall.triangles) {
        tri.brdf.albedo = glm::vec3(0.14, 0.45, 0.091);
    }
    aabbs.emplace_back(left_wall);

    modelMatrix = glm::mat4(1.0f);
    auto right_wall = cube;
    modelMatrix = glm::translate(modelMatrix, glm::vec3(5.555, 2.775, 2.775));
    modelMatrix = glm::scale(modelMatrix, glm::vec3(0.01, 5.55, 5.55));
    right_wall.modelMatrix = modelMatrix;
    right_wall.invModelMatrix = glm::inverse(modelMatrix);
    right_wall.normalMatrix = glm::transpose(glm::inverse(modelMatrix));
    for (PathTracing::Triangle& tri : right_wall.triangles) {
        tri.brdf.albedo = glm::vec3(0.63, 0.065, 0.05);
    }
    aabbs.emplace_back(right_wall);

    modelMatrix = glm::mat4(1.0f);
    auto big_cube = cube;
    modelMatrix = glm::translate(modelMatrix, glm::vec3(3.475f, 1.65f, 3.775f));
    modelMatrix = glm::rotate(modelMatrix, glm::radians(18.0f), glm::vec3(0, 1, 0));
    modelMatrix = glm::scale(modelMatrix, glm::vec3(1.65f, 3.30f, 1.65f));
    big_cube.modelMatrix = modelMatrix;
    big_cube.invModelMatrix = glm::inverse(modelMatrix);
    big_cube.normalMatrix = glm::transpose(glm::inverse(modelMatrix));
    for (PathTracing::Triangle& tri : big_cube.triangles) {
        tri.brdf.albedo = glm::vec3(0.725, 0.71, 0.68);
    }
    aabbs.emplace_back(big_cube);

    modelMatrix = glm::mat4(1.0f);
    auto small_cube = cube;
    modelMatrix = glm::translate(modelMatrix, glm::vec3(2.125f, 0.825f, 1.475f));
    modelMatrix = glm::rotate(modelMatrix, glm::radians(-18.0f), glm::vec3(0, 1, 0));
    modelMatrix = glm::scale(modelMatrix, glm::vec3(1.65f, 1.65f, 1.65f));
    small_cube.modelMatrix = modelMatrix;
    small_cube.invModelMatrix = glm::inverse(modelMatrix);
    small_cube.normalMatrix = glm::transpose(glm::inverse(modelMatrix));
    for (PathTracing::Triangle& tri : small_cube.triangles) {
        tri.brdf.albedo = glm::vec3(0.725, 0.71, 0.68);
    }
    aabbs.emplace_back(small_cube);

    modelMatrix = glm::mat4(1.0f);
    // 光源：单面发光（两个三角形），局部 XZ 平面单位方形，法线朝下
    auto light = PathTracing::AreaLight(glm::vec3(1, 0.9, 0.7) * 40.0f);
    modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(2.78f, 5.549f, 2.795f))
                                * glm::scale(glm::mat4(1.0f), glm::vec3(1.30f, 1.0f, 1.05f));
    light.modelMatrix = modelMatrix;
    light.invModelMatrix = glm::inverse(modelMatrix);
    light.normalMatrix = glm::mat3(glm::transpose(glm::inverse(modelMatrix)));
    lights.emplace_back(light);
    aabbs.emplace_back(lights.back());


    PathTracing::RenderServer::PathTracing(camera, aabbs, lights, "Output", PathTracing::PrintType::HDR, 1);

    return 0;
}


