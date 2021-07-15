// The MIT License (MIT)
//
// Copyright (c) 2013 Dan Ginsburg, Budirijanto Purnomo
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

//
// Book:      OpenGL(R) ES 3.0 Programming Guide, 2nd Edition
// Authors:   Dan Ginsburg, Budirijanto Purnomo, Dave Shreiner, Aaftab Munshi
// ISBN-10:   0-321-93388-5
// ISBN-13:   978-0-321-93388-1
// Publisher: Addison-Wesley Professional
// URLs:      http://www.opengles-book.com
//            http://my.safaribooksonline.com/book/animation-and-3d/9780133440133
//
// Hello_Triangle.c
//
//    This is a simple example that draws a single triangle with
//    a minimal vertex/fragment shader.  The purpose of this
//    example is to demonstrate the basic concepts of
//    OpenGL ES 3.0 rendering.
#include "esUtil.h"

typedef struct
{
   // Handle to a program object
   GLuint programObject;
   GLint timeLoc;
   GLint resolutionLoc;

   float time;
} UserData;

///
// Create a shader object, load the shader source, and
// compile the shader.
//
GLuint LoadShader(GLenum type, const char *shaderSrc)
{
   GLuint shader;
   GLint compiled;

   // Create the shader object
   shader = glCreateShader(type);

   if (shader == 0)
   {
      return 0;
   }

   // Load the shader source
   glShaderSource(shader, 1, &shaderSrc, NULL);

   // Compile the shader
   glCompileShader(shader);

   // Check the compile status
   glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

   if (!compiled)
   {
      GLint infoLen = 0;

      glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);

      if (infoLen > 1)
      {
         char *infoLog = malloc(sizeof(char) * infoLen);

         glGetShaderInfoLog(shader, infoLen, NULL, infoLog);
         esLogMessage("Error compiling shader:\n%s\n", infoLog);

         free(infoLog);
      }

      glDeleteShader(shader);
      return 0;
   }

   return shader;
}

///
// Initialize the shader and program object
//
int Init(ESContext *esContext)
{
   UserData *userData = esContext->userData;
   userData->time = 0.0f;

   char vShaderStr[] = R"(
      #version 300 es
      layout(location = 0) in vec4 vPosition;
      void main()
      {
         gl_Position = vPosition;
      }
   )";

   char *fShaderStr = R"(
#version 300 es
precision mediump float;
uniform float iTime;
uniform vec2 iResolution;

out vec4 fragColor;

#define S(x, y, z) smoothstep(x, y, z)
#define B(a, b, edge, t) S(a-edge, a+edge, t)*S(b+edge, b-edge, t)
#define sat(x) clamp(x,0.,1.)

#define streetLightCol vec3(1., .7, .3)
#define headLightCol vec3(.8, .8, 1.)
#define tailLightCol vec3(1., .1, .1)

#define HIGH_QUALITY
#define CAM_SHAKE 1.
#define LANE_BIAS .5
#define RAIN
//#define DROP_DEBUG

vec3 ro, rd;

float N(float t) {
	return fract(sin(t*10234.324)*123423.23512);
}
vec3 N31(float p) {
    //  3 out, 1 in... DAVE HOSKINS
   vec3 p3 = fract(vec3(p) * vec3(.1031,.11369,.13787));
   p3 += dot(p3, p3.yzx + 19.19);
   return fract(vec3((p3.x + p3.y)*p3.z, (p3.x+p3.z)*p3.y, (p3.y+p3.z)*p3.x));
}
float N2(vec2 p)
{	// Dave Hoskins - https://www.shadertoy.com/view/4djSRW
	vec3 p3  = fract(vec3(p.xyx) * vec3(443.897, 441.423, 437.195));
    p3 += dot(p3, p3.yzx + 19.19);
    return fract((p3.x + p3.y) * p3.z);
}


float DistLine(vec3 ro, vec3 rd, vec3 p) {
	return length(cross(p-ro, rd));
}

vec3 ClosestPoint(vec3 ro, vec3 rd, vec3 p) {
    // returns the closest point on ray r to point p
    return ro + max(0., dot(p-ro, rd))*rd;
}

float Remap(float a, float b, float c, float d, float t) {
	return ((t-a)/(b-a))*(d-c)+c;
}

float BokehMask(vec3 ro, vec3 rd, vec3 p, float size, float blur) {
	float d = DistLine(ro, rd, p);
    float m = S(size, size*(1.-blur), d);

    #ifdef HIGH_QUALITY
    m *= mix(.7, 1., S(.8*size, size, d));
    #endif

    return m;
}



float SawTooth(float t) {
    return cos(t+cos(t))+sin(2.*t)*.2+sin(4.*t)*.02;
}

float DeltaSawTooth(float t) {
    return 0.4*cos(2.*t)+0.08*cos(4.*t) - (1.-sin(t))*sin(t+cos(t));
}

vec2 GetDrops(vec2 uv, float seed, float m) {

    float t = iTime+m*30.;
   //  float t = m*30.;
    vec2 o = vec2(0.);

    #ifndef DROP_DEBUG
    uv.y += t*.05;
    #endif

    uv *= vec2(10., 2.5)*2.;
    vec2 id = floor(uv);
    vec3 n = N31(id.x + (id.y+seed)*546.3524);
    vec2 bd = fract(uv);

    vec2 uv2 = bd;

    bd -= .5;

    bd.y*=4.;

    bd.x += (n.x-.5)*.6;

    t += n.z * 6.28;
    float slide = SawTooth(t);

    float ts = 1.5;
    vec2 trailPos = vec2(bd.x*ts, (fract(bd.y*ts*2.-t*2.)-.5)*.5);

    bd.y += slide*2.;								// make drops slide down

    #ifdef HIGH_QUALITY
    float dropShape = bd.x*bd.x;
    dropShape *= DeltaSawTooth(t);
    bd.y += dropShape;								// change shape of drop when it is falling
    #endif

    float d = length(bd);							// distance to main drop

    float trailMask = S(-.2, .2, bd.y);				// mask out drops that are below the main
    trailMask *= bd.y;								// fade dropsize
    float td = length(trailPos*max(.5, trailMask));	// distance to trail drops

    float mainDrop = S(.2, .1, d);
    float dropTrail = S(.1, .02, td);

    dropTrail *= trailMask;
    o = mix(bd*mainDrop, trailPos, dropTrail);		// mix main drop and drop trail

    #ifdef DROP_DEBUG
    if(uv2.x<.02 || uv2.y<.01) o = vec2(1.);
    #endif

    return o;
}

void CameraSetup(vec2 uv, vec3 pos, vec3 lookat, float zoom, float m) {
	ro = pos;
    vec3 f = normalize(lookat-ro);
    vec3 r = cross(vec3(0., 1., 0.), f);
    vec3 u = cross(f, r);
    float t = iTime;
   //  float t = 0.0;

    vec2 offs = vec2(0.);
    #ifdef RAIN
    vec2 dropUv = uv;

    #ifdef HIGH_QUALITY
    float x = (sin(t*.1)*.5+.5)*.5;
    x = -x*x;
    float s = sin(x);
    float c = cos(x);

    mat2 rot = mat2(c, -s, s, c);

    #ifndef DROP_DEBUG
    dropUv = uv*rot;
    dropUv.x += -sin(t*.1)*.5;
    #endif
    #endif

    offs = GetDrops(dropUv, 1., m);

    #ifndef DROP_DEBUG
    offs += GetDrops(dropUv*1.4, 10., m);
    #ifdef HIGH_QUALITY
    offs += GetDrops(dropUv*2.4, 25., m);
    //offs += GetDrops(dropUv*3.4, 11.);
    //offs += GetDrops(dropUv*3., 2.);
    #endif

    float ripple = sin(t+uv.y*3.1415*30.+uv.x*124.)*.5+.5;
    ripple *= .005;
    offs += vec2(ripple*ripple, ripple);
    #endif
    #endif
    vec3 center = ro + f*zoom;
    vec3 i = center + (uv.x-offs.x)*r + (uv.y-offs.y)*u;

    rd = normalize(i-ro);
}

vec3 HeadLights(float i, float t) {
    float z = fract(-t*2.+i);
    vec3 p = vec3(-.3, .1, z*40.);
    float d = length(p-ro);

    float size = mix(.03, .05, S(.02, .07, z))*d;
    float m = 0.;
    float blur = .1;
    m += BokehMask(ro, rd, p-vec3(.08, 0., 0.), size, blur);
    m += BokehMask(ro, rd, p+vec3(.08, 0., 0.), size, blur);

    #ifdef HIGH_QUALITY
    m += BokehMask(ro, rd, p+vec3(.1, 0., 0.), size, blur);
    m += BokehMask(ro, rd, p-vec3(.1, 0., 0.), size, blur);
    #endif

    float distFade = max(.01, pow(1.-z, 9.));

    blur = .8;
    size *= 2.5;
    float r = 0.;
    r += BokehMask(ro, rd, p+vec3(-.09, -.2, 0.), size, blur);
    r += BokehMask(ro, rd, p+vec3(.09, -.2, 0.), size, blur);
    r *= distFade*distFade;

    return headLightCol*(m+r)*distFade;
}


vec3 TailLights(float i, float t) {
    t = t*1.5+i;

    float id = floor(t)+i;
    vec3 n = N31(id);

    float laneId = S(LANE_BIAS, LANE_BIAS+.01, n.y);

    float ft = fract(t);

    float z = 3.-ft*3.;						// distance ahead

    laneId *= S(.2, 1.5, z);				// get out of the way!
    float lane = mix(.6, .3, laneId);
    vec3 p = vec3(lane, .1, z);
    float d = length(p-ro);

    float size = .05*d;
    float blur = .1;
    float m = BokehMask(ro, rd, p-vec3(.08, 0., 0.), size, blur) +
    			BokehMask(ro, rd, p+vec3(.08, 0., 0.), size, blur);

    #ifdef HIGH_QUALITY
    float bs = n.z*3.;						// start braking at random distance
    float brake = S(bs, bs+.01, z);
    brake *= S(bs+.01, bs, z-.5*n.y);		// n.y = random brake duration

    m += (BokehMask(ro, rd, p+vec3(.1, 0., 0.), size, blur) +
    	BokehMask(ro, rd, p-vec3(.1, 0., 0.), size, blur))*brake;
    #endif

    float refSize = size*2.5;
    m += BokehMask(ro, rd, p+vec3(-.09, -.2, 0.), refSize, .8);
    m += BokehMask(ro, rd, p+vec3(.09, -.2, 0.), refSize, .8);
    vec3 col = tailLightCol*m*ft;

    float b = BokehMask(ro, rd, p+vec3(.12, 0., 0.), size, blur);
    b += BokehMask(ro, rd, p+vec3(.12, -.2, 0.), refSize, .8)*.2;

    vec3 blinker = vec3(1., .7, .2);
    blinker *= S(1.5, 1.4, z)*S(.2, .3, z);
    blinker *= sat(sin(t*200.)*100.);
    blinker *= laneId;
    col += blinker*b;

    return col;
}

vec3 StreetLights(float i, float t) {
	 float side = sign(rd.x);
    float offset = max(side, 0.)*(1./16.);
    float z = fract(i-t+offset);
    vec3 p = vec3(2.*side, 2., z*60.);
    float d = length(p-ro);
	float blur = .1;
    vec3 rp = ClosestPoint(ro, rd, p);
    float distFade = Remap(1., .7, .1, 1.5, 1.-pow(1.-z,6.));
    distFade *= (1.-z);
    float m = BokehMask(ro, rd, p, .05*d, blur)*distFade;

    return m*streetLightCol;
}

vec3 EnvironmentLights(float i, float t) {
	float n = N(i+floor(t));

    float side = sign(rd.x);
    float offset = max(side, 0.)*(1./16.);
    float z = fract(i-t+offset+fract(n*234.));
    float n2 = fract(n*100.);
    vec3 p = vec3((3.+n)*side, n2*n2*n2*1., z*60.);
    float d = length(p-ro);
	float blur = .1;
    vec3 rp = ClosestPoint(ro, rd, p);
    float distFade = Remap(1., .7, .1, 1.5, 1.-pow(1.-z,6.));
    float m = BokehMask(ro, rd, p, .05*d, blur);
    m *= distFade*distFade*.5;

    m *= 1.-pow(sin(z*6.28*20.*n)*.5+.5, 20.);
    vec3 randomCol = vec3(fract(n*-34.5), fract(n*4572.), fract(n*1264.));
    vec3 col = mix(tailLightCol, streetLightCol, fract(n*-65.42));
    col = mix(col, randomCol, n);
    return m*col*.2;
}

void main()
{
	float t = iTime;
	// float t = 0.0;
    vec3 col = vec3(0.);
   //  vec2 iResolution = vec2(1280.f, 720.f);
    vec2 uv = gl_FragCoord.xy / iResolution.xy; // 0 <> 1

    uv -= .5;
    uv.x *= iResolution.x/iResolution.y;

    // vec2 mouse = iMouse.xy/iResolution.xy;

    vec3 pos = vec3(.3, .15, 0.);

    float bt = t * 5.;
    float h1 = N(floor(bt));
    float h2 = N(floor(bt+1.));
    float bumps = mix(h1, h2, fract(bt))*.1;
    bumps = bumps*bumps*bumps*CAM_SHAKE;

    pos.y += bumps;
    float lookatY = pos.y+bumps;
    vec3 lookat = vec3(0.3, lookatY, 1.);
    vec3 lookat2 = vec3(0., lookatY, .7);
    lookat = mix(lookat, lookat2, sin(t*.1)*.5+.5);

    uv.y += bumps*4.;
    // CameraSetup(uv, pos, lookat, 2., mouse.x);
    CameraSetup(uv, pos, lookat, 2., 0.0);

    t *= .03;
    // t += mouse.x;

    // fix for GLES devices by MacroMachines
    #ifdef GL_ES
	const float stp = 1./8.;
	#else
	float stp = 1./8.
	#endif

    for(float i=0.; i<1.; i+=stp) {
       col += StreetLights(i, t);
    }

    for(float i=0.; i<1.; i+=stp) {
        float n = N(i+floor(t));
    	col += HeadLights(i+n*stp*.7, t);
    }

    #ifndef GL_ES
    #ifdef HIGH_QUALITY
    stp = 1./32.;
    #else
    stp = 1./16.;
    #endif
    #endif

    for(float i=0.; i<1.; i+=stp) {
       col += EnvironmentLights(i, t);
    }

    col += TailLights(0., t);
    col += TailLights(.5, t);

    col += sat(rd.y)*vec3(.6, .5, .9);

	fragColor = vec4(col, 0.);
}
   )";

   GLuint vertexShader;
   GLuint fragmentShader;
   GLuint programObject;
   GLint linked;

   // Load the vertex/fragment shaders
   vertexShader = LoadShader(GL_VERTEX_SHADER, vShaderStr);
   fragmentShader = LoadShader(GL_FRAGMENT_SHADER, fShaderStr);

   // Create the program object
   programObject = glCreateProgram();

   if (programObject == 0)
   {
      return 0;
   }

   glAttachShader(programObject, vertexShader);
   glAttachShader(programObject, fragmentShader);

   // Link the program
   glLinkProgram(programObject);

   // Check the link status
   glGetProgramiv(programObject, GL_LINK_STATUS, &linked);

   if (!linked)
   {
      GLint infoLen = 0;

      glGetProgramiv(programObject, GL_INFO_LOG_LENGTH, &infoLen);

      if (infoLen > 1)
      {
         char *infoLog = malloc(sizeof(char) * infoLen);

         glGetProgramInfoLog(programObject, infoLen, NULL, infoLog);
         esLogMessage("Error linking program:\n%s\n", infoLog);

         free(infoLog);
      }

      glDeleteProgram(programObject);
      return FALSE;
   }

   // Store the program object
   userData->programObject = programObject;

   glUseProgram(userData->programObject);

   userData->timeLoc = glGetUniformLocation ( userData->programObject, "iTime" );
   userData->resolutionLoc = glGetUniformLocation ( userData->programObject, "iResolution" );
   glUniform2f(userData->resolutionLoc, 1280.f, 720.f);

   glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
   return TRUE;
}

///
// Draw a triangle using the shader pair created in Init()
//
void Draw(ESContext *esContext)
{
   UserData *userData = esContext->userData;

   GLfloat vVertices[] = {
       -1.0f, 1.0f, 0.0f,  // Position 0
       -1.0f, -1.0f, 0.0f, // Position 1
       1.0f, -1.0f, 0.0f,  // Position 2
       1.0f, 1.0f, 0.0f,   // Position 3
   };
   GLushort indices[] = {0, 1, 2, 0, 2, 3};

   // Set the viewport
   glViewport(0, 0, esContext->width, esContext->height);

   // Clear the color buffer
   glClear(GL_COLOR_BUFFER_BIT);

   // Use the program object
   glUseProgram(userData->programObject);

   // Load the vertex data
   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, vVertices);
   glEnableVertexAttribArray(0);

   // glDrawArrays(GL_TRIANGLES, 0, 3);
   glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices);
}

void Update ( ESContext *esContext, float deltaTime )
{
   UserData *userData = esContext->userData;

   userData->time += deltaTime;

   glUseProgram ( userData->programObject );

   // Load uniform time variable
   glUniform1f ( userData->timeLoc, userData->time );
}

void Shutdown(ESContext *esContext)
{
   UserData *userData = esContext->userData;

   glDeleteProgram(userData->programObject);
}

int esMain(ESContext *esContext)
{
   esContext->userData = malloc(sizeof(UserData));

   esCreateWindow(esContext, "Hello Triangle", 1280, 720, ES_WINDOW_RGB);

   if (!Init(esContext))
   {
      return GL_FALSE;
   }

   esRegisterShutdownFunc(esContext, Shutdown);
   esRegisterUpdateFunc ( esContext, Update );
   esRegisterDrawFunc(esContext, Draw);

   return GL_TRUE;
}
