#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_shader_explicit_arithmetic_types : enable

// 光线追踪的输入绑定
layout(binding = 0, set = 0) uniform accelerationStructureEXT topLevelAS;
layout(binding = 1, set = 0, rgba32f) uniform image2D outImage;
layout(binding = 2, set = 0, rgba32f) uniform image2D normalDepth;
layout(binding = 3, set = 0) uniform image2D selectImage;

// 推送常量
layout(push_constant) uniform PushConstant {
    uint maxDepth;
    uint samples;
    uint frameCount;
} pushConst;

// 相机参数（在实际应用中通常从uniform buffer中读取）
layout(binding = 0, set = 1) uniform CameraInfo {
    mat4 viewInverse;
    mat4 projInverse;
    vec4 cameraPos;
} camera;

// 光线命中结果
struct HitInfo {
    vec3 color;
    vec3 normal;
    float depth;
    int objectId;
};

// 光线载荷
layout(location = 0) rayPayloadEXT HitInfo hitInfo;

// 生成一个光线方向
vec3 generateRay(vec2 pixelCenter, vec2 imageSize) {
    // 将像素坐标转换为 [-1, 1] 范围的 NDC 坐标
    vec2 d = (pixelCenter / imageSize) * 2.0 - 1.0;
    
    // 使用投影矩阵逆矩阵转换到相机空间
    vec4 origin = camera.projInverse * vec4(d.x, d.y, -1.0, 1.0);
    origin /= origin.w;
    
    // 使用视图矩阵逆矩阵转换到世界空间
    vec4 worldOrigin = camera.viewInverse * origin;
    vec3 rayDir = normalize(worldOrigin.xyz - camera.cameraPos.xyz);
    
    return rayDir;
}

// 简单的随机数生成
uint wang_hash(inout uint seed) {
    seed = uint(seed ^ uint(61)) ^ uint(seed >> uint(16));
    seed *= uint(9);
    seed = seed ^ (seed >> 4);
    seed *= uint(0x27d4eb2d);
    seed = seed ^ (seed >> 15);
    return seed;
}

float random(inout uint seed) {
    return float(wang_hash(seed)) / 4294967296.0;
}

// 主函数
void main() {
    // 获取当前像素坐标
    const vec2 pixelCenter = vec2(gl_LaunchIDEXT.xy) + vec2(0.5);
    const vec2 imageSize = vec2(gl_LaunchSizeEXT.xy);
    
    // 使用帧号作为随机种子
    uint seed = gl_LaunchIDEXT.x + gl_LaunchIDEXT.y * gl_LaunchSizeEXT.x + pushConst.frameCount * 719393;
    
    // 累积的颜色和采样计数
    vec3 accumulatedColor = vec3(0.0);
    
    // 进行多次采样
    for (uint s = 0; s < pushConst.samples; ++s) {
        // 添加随机抖动进行抗锯齿
        vec2 jitter = vec2(random(seed), random(seed)) - vec2(0.5);
        vec2 sampledPixel = pixelCenter + jitter;
        
        // 生成光线方向
        vec3 rayDir = generateRay(sampledPixel, imageSize);
        
        // 设置光线起点（相机位置）
        vec3 rayOrigin = camera.cameraPos.xyz;
        
        // 光线追踪
        hitInfo.color = vec3(0.0);
        hitInfo.normal = vec3(0.0);
        hitInfo.depth = 0.0;
        hitInfo.objectId = -1;
        
        // 发射主光线
        traceRayEXT(
            topLevelAS,            // 加速结构
            gl_RayFlagsOpaqueEXT,  // 光线标志
            0xFF,                  // 掩码
            0,                     // sbt记录偏移
            0,                     // sbt记录跨度
            0,                     // miss索引
            rayOrigin,             // 光线起点
            0.001,                 // 最小距离
            rayDir,                // 光线方向
            10000.0,               // 最大距离
            0                      // 负载位置
        );
        
        // 积累采样结果
        accumulatedColor += hitInfo.color;
    }
    
    // 计算平均颜色
    vec3 finalColor = accumulatedColor / float(pushConst.samples);
    
    // 在首帧清空图像，然后进行累积
    if (pushConst.frameCount == 0) {
        // 写入输出图像
        imageStore(outImage, ivec2(gl_LaunchIDEXT.xy), vec4(finalColor, 1.0));
        
        // 写入法线和深度信息
        imageStore(normalDepth, ivec2(gl_LaunchIDEXT.xy), vec4(hitInfo.normal, hitInfo.depth));
        
        // 写入对象ID
        imageStore(selectImage, ivec2(gl_LaunchIDEXT.xy), vec4(hitInfo.objectId, 0.0, 0.0, 0.0));
    } else {
        // 读取之前的结果
        vec4 prevColor = imageLoad(outImage, ivec2(gl_LaunchIDEXT.xy));
        
        // 指数移动平均值 - 简单的时间性抗锯齿
        float blendFactor = 1.0 / float(pushConst.frameCount + 1);
        vec3 newColor = mix(prevColor.rgb, finalColor, blendFactor);
        
        // 写入合并后的结果
        imageStore(outImage, ivec2(gl_LaunchIDEXT.xy), vec4(newColor, 1.0));
    }
} 