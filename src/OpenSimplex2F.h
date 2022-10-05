#pragma once

#define PSIZE 64

typedef struct {
    int xsv, ysv;
    double dx, dy;
} LatticePoint2D;

typedef struct _LatticePoint3D {
    double dxr, dyr, dzr;
    int xrv, yrv, zrv;
    struct _LatticePoint3D *nextOnFailure;
    struct _LatticePoint3D *nextOnSuccess;
} LatticePoint3D;

typedef struct {
    int xsv, ysv, zsv, wsv;
    double dx, dy, dz, dw;
    double xsi, ysi, zsi, wsi;
    double ssiDelta;
} LatticePoint4D;

typedef struct {
    double dx, dy;
} Grad2;

typedef struct {
    double dx, dy, dz;
} Grad3;

typedef struct {
    double dx, dy, dz, dw;
} Grad4;

typedef struct {
    short perm[PSIZE];
    Grad2 permGrad2[PSIZE];
    Grad3 permGrad3[PSIZE];
    Grad4 permGrad4[PSIZE];
} OpenSimplexGradients;

typedef struct {
    Grad2 GRADIENTS_2D[PSIZE];
    Grad3 GRADIENTS_3D[PSIZE];
    Grad4 GRADIENTS_4D[PSIZE];
    LatticePoint2D LOOKUP_2D[4];
    LatticePoint3D LOOKUP_3D[8 * 8];  // 8 blocks of lattice linked lists
    LatticePoint4D VERTICES_4D[16];
} OpenSimplexEnv;

OpenSimplexEnv *initOpenSimplex();
void freeOpenSimplex(OpenSimplexEnv *ose);
OpenSimplexGradients *initOpenSimplexGradients(OpenSimplexEnv *ose, long long seed);
void freeOpenSimplexGradients(OpenSimplexGradients *osg);
double noise2(OpenSimplexEnv *ose, OpenSimplexGradients *osg, double x, double y);
double noise2_XBeforeY(OpenSimplexEnv *ose, OpenSimplexGradients *osg, double x, double y);
double noise3_Classic(OpenSimplexEnv *ose, OpenSimplexGradients *osg, double x, double y, double z);
double noise3_XYBeforeZ(OpenSimplexEnv *ose, OpenSimplexGradients *osg, double x, double y, double z);
double noise3_XZBeforeY(OpenSimplexEnv *ose, OpenSimplexGradients *osg, double x, double y, double z);
double noise4_Classic(OpenSimplexEnv *ose, OpenSimplexGradients *osg, double x, double y, double z, double w);
double noise4_XYBeforeZW(OpenSimplexEnv *ose, OpenSimplexGradients *osg, double x, double y, double z, double w);
double noise4_XZBeforeYW(OpenSimplexEnv *ose, OpenSimplexGradients *osg, double x, double y, double z, double w);
double noise4_XYZBeforeW(OpenSimplexEnv *ose, OpenSimplexGradients *osg, double x, double y, double z, double w);
