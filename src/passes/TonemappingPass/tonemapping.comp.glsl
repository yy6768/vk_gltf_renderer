#version 450
layout(local_size_x = 16, local_size_y = 16) in;

layout(binding = 0, rgba32f) uniform readonly image2D inputImage;
layout(binding = 1, rgba8) uniform writeonly image2D outputImage;

// ACES 参数
const float A = 2.51f;
const float B = 0.03f;
const float C = 2.43f;
const float D = 0.59f;
const float E = 0.14f;

// ACES Film Tonemapping
vec3 ACESFilm(vec3 x) {
    return clamp((x * (A * x + B)) / (x * (C * x + D) + E), 0.0, 1.0);
}

// 可选：曝光控制参数
layout(push_constant) uniform Params {
    float exposure;
} params;

void main() {
    ivec2 texCoord = ivec2(gl_GlobalInvocationID.xy);
    ivec2 imgSize = imageSize(inputImage);
    
    if(texCoord.x >= imgSize.x || texCoord.y >= imgSize.y) {
        return;
    }
    
    // 读取HDR颜色
    vec4 hdrColor = imageLoad(inputImage, texCoord);
    
    // 应用曝光控制 (可选)
    vec3 exposedColor = hdrColor.rgb * pow(2.0, params.exposure); 
    
    // 应用ACES tonemapping
    vec3 ldrColor = ACESFilm(exposedColor);
    
    // 可选：应用gamma校正 (假设输入已经是线性空间)
    ldrColor = pow(ldrColor, vec3(1.0/2.2));
    
    // 写入结果
    imageStore(outputImage, texCoord, vec4(ldrColor, 1.0));
} 