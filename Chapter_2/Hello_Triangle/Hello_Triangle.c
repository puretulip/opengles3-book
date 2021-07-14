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
      out vec4 fragColor;

      float DigitBin( const int x )
      {
         return x==0?480599.0:x==1?139810.0:x==2?476951.0:x==3?476999.0:x==4?350020.0:x==5?464711.0:x==6?464727.0:x==7?476228.0:x==8?481111.0:x==9?481095.0:0.0;
      }

      float PrintValue( vec2 vStringCoords, float fValue, float fMaxDigits, float fDecimalPlaces )
      {
         if ((vStringCoords.y < 0.0) || (vStringCoords.y >= 1.0)) return 0.0;

         bool bNeg = ( fValue < 0.0 );
      	fValue = abs(fValue);

      	float fLog10Value = log2(abs(fValue)) / log2(10.0);
      	float fBiggestIndex = max(floor(fLog10Value), 0.0);
      	float fDigitIndex = fMaxDigits - floor(vStringCoords.x);
      	float fCharBin = 0.0;
      	if(fDigitIndex > (-fDecimalPlaces - 1.01)) {
      		if(fDigitIndex > fBiggestIndex) {
      			if((bNeg) && (fDigitIndex < (fBiggestIndex+1.5))) fCharBin = 1792.0;
      		} else {
      			if(fDigitIndex == -1.0) {
      				if(fDecimalPlaces > 0.0) fCharBin = 2.0;
      			} else {
                      float fReducedRangeValue = fValue;
                      if(fDigitIndex < 0.0) { fReducedRangeValue = fract( fValue ); fDigitIndex += 1.0; }
      				float fDigitValue = (abs(fReducedRangeValue / (pow(10.0, fDigitIndex))));
                      fCharBin = DigitBin(int(floor(mod(fDigitValue, 10.0))));
      			}
              }
      	}
          return floor(mod((fCharBin / pow(2.0, floor(fract(vStringCoords.x) * 4.0) + (floor(vStringCoords.y * 5.0) * 4.0))), 2.0));
      }

      void main()
      {
         vec3 vColour = vec3(0);
         vec2 vFontSize = vec2(8.0, 15.0);
         float fDigits;
         float fDecimalPlaces;

         // Print Shader Time
      	vec2 vPixelCoord1 = vec2(96.0, 5.0);
      	float fValue1 = 222.0;
      	fDigits = 6.0;
      	float fIsDigit1 = PrintValue( (gl_FragCoord.xy - vPixelCoord1) / vFontSize, fValue1, fDigits, fDecimalPlaces);
      	vColour = mix( vColour, vec3(0.0, 1.0, 1.0), fIsDigit1);

         fragColor = vec4(vColour, 1);
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

   glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
   return TRUE;
}

///
// Draw a triangle using the shader pair created in Init()
//
void Draw(ESContext *esContext)
{
   UserData *userData = esContext->userData;
   // GLfloat vVertices[] = {0.0f, 0.5f, 0.0f,
   //                        -0.5f, -0.5f, 0.0f,
   //                        0.5f, -0.5f, 0.0f};
   // GLfloat vVertices[] = {0.0f, 1.0f, 1.0f,
   //                        -1.0f, -1.0f, 1.0f,
   //                        1.0f, -1.0f, 1.0f};

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
   // glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, vVertices);
   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, vVertices);
   glEnableVertexAttribArray(0);

   // glDrawArrays(GL_TRIANGLES, 0, 3);
   glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices);
}

void Shutdown(ESContext *esContext)
{
   UserData *userData = esContext->userData;

   glDeleteProgram(userData->programObject);
}

int esMain(ESContext *esContext)
{
   esContext->userData = malloc(sizeof(UserData));

   esCreateWindow(esContext, "Hello Triangle", 600, 600, ES_WINDOW_RGB);

   if (!Init(esContext))
   {
      return GL_FALSE;
   }

   esRegisterShutdownFunc(esContext, Shutdown);
   esRegisterDrawFunc(esContext, Draw);

   return GL_TRUE;
}
