#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : enable

#include "raycommon.glsl"

// 光线命中结果
struct HitInfo {
    vec3 color;
    vec3 normal;
    float depth;
    int objectId;
};

// 光线载荷
layout(location = 0) rayPayloadInEXT HitInfo hitInfo;

// 简单的天空盒函数
vec3 getSkyColor(vec3 direction) {
    // 简单的渐变天空
    vec3 zenith = vec3(0.0, 0.3, 0.8);  // 天顶颜色
    vec3 horizon = vec3(0.7, 0.8, 1.0); // 地平线颜色
    
    float t = 0.5 * (direction.y + 1.0);
    return mix(horizon, zenith, t);
}

void main() {
    // 设置未命中的信息
    hitInfo.color = getSkyColor(gl_WorldRayDirectionEXT);
    hitInfo.normal = vec3(0.0, 1.0, 0.0);  // 默认向上的法线
    hitInfo.depth = 10000.0;  // 很远
    hitInfo.objectId = -1;    // 无效ID
} 