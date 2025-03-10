#version 300 es
precision mediump float;

in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D heightMap;
uniform sampler2D colorMap;
uniform int mapWidth;
uniform int mapHeight;
uniform float cameraZoom;
uniform vec2 cameraPos;
uniform float cameraRot;
uniform vec2 screenSize;


void main()
{
    // Transform from screen space to world space
    vec2 worldPos = TexCoord;
    
    // Check if within map boundaries
    if (worldPos.x >= 0.0 && worldPos.x < float(mapWidth) && 
        worldPos.y >= 0.0 && worldPos.y < float(mapHeight)) {
        
        // Get integer and fractional parts
        int x = int(worldPos.x);
        int y = int(worldPos.y);
        
        // Sample height and color at this position
        // Fix: Explicit cast int to float to avoid implicit cast errors
        vec2 normalizedCoord = vec2(float(x) / float(mapWidth), float(y) / float(mapHeight));
        float height = texture(heightMap, normalizedCoord).r;
        vec4 color = texture(colorMap, normalizedCoord);
        
        // Apply height effect (make higher terrain brighter)
        float heightEffect = height * 0.3;
        color.rgb = min(color.rgb + heightEffect, 1.0);
        
        // Apply some simple shading based on position
        float edge = 0.0;
        float fx = fract(worldPos.x);
        float fy = fract(worldPos.y);
        if (fx < 0.05 || fx > 0.95 || fy < 0.05 || fy > 0.95)
            edge = 0.2;
        
        color.rgb = color.rgb * (1.0 - edge);
        
        FragColor = color;
    } else {
        // Outside map boundaries - use sky color gradient
        float gradientFactor = (TexCoord.y + 1.0) * 0.5;
        vec4 skyBottom = vec4(254.0/255.0, 245.0/255.0, 222.0/255.0, 1.0);
        vec4 skyTop = vec4((56.0 + cameraPos.x)/255.0, (47.0 + cameraPos.y)/255.0, 25.0/255.0, 1.0);
        FragColor = mix(skyBottom, skyTop, gradientFactor);
    }

    // Debug: Display colorMap in a square in the top-left corner
    float debugSize = 0.25; // Size of debug square (25% of screen width/height)
    if (TexCoord.x < debugSize && TexCoord.y > (1.0 - debugSize)) {
        // Map coordinates to [0,1] within the debug square
        float debugX = TexCoord.x / debugSize;
        float debugY = (TexCoord.y - (1.0 - debugSize)) / debugSize;
        
        // Flip y-coordinate to display texture right side up
        debugY = 1.0 - debugY;
        
        // Get direct texture lookup from colorMap
        vec2 debugCoord = vec2(debugX, debugY);
        FragColor = texture(colorMap, debugCoord);
    }

    FragColor = vec4(worldPos.x / float(mapWidth), worldPos.y / float(mapHeight), 0.0, 1.0); // Debug: Output world position for testing


    vec2 normalizedCoord = TexCoord;

    //Translate
    //Zoom
    //Rotate
    //isometric
    //Output will be 0-mapWidth, 0-mapHeight normalize by dividing by mapWidth and mapHeight

    //UnNormalize screen coordinates
    vec2 screenCoord = TexCoord * vec2(float(mapWidth), float(mapHeight));


    //Translate camera posiiton
    vec2 isoPos = cameraPos; 
    normalizedCoord.x = (screenCoord.x - isoPos.x);
    normalizedCoord.y = (screenCoord.y - isoPos.y);

    //Zoom
    normalizedCoord.x *= cameraZoom;
    normalizedCoord.y *= cameraZoom;

    //Rotate
    vec2 rotatedCoord;
    rotatedCoord.x = cos(cameraRot) * normalizedCoord.x - sin(cameraRot) * normalizedCoord.y;
    rotatedCoord.y = sin(cameraRot) * normalizedCoord.x + cos(cameraRot) * normalizedCoord.y;

    // Convert to isometric coordinates
    vec2 isoCoord;
    isoCoord.x = (sqrt(2.0) / 2.0) * normalizedCoord.x + (sqrt(6.0) / 2.0) * normalizedCoord.y;
    isoCoord.y = (sqrt(6.0) / 2.0) * normalizedCoord.y - (sqrt(2.0) / 2.0) * normalizedCoord.x; 

    //N0rmalize
    normalizedCoord.x = isoCoord.x / float(mapWidth);
    normalizedCoord.y = isoCoord.y / float(mapHeight);

    float height = texture(heightMap, normalizedCoord).r;
    vec4 color = texture(colorMap, normalizedCoord);

    height /= 100.0;

    FragColor = vec4(color.rgb, 1.0); // Debug: Output height and color for testing
}
