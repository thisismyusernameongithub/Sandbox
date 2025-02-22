#version 300 es
precision mediump float;

in vec2 texCoord;
out vec4 FragColor;
uniform sampler2D inputDataTexture1;
uniform vec2 resolution;
uniform int blurDirection;  // 0 for horizontal, 1 for vertical
uniform int radius;         // Kernel radius

// Function to calculate Gaussian weight
float gaussian(float x, float sigma) {
    return exp(-(x * x) / (2.0 * sigma * sigma)) / (2.0 * 3.14159 * sigma * sigma);
}

void main() {
    vec2 texOffset = 1.0 / resolution; // Adjusts step size based on texture resolution

    // Calculate the total number of samples (2 * radius + 1)
    int kernelSize = 2 * radius + 1;
    float sigma = float(radius) / 3.0; // A common choice for sigma is radius / 3

    vec4 blurColor = vec4(0.0);
    float weightSum = 0.0;

    // Apply blur based on the direction and calculate dynamic weights
    for (int i = -radius; i <= radius; i++) {
        float weight = gaussian(float(i), sigma);
        weightSum += weight;

        vec2 sampleCoord = texCoord;
        if (blurDirection == 0) {
            sampleCoord += texOffset * vec2(i, 0);
        } else {
            sampleCoord += texOffset * vec2(0, i);
        }
        blurColor += texture(inputDataTexture1, sampleCoord) * weight;
    }

    // Normalize the blurColor by the sum of weights
    blurColor /= weightSum;

    FragColor = blurColor;
}
