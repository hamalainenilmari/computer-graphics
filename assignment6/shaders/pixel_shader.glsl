#version 330

// control
uniform bool useNormals;
uniform bool useTextures;
uniform bool useNormalMap;
uniform bool setDiffuseToZero;
uniform bool setSpecularToZero;

// 0: with lighting, 1: diffuse texture only,
// 2: normal map texture only, 3: final normal only
// 4: GGX distribution term D, 5: GGX geometry term G
// 6: Fresnel term Fr
uniform int renderMode;

// material parameters
uniform vec4 diffuseUniform;
uniform vec3 specularUniform;
uniform mat3 normalToCamera;
uniform mat4 posToCamera;
uniform float roughness;

// texture samplers
uniform sampler2D diffuseSampler;
uniform sampler2D specularSampler;
uniform sampler2D normalSampler;

// lighting information, in camera space
uniform int  numLights;
uniform vec3 lightIntensities[3];
uniform vec3 lightDirections[3];

// interpolated inputs from vertex shader
in vec3     positionVarying;
in vec3     normalVarying;
in vec2     texCoordVarying;
in vec2     shadowUV[3]; // location of the current fragment in the lights space
in float    lightDist[3];

// output color
out vec4 outColor;

uniform sampler2D shadowSampler[3];
uniform bool shadowMaps;
uniform float shadowEps;

const float PI = 3.14159265359;

// The GGX distribution function D
// YOUR CODE HERE (R3)
float D(vec3 N, vec3 H)
{
    float cos = dot(N,H);
    if (cos <= 0) return 0.0; 
    float tan = sqrt( 1 - pow(cos,2) ) / cos;
    return ( 
    pow(roughness,2) / 
    (PI * pow(cos,4) * pow(  (pow(roughness,2) + pow(tan,2)),2  ) ) 
    );
    //return 1.;
}

// The Smith geometry term G
// YOUR CODE HERE (R4)
float G1(vec3 X, vec3 H)
{
    float cos = dot(X,H);
    if (cos <= 0.0 ) return 0.0;
    float tan = sqrt( 1.0 - pow(cos,2) ) / cos;
    return ( 
    2.0 /
    (1.0 + sqrt( 1.0 + (pow(roughness, 2) * pow(tan,2))) ) );
    //return 1.;
}

float G(vec3 V, vec3 L, vec3 H)
{
    return ( G1(V,H) * G1(L,H) );
    //return 1.;
}

// Fr is the Fresnel equation for dielectrics
// YOUR CODE HERE (R5)
float Fr(vec3 L, vec3 H)
{
    const float n1 = 1.0; // air
    const float n2 = 1.4; // surface
    float n = n1/n2;
    float cos = dot(L,H);
    float b = sqrt(1/pow(n,2) + pow(cos,2) -1);
    if (b < 0) return 1.0;
    float rs = pow( (cos - b )/(cos + b) ,2.0);
    float rp = pow( (pow(n,2) * b - cos )/( pow(n,2) * b  + cos ) ,2.0);
    return ( (1/2) * (rs + rp) );

    //return 1.0;
}

// 4: GGX distribution term D, 5: GGX geometry term G
// 6: Fresnel term Fr, 7: GGX normalization term

// Cook-Torrance BRDF:
float CookTorrance(vec3 N, vec3 H, vec3 V, vec3 L)
{
    switch (renderMode)
    {
    // debug modes..
    case 4: return (dot(N, L)>=0.0) ? D(N, H) : 0.0;
    case 5: return G(V, L, N) * 0.1;
    case 6: return (dot(N, L)>=0.0) ? Fr(L, H) : 0.0;
    default:
        // actual Cook-Torrance
        return Fr(L, H) * D(N, H) * G(V, L, N) / (4 * abs(dot(N, V) * dot(N, L)));
    }
}

void main()
{
    vec4 diffuseColor = diffuseUniform;
    vec4 specularColor = vec4(1.);

    if (useTextures)
    {
        // YOUR CODE HERE (R1)
        // Fetch the diffuse material albedos at the texture coordinates of the fragment.
        // This should be a one-liner and the same for both diffuse and specular.
        diffuseColor = texture(diffuseSampler, texCoordVarying);
        specularColor = texture(specularSampler, texCoordVarying);
    }

    // diffuse only?
    if (renderMode == 1)
    {
        outColor = vec4(diffuseColor.rgb, 1);
        return;
    }

    vec3 normal_camera = normalize(normalToCamera * normalVarying);

    if (useNormalMap)
    {
        // YOUR CODE HERE (R1)
        // Fetch the object space normal from the normal map and scale it.
        // Then transform to camera space and assign to mappedNormal.
        // Don't forget to normalize!
        vec3 normalFromTexture = vec3(.0);

        // new = -1 + normal * 2
        normalFromTexture = vec3(
        (-1 + (texture(normalSampler, texCoordVarying)[0] * 2)),
        (-1 + (texture(normalSampler, texCoordVarying)[1] * 2)),
        (-1 + (texture(normalSampler, texCoordVarying)[2] * 2))
        );

        normal_camera = normalize(normalToCamera  * normalFromTexture);
            
        // debug display: normals as read from the texture
        if (renderMode == 2)
        {
            outColor = vec4(normalFromTexture*0.5 + 0.5, 1);
            return;
        }
    }

    // debug display: camera space normals
    if (renderMode == 3)
    {
        outColor = vec4(normal_camera*0.5 + 0.5, 1);
        return;
    }

    vec3 N = normal_camera;

    // YOUR CODE HERE (R3)
    // Compute the to-viewer vector V which you'll need in the loop
    // below for the specular computation.
    //uniform mat4 posToCamera;
    //in vec3     positionVarying;

    vec3 V = vec3(.0);
    V = normalize(-vec3(
    (posToCamera * vec4(positionVarying,1.0))[0],
    (posToCamera * vec4(positionVarying,1.0))[1],
    (posToCamera * vec4(positionVarying,1.0))[2]
    ));


    // add the contribution of all lights to the answer
    vec3 answer = vec3(.0);

    for (int i = 0; i < numLights; ++i)
    {
        vec3 light_contribution = vec3(.0);

        // YOUR CODE HERE (R2)
        // Compute the diffuse shading contribution of this light.

        vec3 L = normalize(lightDirections[i]);
        vec3 Li = lightIntensities[i];
        vec3 diffuse = diffuseColor.xyz;
        float clamp = 0;

        if (dot(N,L) > 0) clamp = dot(N,L);

        light_contribution = diffuse * clamp * Li;

        // skip inactive lights also for visualization
        if (max(max(Li.r, Li.g), Li.b) == 0.0f)
            continue;

        // YOUR CODE HERE (R3, R4, R5)
        // Compute the GGX specular contribution of this light.
        vec3 specular = specularColor.xyz;

        vec3 h = normalize(L + V);
        float d = D(N,h);
        float g = G1(V, h) * G1(L, h);
        float fr = Fr(L, h);

        float Si = (fr * d * g)/(4 * abs(dot(N, V) * dot(N, L)));

        light_contribution = (diffuse + specular*Si) * clamp * Li;

        if (setDiffuseToZero)
            diffuse = vec3(0, 0, 0);

        if (setSpecularToZero)
            specular = vec3(0, 0, 0);

        if (shadowMaps)
        {
            // YOUR SHADOWS HERE: use lightDist and shadowUV, maybe modify Li
            // this point is in a shadow is some point is closer to the light than this
            // (try also adding a small value to either of those and see what happens)
            //float shadow = 1.0f; // placeholder
        }
       
        if (renderMode >= 4) // debug mode; just sum up the specular distribution for each light
            answer += CookTorrance(N,h,V,L) * vec3(specular);
        else
            answer += light_contribution;
    }
    outColor = vec4(answer, 1);
}