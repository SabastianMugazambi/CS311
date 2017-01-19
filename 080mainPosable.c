/*
@ Author:  Sabastian Mugazambi & Tore Banta
@ Date: 01/07/2017
This files includes the main function that test the 020triangle.c rasterizing
script.
Run the script like so  clang 080mainPosable.c 000pixel.o -lglfw -framework OpenGL
*/

#include <stdio.h>
#include <math.h>
#include <stdarg.h>
#include "000pixel.h"
#include "070vector.c"
#include "030matrix.c"
#include "040texture.c"

#define renVARYDIMBOUND 16
#define renVERTNUMBOUND 300

#include "050renderer.c"

#define renVARYX 0
#define renVARYY 1
#define renVARYS 2
#define renVARYT 3
#define renVARYR 4
#define renVARYG 5
#define renVARYB 6
#define renUNIFR 0
#define renUNIFG 1
#define renUNIFB 2
#define renTEXR 0
#define renTEXG 1
#define renTEXB 2

#define renATTRX 0
#define renATTRY 1
#define renATTRS 2
#define renATTRT 3
#define renATTRR 4
#define renATTRG 5
#define renATTRB 6
#define degree (M_PI/180)

double ch = 0.0;
int tria = 0;
double y_val = 0;

/* Writes the vary vector, based on the other parameters. */
void transformVertex(renRenderer *ren, double unif[], double attr[],
        double vary[]) {
    /* For now, just copy attr to varying. Baby steps. */

    double R[2][2];
    double attrXYvec [2];
    double RtimesXYvec[2];
    double T[2];

    double colP0[2] = { cos(unif[0]*degree) , sin(unif[0]*degree)};
    double colP1[2] = { (-1)*sin(unif[0]*degree) , cos(unif[0]*degree)};

    mat22Columns(colP0,colP1,R);
    //mat22Print(R);

    attrXYvec[0] = attr[renATTRX];
    attrXYvec[1] = attr[renATTRY];

    mat22Multiply(R,attrXYvec,RtimesXYvec);

    T[0] = unif[1];
    T[1] = unif[2];
    vecAdd(2,T,RtimesXYvec,vary);


    //printf("varyX: %f, varyY: %f,\n", vary[renVARYX],vary[renVARYY]);

    //vary[renVARYX] = attr[renATTRX];
    //vary[renVARYY] = attr[renATTRY];
    vary[renVARYS] = attr[renATTRS];
    vary[renVARYT] = attr[renATTRT];

}

/* Sets rgb, based on the other parameters, which are unaltered. attr is an
interpolated attribute vector. */
void colorPixel(renRenderer *ren, double unif[], texTexture *tex[],
                double vary[], double rgb[]) {
  texSample(tex[0], vary[renVARYS], vary[renVARYT]);
  rgb[0] = tex[0]->sample[renTEXR];
  rgb[1] = tex[0]->sample[renTEXG];
  rgb[2] = tex[0]->sample[renTEXB];
}

#include "080triangle.c"
#include "080mesh.c"

int filter = 0;
texTexture *tex[1];
renRenderer ren;
meshMesh mesh;


void handleKeyUp(int button, int shiftIsDown, int controlIsDown,
                 int altOptionIsDown, int superCommandIsDown) {
  if (button == 257) {
    if (filter == 0) {
      texSetFiltering(tex[0], texQUADRATIC);
      filter = 1;
    } else {
      texSetFiltering(tex[0], texNEAREST);
      filter = 0;
    }
  }
}


void draw(double ch) {
// [88.265886,364.927807] [98.882275,273.720626] [256.000000, 256.000000]
/*
  double a[4] = {88.265886,364.927807,0.0,0.0};
  double b[4] = {98.882275,273.720626,0.0,1.0};
  double c[4] = {256.000000, 256.000000, 1.0, 1.0};
*/
/*
double a[4] = {78.265886,374.927807,0.0,0.0};
double b[4] = {108.882275,263.720626,0.0,1.0};
double c[4] = {266.000000, 266.000000, 1.0, 1.0};
*/
/*
  double a[4] = {100.0, 100.0,0.0,0.0};
  double b[4] = {256.0,256.0,0.0,1.0};
  double c[4] = {300.0,300.0,1.0,1.0};
*/
  double unif[3] = {57.0, 266.0, 266.0};
  meshInitializeEllipse(&mesh, 0.0, 0.0, 100.0, 200.0, 8);
  meshRender(&mesh, &ren, unif, tex);
  //triRender(&ren, unif, tex, a, b, c);

}

void handleTimeStep(double oldTime, double newTime) {
  if (floor(newTime) - floor(oldTime) >= 1.0)
    printf("handleTimeStep: %f frames/sec\n", 1.0 / (newTime - oldTime));
  y_val = y_val + 0.01;
  ch = ch+5;
  //pixClearRGB(0.0, 0.0, 0.0);
  //draw(ch);
}

/*
@function main
@param void
@purpose This function initialises the GL window with the specified width and
hieght and then calls the
triRender function in 020triangle.c . This is being used to test if triRender
works fine.
*/
int main(void) {

  if (pixInitialize(512, 512, "Pixel Graphics") != 0)
    return 1;
  else {
    texTexture texture;
    texInitializeFile(&texture, "aliens.jpg");
    tex[0] = &texture;
    ren.attrDim = 4;
    ren.varyDim = 4;
    ren.texNum = 1;
    ren.unifDim = 3;

    draw(ch);

    pixSetTimeStepHandler(handleTimeStep);
    pixSetKeyUpHandler(handleKeyUp);

    texDestroy(tex[0]);
    meshDestroy(&mesh);
    pixRun();
    return 0;
  }
}
