#include <iostream>
#include <vector>
#include "path_tracing.h"



int main() {

    glm::vec3 camPos(2.78f, 2.78f, -8.0f);
    glm::vec3 lookTarget(2.78f, 2.78f, 2.78f);
    glm::mat4 viewMatrix = glm::lookAt(camPos, lookTarget, glm::vec3(0,1,0));
    glm::mat4 camWorld = glm::inverse(viewMatrix);
    auto camera = std::make_unique<PathTracing::Camera>(camWorld, 256, 256, 40, 0.01f, 200.0f);
    auto scene = PathTracing::ModelServer::GetCornellBox();

    PathTracing::RenderServer::PathTracing(*camera, scene->aabbs, scene->lights, "Output", PathTracing::PrintType::HDR, 1);

    return 0;
}


