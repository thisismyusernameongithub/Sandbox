#version 300 es
/*
 * Isometric Height Map Renderer
 * 
 * This fragment shader renders a 2D height map in an isometric perspective.
 * It uses parallax mapping to create the illusion of depth by sampling heights
 * along the view direction and finding where the ray intersects with the height field.
 * The shader handles camera transformations including position, rotation and zoom.
 */

precision mediump float;
precision mediump int; // Ensure int precision matches float precision

// Input from vertex shader
in vec2 TexCoord;     // Texture coordinates (0,0 to 1,1)
in vec3 FragPos;      // Fragment position in world space
out vec4 FragColor;   // Output color

// Uniform variables
uniform sampler2D heightMap;   // Texture containing height data
uniform sampler2D colorMap;    // Texture containing color/albedo data
uniform int mapWidth;          // Width of the map in world units
uniform int mapHeight;         // Height of the map in world units
uniform float cameraZoom;      // Camera zoom factor (1.0 = no zoom)
uniform vec2 cameraPos;        // Camera position in world space
uniform float cameraRot;       // Camera rotation in radians
uniform vec2 screenSize;       // Size of the screen in pixels
uniform float heightScale;     // Scale factor for height values

/**
 * Transforms screen coordinates to world coordinates
 * 
 * @param screenPos - Position in screen space (pixels)
 * @return Position in world space
 */
vec2 screen2World(vec2 screenPos)
{
    // Convert to isometric coordinates
    // The isometric projection uses a 30-degree angle which leads to these
    // sqrt(2) and sqrt(6) constants in the transformation matrix
    vec2 isoCoord;
    isoCoord.x = (sqrt(2.0) / 2.0) * screenPos.x + (sqrt(6.0) / 2.0) * screenPos.y;
    isoCoord.y = (sqrt(6.0) / 2.0) * screenPos.y - (sqrt(2.0) / 2.0) * screenPos.x; 

    // Apply camera translation
    vec2 translatedCoord; 
    translatedCoord.x = (isoCoord.x + cameraPos.x);
    translatedCoord.y = (isoCoord.y + cameraPos.y);

    // Apply camera rotation
    vec2 rotatedCoord;
    rotatedCoord.x = cos(cameraRot) * translatedCoord.x - sin(cameraRot) * translatedCoord.y;
    rotatedCoord.y = sin(cameraRot) * translatedCoord.x + cos(cameraRot) * translatedCoord.y;

    // Apply camera zoom
    vec2 zoomedCoord;
    zoomedCoord.x = rotatedCoord.x * cameraZoom;
    zoomedCoord.y = rotatedCoord.y * cameraZoom;

    return zoomedCoord;
}

/**
 * Transforms world coordinates to screen coordinates
 * The inverse of screen2World function
 * 
 * @param worldPos - Position in world space
 * @return Position in screen space (pixels)
 */
vec2 world2screen(vec2 worldPos)
{
    // Inverse of the screen2World transformation
    
    // Unzoom
    vec2 unzoomedCoord;
    unzoomedCoord.x = worldPos.x / cameraZoom;
    unzoomedCoord.y = worldPos.y / cameraZoom;
    
    // Inverse rotation (negative angle)
    vec2 unrotatedCoord;
    unrotatedCoord.x =  cos(cameraRot) * unzoomedCoord.x + sin(cameraRot) * unzoomedCoord.y;
    unrotatedCoord.y = -sin(cameraRot) * unzoomedCoord.x + cos(cameraRot) * unzoomedCoord.y;
    
    // Inverse translation
    vec2 untranslatedCoord;
    untranslatedCoord.x = unrotatedCoord.x - cameraPos.x;
    untranslatedCoord.y = unrotatedCoord.y - cameraPos.y;
    
    // Convert from isometric to screen coordinates
    vec2 screenCoord;
    // Apply inverse isometric transformation
    screenCoord.x = (untranslatedCoord.x - untranslatedCoord.y) / sqrt(2.0);
    screenCoord.y = (untranslatedCoord.x + untranslatedCoord.y) / sqrt(6.0);
    
    return screenCoord;
}


vec4 getSkyColor(float y) {
    float gradientFactor = (y + 1.0) * 0.5;
    vec4 skyBottom = vec4(254.0/255.0, 245.0/255.0, 222.0/255.0, 1.0);
    vec4 skyTop = vec4(56.0/255.0, 47.0/255.0, 25.0/255.0, 1.0);
    return mix(skyBottom, skyTop, gradientFactor);
}

/**
 * Main shader entry point
 * Performs parallax mapping to render the heightmap with depth
 */
void main()
{

    //UnNormalize screen coordinates
    vec2 screenCoord = TexCoord; // Our vertex shader draws the texture as a fullscreen quad, thus its coordinates match the screen space coordinates
    screenCoord.y = 1.0 - screenCoord.y; // Flip y-coordinate to match OpenGL's coordinate system
    screenCoord *= screenSize; // Convert from [0-1] to [0-screenSize]

    vec2 worldCoord = screen2World(screenCoord);

    //Normalize to map coordinates
    vec2 normalizedCoord;
    normalizedCoord.x = worldCoord.x / float(mapWidth);
    normalizedCoord.y = worldCoord.y / float(mapHeight);

    // Calculate the "up" direction in world coordinates for ray marching
    vec2 topLeftOfScreen = screen2World(vec2(0.0, 0.0));
    vec2 bottomLeftOfScreen = screen2World(vec2(0.0, screenSize.y));
    vec2 upVec = normalize(topLeftOfScreen - bottomLeftOfScreen);

    // Sample height at current position
    float height = 0.0;
    vec2 currentCoord = worldCoord;

    vec4 resultColor = vec4(1.0);
    bool outsideMap = true;

    vec2 stl = world2screen(vec2(0.0, 0.0));
    vec2 str = world2screen(vec2(float(mapWidth), 0.0));
    vec2 sbl = world2screen(vec2(0.0, float(mapHeight)));
    vec2 sbr = world2screen(vec2(float(mapWidth), float(mapHeight)));

    float minX = min(min(stl.x, str.x), min(sbl.x, sbr.x));
    float maxX = max(max(stl.x, str.x), max(sbl.x, sbr.x));

    
    if(screenCoord.x < minX || screenCoord.x > maxX){
        resultColor = getSkyColor(TexCoord.y);
        FragColor = resultColor;
        return;
    }

    for(int i = 0; i < 200; i++){
        vec2 newCoord = currentCoord - (upVec * float(i));

        vec2 newCoordNorm;
        newCoordNorm.x = newCoord.x / float(mapWidth);
        newCoordNorm.y = newCoord.y / float(mapHeight);

        // If new coordinates are below map then we can stop sampling because there is nothing left to sample
        if( ((upVec.x > 0.0) && (newCoordNorm.x < 0.0)) || 
            ((upVec.x < 0.0) && (newCoordNorm.x > 1.0)) || 
            ((upVec.y > 0.0) && (newCoordNorm.y < 0.0)) || 
            ((upVec.y < 0.0) && (newCoordNorm.y > 1.0)) ){
            // resultColor = vec4(0.0, 1.0, 0.0, 1.0); // Debug: Output green color for out-of-bounds
            break;
        }

        if( ((newCoordNorm.x > 0.0)) && 
            ((newCoordNorm.x < 1.0)) && 
            ((newCoordNorm.y > 0.0)) && 
            ((newCoordNorm.y < 1.0)) ){

            height = texelFetch(heightMap, ivec2(newCoord), 0).r / (cameraZoom * sqrt(2.0));
            vec2 newScreen = world2screen(newCoord);
            if(newScreen.y - height < screenCoord.y){
                resultColor = texelFetch(colorMap, ivec2(newCoord), 0);
                outsideMap = false;
            }
        }
    }

    if(outsideMap == true){
        resultColor = getSkyColor(TexCoord.y);
    }

    FragColor = resultColor;

}
