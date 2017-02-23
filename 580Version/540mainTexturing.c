

/* On macOS, compile with...
    clang 540mainTexturing.c -lglfw -framework OpenGL
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdarg.h>
#include <GLFW/glfw3.h>

#include "500shader.c"
#include "530vector.c"
#include "510mesh.c"
#include "520matrix.c"
#include "520camera.c"
#include "540texture.c"
#include "540scene.c"

GLdouble alpha = 0.0;
GLuint program;
GLint attrLocs[3];
GLint viewingLoc, modelingLoc;
GLint unifLocs[1];
GLint textureLoc, textureBLoc;
camCamera cam;
/* Allocate three meshes and three scene graph nodes. */
meshGLMesh rootMesh, childMesh, siblingMesh;
sceneNode rootNode, childNode, siblingNode;
texTexture *tex[2];
texTexture *texB[2];
texTexture texture, texture1, texture2;

void handleError(int error, const char *description) {
  fprintf(stderr, "handleError: %d\n%s\n", error, description);
}

void handleResize(GLFWwindow *window, int width, int height) {
  glViewport(0, 0, width, height);
  camSetWidthHeight(&cam, width, height);
}

void handleKey(GLFWwindow *window, int key, int scancode, int action,
               int mods) {
  int shiftIsDown = mods & GLFW_MOD_SHIFT;
  int controlIsDown = mods & GLFW_MOD_CONTROL;
  int altOptionIsDown = mods & GLFW_MOD_ALT;
  int superCommandIsDown = mods & GLFW_MOD_SUPER;
  if (action == GLFW_PRESS && key == GLFW_KEY_L) {
    camSwitchProjectionType(&cam);
  } else if (action == GLFW_PRESS || action == GLFW_REPEAT) {
    if (key == GLFW_KEY_O)
      camAddTheta(&cam, -0.1);
    else if (key == GLFW_KEY_P)
      camAddTheta(&cam, 0.1);
    else if (key == GLFW_KEY_I)
      camAddPhi(&cam, -0.1);
    else if (key == GLFW_KEY_K)
      camAddPhi(&cam, 0.1);
    else if (key == GLFW_KEY_U)
      camAddDistance(&cam, -0.1);
    else if (key == GLFW_KEY_J)
      camAddDistance(&cam, 0.1);
  }
}

/* Returns 0 on success, non-zero on failure. Warning: If initialization fails
midway through, then does not properly deallocate all resources. But that's
okay, because the program terminates almost immediately after this function
returns. */
int initializeScene(void) {
  /* Initialize meshes. */
  meshMesh mesh;
  // might not need to do this since its being done in tex render.
  // glActiveTexture(GL_TEXTURE0);
  // glActiveTexture(GL_TEXTURE1);
  // glEnable(GL_TEXTURE_2D);

  if (texInitializeFile(&texture, "Checkerboard_pattern.jpg", GL_LINEAR,
                        GL_LINEAR, GL_REPEAT, GL_REPEAT) != 0) {
    return 3;
  }

  if (texInitializeFile(&texture1, "box.jpg", GL_LINEAR, GL_LINEAR, GL_REPEAT,
                        GL_REPEAT) != 0) {
    return 3;
  }

  if (texInitializeFile(&texture2, "beachball.jpg", GL_LINEAR, GL_LINEAR,
                        GL_REPEAT, GL_REPEAT) != 0) {
    return 3;
  }

  tex[0] = &texture;
  tex[1] = &texture1;

  texB[0] = &texture;
  texB[1] = &texture2;

  if (meshInitializeCapsule(&mesh, 0.5, 2.0, 16, 32) != 0) return 1;
  meshGLInitialize(&rootMesh, &mesh);
  meshDestroy(&mesh);
  if (meshInitializeBox(&mesh, -0.5, 0.5, -0.5, 0.5, -0.5, 0.5) != 0) return 2;
  meshGLInitialize(&childMesh, &mesh);
  meshDestroy(&mesh);
  if (meshInitializeSphere(&mesh, 0.5, 16, 32) != 0) return 3;
  meshGLInitialize(&siblingMesh, &mesh);
  meshDestroy(&mesh);
  /* Initialize scene graph nodes. */
  if (sceneInitialize(&siblingNode, 2, 2, &siblingMesh, NULL, NULL) != 0)
    return 4;
  if (sceneInitialize(&childNode, 2, 2, &childMesh, NULL, NULL) != 0) return 5;
  if (sceneInitialize(&rootNode, 2, 2, &rootMesh, &childNode, &siblingNode) !=
      0)
    return 6;
  /* Customize the uniforms. */
  GLdouble trans[3] = {1.0, 0.0, 0.0};
  sceneSetTranslation(&childNode, trans);
  vecSet(3, trans, 0.0, 1.0, 0.0);
  sceneSetTranslation(&siblingNode, trans);
  GLdouble unif[2] = {1.0, 1.0};

  sceneSetTexture(&siblingNode, tex);
  sceneSetTexture(&childNode, tex);
  sceneSetTexture(&rootNode, tex);
  sceneSetOneTexture(&siblingNode, 1, &texture2);
  sceneSetUniform(&siblingNode, unif);
  sceneSetUniform(&childNode, unif);
  sceneSetUniform(&rootNode, unif);
  return 0;
}

void destroyScene(void) {
  meshGLDestroy(&siblingMesh);
  meshGLDestroy(&childMesh);
  meshGLDestroy(&rootMesh);
  sceneDestroyRecursively(&rootNode);
}

/* Returns 0 on success, non-zero on failure. */
int initializeShaderProgram(void) {
  GLchar vertexCode[] =
      "\
		uniform mat4 viewing;\
		uniform mat4 modeling;\
		attribute vec3 position;\
    attribute vec3 color;\
		attribute vec2 texCoords;\
		attribute vec3 normal;\
		uniform vec2 spice;\
		varying vec4 rgba;\
    varying vec2 st;\
		void main() {\
			gl_Position = viewing * modeling * vec4(position, 1.0);\
			rgba = vec4(texCoords, spice) + vec4(normal, 1.0);\
      st = texCoords;\
		}";
  GLchar fragmentCode[] =
      "\
    uniform sampler2D texture;\
    uniform sampler2D texture1;\
		varying vec4 rgba;\
    varying vec2 st;\
		void main() {\
    vec4 tex1 = texture2D(texture,st);\
    if (tex1[0] < 0.2 || tex1[1] < 0.2 || tex1[2] < 0.2) {\
      gl_FragColor = rgba * tex1;\
    }\
    else {\
      gl_FragColor = rgba * texture2D(texture1, st);\
    }\
}\
";
  program = makeProgram(vertexCode, fragmentCode);
  if (program != 0) {
    glUseProgram(program);
    attrLocs[0] = glGetAttribLocation(program, "position");
    attrLocs[1] = glGetAttribLocation(program, "texCoords");
    attrLocs[2] = glGetAttribLocation(program, "normal");
    viewingLoc = glGetUniformLocation(program, "viewing");
    modelingLoc = glGetUniformLocation(program, "modeling");
    unifLocs[0] = glGetUniformLocation(program, "spice");
    textureLoc = glGetUniformLocation(program, "texture");
    textureBLoc = glGetUniformLocation(program, "texture1");
  }
  return (program == 0);
}

void render(void) {
  /* This part is the same as in 520mainCamera.c. */
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glUseProgram(program);
  camRender(&cam, viewingLoc);
  /* This animation code is different from that in 520mainCamera.c. */
  GLdouble rot[3][3], identity[4][4], axis[3] = {1.0, 1.0, 1.0};
  vecUnit(3, axis, axis);
  alpha += 0.01;
  mat33AngleAxisRotation(alpha, axis, rot);
  sceneSetRotation(&rootNode, rot);
  sceneSetOneUniform(&rootNode, 0, 0.5 + 0.5 * sin(alpha * 7.0));
  /* This rendering code is different from that in 520mainCamera.c. */
  mat44Identity(identity);
  GLuint unifDims[1] = {2};
  GLuint attrDims[3] = {3, 2, 3};
  GLint textureLocs[2] = {textureLoc, textureBLoc};
  sceneRender(&rootNode, identity, modelingLoc, 1, unifDims, unifLocs, 3,
              attrDims, attrLocs, textureLocs);
}

int main(void) {
  glfwSetErrorCallback(handleError);
  if (glfwInit() == 0) return 1;
  GLFWwindow *window;
  window = glfwCreateWindow(512, 512, "Scene Graph", NULL, NULL);
  if (window == NULL) {
    glfwTerminate();
    return 2;
  }
  glfwSetWindowSizeCallback(window, handleResize);
  glfwSetKeyCallback(window, handleKey);
  glfwMakeContextCurrent(window);
  fprintf(stderr, "main: OpenGL %s, GLSL %s.\n", glGetString(GL_VERSION),
          glGetString(GL_SHADING_LANGUAGE_VERSION));
  glEnable(GL_DEPTH_TEST);
  glDepthRange(1.0, 0.0);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  /* Initialize a whole scene, rather than just one mesh. */
  if (initializeScene() != 0) return 3;
  if (initializeShaderProgram() != 0) return 4;

  GLdouble target[3] = {0.0, 0.0, 0.0};
  camSetControls(&cam, camPERSPECTIVE, M_PI / 6.0, 10.0, 512.0, 512.0, 10.0,
                 M_PI / 4.0, M_PI / 4.0, target);
  while (glfwWindowShouldClose(window) == 0) {

    render();
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  texDestroy(&texture);
  texDestroy(&texture1);
  glDeleteProgram(program);
  /* Don't forget to destroy the whole scene. */
  destroyScene();
  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}