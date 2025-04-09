#ifndef RAYCOMMON_GLSL
#define RAYCOMMON_GLSL

// 光线命中结果
struct HitInfo {
    vec3 color;
    vec3 normal;
    float depth;
    int objectId;
};

// 材质属性
struct MaterialInfo {
    vec3 albedo;
    vec3 emission;
    float roughness;
    float metallic;
};

// 光线结构
struct Ray {
    vec3 origin;
    vec3 direction;
};

// 常量定义
#define EPSILON 0.001
#define PI 3.1415926535897932384626433832795
#define TWO_PI 6.283185307179586476925286766559
#define INV_PI 0.3183098861837906715377675267450

// 随机数生成
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

// 在半球上采样
vec3 sampleHemisphere(float u1, float u2) {
    const float r = sqrt(1.0 - u1 * u1);
    const float phi = u2 * TWO_PI;
    return vec3(r * cos(phi), u1, r * sin(phi));
}

// 构造正交基
void createOrthoNormalBasis(in vec3 n, out vec3 b1, out vec3 b2) {
    if (abs(n.z) > 0.999) {
        b1 = vec3(0, 1, 0);
        b2 = vec3(1, 0, 0);
        return;
    }
    
    b1 = normalize(cross(n, vec3(0, 0, 1)));
    b2 = normalize(cross(n, b1));
}

// 将向量从切线空间转换到世界空间
vec3 tangentToWorld(vec3 v, vec3 n) {
    vec3 b1, b2;
    createOrthoNormalBasis(n, b1, b2);
    return b1 * v.x + n * v.y + b2 * v.z;
}

#endif // RAYCOMMON_GLSL 