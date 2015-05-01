uniform sampler2D tex0, tex1, tex2, tex3;uniform vec3 ambientColor;void main(){	vec4 dcol = vec4(0.5, 0.5, 0.5, 0.5);	vec4 a = texture2D(tex0, gl_TexCoord[0].xy);	vec4 c1 = texture2D(tex1, gl_TexCoord[1].xy);	vec4 c2 = texture2D(tex2, gl_TexCoord[1].xy);	vec4 d = texture2D(tex3, gl_TexCoord[1].xy) - dcol;	vec4 finalColor = (mix(c1, c2, 0.3 * a.r + 0.59 * a.g + 0.11 * a.b) + d) * vec4(gl_Color.rgb + ambientColor, 1.0);	float z = gl_FragCoord.z / gl_FragCoord.w;	float fogFactor = clamp((gl_Fog.end - z) * gl_Fog.scale, 0.0, 1.0);	gl_FragColor = mix(gl_Fog.color, finalColor, fogFactor);}