// Underwater Interior Effects
// Distortion wobble and near-surface caustics
// Hrnchamd
// v1

// Compatibility: MGE XE 0
int mgeflags = 22;


texture lastshader;
texture depthframe;
texture watertexture;

sampler s0 = sampler_state { texture = <lastshader>; magfilter = linear; minfilter = linear; };
sampler s1 = sampler_state { texture = <depthframe>; magfilter = none; minfilter = none; };
sampler s2 = sampler_state { texture = <watertexture>; magfilter = linear; minfilter = linear; };


matrix mview;
matrix mproj;
float3 eyepos;
float3 eyevec;
float waterlevel;
float time;



float3 toWorld(float2 tex)
{
    float3 v = float3(mview[0][2], mview[1][2], mview[2][2]);
    v += (1/mproj[0][0] * (2*tex.x-1)).xxx * float3(mview[0][0], mview[1][0], mview[2][0]);
    v += (-1/mproj[1][1] * (2*tex.y-1)).xxx * float3(mview[0][1], mview[1][1], mview[2][1]);
    return v;
}

float4 wobble(float2 tex : TEXCOORD0) : COLOR
{
    float2 wobble = 0.01 * (2 * tex3D(s2, float3(0.25*tex, 0.2*time)).rg - 1);
    wobble *= 1 - pow(2*tex - 1, 32);
    tex += wobble;

    float4 c = tex2D(s0, tex);
    float d = tex2D(s1, tex).r;
    float3 v = eyepos + d * toWorld(tex);
    float k = 1 - tex3D(s2, float3(v.xy / 1783, 0.4 * time)).b;

    c *= 1 - 0.2 * step(v.z, waterlevel - 1) * saturate(exp((v.z - waterlevel)/100)) * k;
    return c;
}

technique T0  < string MGEinterface = "MGE XE 0"; >
{
    pass { PixelShader = compile ps_3_0 wobble(); }
}
