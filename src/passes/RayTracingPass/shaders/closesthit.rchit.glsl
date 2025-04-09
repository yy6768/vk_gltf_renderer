#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable

#include "raycommon.glsl"

// 光线载荷
layout(location = 0) rayPayloadInEXT HitInfo hitInfo;

// 三角形属性
hitAttributeEXT vec2 attribs;

// 场景描述符
layout(set = 1, binding = 1, scalar) readonly buffer Vertices { vec4 vertices[]; };
layout(set = 1, binding = 2, scalar) readonly buffer Indices { uint indices[]; };
layout(set = 1, binding = 3, scalar) readonly buffer Normals { vec4 normals[]; };
layout(set = 1, binding = 4, scalar) readonly buffer TexCoords { vec2 texCoords[]; };
layout(set = 1, binding = 5, scalar) readonly buffer Materials { MaterialInfo materials[]; };
layout(set = 1, binding = 6) uniform sampler2D textures[];

// 处理命中
void main() {
    // 获取三角形索引
    ivec3 indicesVec = ivec3(indices[3 * gl_PrimitiveID], 
                           indices[3 * gl_PrimitiveID + 1], 
                           indices[3 * gl_PrimitiveID + 2]);

    // 获取顶点坐标
    vec3 v0 = vertices[indicesVec.x].xyz;
    vec3 v1 = vertices[indicesVec.y].xyz;
    vec3 v2 = vertices[indicesVec.z].xyz;

    // 获取重心坐标
    const vec3 barycentrics = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);

    // 获取命中点的世界坐标
    const vec3 hitPosition = v0 * barycentrics.x + v1 * barycentrics.y + v2 * barycentrics.z;

    // 获取法线
    vec3 n0 = normals[indicesVec.x].xyz;
    vec3 n1 = normals[indicesVec.y].xyz;
    vec3 n2 = normals[indicesVec.z].xyz;
    vec3 normal = normalize(n0 * barycentrics.x + n1 * barycentrics.y + n2 * barycentrics.z);

    // 获取纹理坐标
    vec2 tc0 = texCoords[indicesVec.x];
    vec2 tc1 = texCoords[indicesVec.y];
    vec2 tc2 = texCoords[indicesVec.z];
    vec2 texCoord = tc0 * barycentrics.x + tc1 * barycentrics.y + tc2 * barycentrics.z;

    // 获取材质 (假设每个三角形有一个材质ID)
    int materialID = gl_InstanceCustomIndexEXT;
    MaterialInfo material = materials[materialID];

    // 获取材质颜色
    vec3 albedo = material.albedo;
    
    // 如果使用纹理，从纹理中采样
    // if (material.albedoTexID >= 0) {
    //     albedo *= texture(textures[material.albedoTexID], texCoord).rgb;
    // }

    // 计算阴影光线
    vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
    float shadow = 1.0;

    // 发射阴影光线
    if (dot(normal, lightDir) > 0.0) {
        // 在实际应用中，这里应该发射阴影光线
        // traceRayEXT(...);
    }

    // 简单着色
    vec3 color = albedo * max(0.0, dot(normal, lightDir)) * shadow;
    
    // 添加环境光
    color += albedo * 0.2;
    
    // 添加自发光
    color += material.emission;

    // 设置输出
    hitInfo.color = color;
    hitInfo.normal = normal;
    hitInfo.depth = gl_HitTEXT;
    hitInfo.objectId = gl_InstanceID;
} 