/*
 * This file is a part of Luminance HDR package (based on PFSTOOLS code).
 * ----------------------------------------------------------------------
 * Copyright (C) 2003-2004 Rafal Mantiuk and Grzegorz Krawczyk
 * Copyright (C) 2006-2008 Giuseppe Rota
 * Copyright (C) 2012 Davide Anastasia
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * ----------------------------------------------------------------------
 *
 */

#ifndef PFS_PROJECTION_H
#define PFS_PROJECTION_H

//! \brief Perform projective transformations of spherical images
//! \author Giuseppe Rota <grota@users.sourceforge.net>
//! \author Miloslaw Smyk, <thorgal@wfmh.org.pl>

#include <map>
#include <string>

#include "Libpfs/array2d_fwd.h"

class Vector3D;
class Point2D;

class Projection {
   protected:
    const char *name;

   public:
    virtual Vector3D *uvToDirection(double u, double v) = 0;
    virtual Point2D *directionToUV(Vector3D *direction) = 0;
    virtual bool isValidPixel(double u, double v) = 0;
    virtual double getSizeRatio(void) = 0;
    virtual ~Projection() {}

    virtual void setOptions(char *) {}

    virtual const char *getName(void) { return name; }
};

typedef Projection *(*ProjectionCreator)(void);
class ProjectionFactory {
    static ProjectionFactory singleton;
    explicit ProjectionFactory(bool);

   public:
    std::map<std::string, ProjectionCreator> projections;
    static void registerProjection(const char *name, ProjectionCreator ptr);
    static Projection *getProjection(char *name);
    static void listProjectionNames(void);
};

class MirrorBallProjection : public Projection {
    explicit MirrorBallProjection(bool initialization);

   public:
    static MirrorBallProjection singleton;
    static Projection *create();
    const char *getName(void);
    double getSizeRatio(void);
    bool isValidPixel(double u, double v);
    Vector3D *uvToDirection(double u, double v);
    Point2D *directionToUV(Vector3D *direction);
};

class AngularProjection : public Projection {
    double totalAngle;
    explicit AngularProjection(bool initialization);

   public:
    static AngularProjection singleton;
    static Projection *create();
    void setOptions(char *opts);
    const char *getName(void);
    double getSizeRatio(void);
    bool isValidPixel(double u, double v);
    Vector3D *uvToDirection(double u, double v);
    Point2D *directionToUV(Vector3D *direction);
    void setAngle(double v) { totalAngle = v; }
};

class CylindricalProjection : public Projection {
    Vector3D *pole;
    Vector3D *equator;
    Vector3D *cross;
    explicit CylindricalProjection(bool initialization);

   public:
    static CylindricalProjection singleton;
    static Projection *create();
    ~CylindricalProjection();
    double getSizeRatio(void);
    bool isValidPixel(double /*u*/, double /*v*/);
    Vector3D *uvToDirection(double u, double v);
    Point2D *directionToUV(Vector3D *direction);
};

class PolarProjection : public Projection {
    Vector3D *pole;
    Vector3D *equator;
    Vector3D *cross;
    explicit PolarProjection(bool initialization);

   public:
    static PolarProjection singleton;
    static Projection *create();
    ~PolarProjection();
    double getSizeRatio(void);
    bool isValidPixel(double /*u*/, double /*v*/);
    Vector3D *uvToDirection(double u, double v);
    Point2D *directionToUV(Vector3D *direction);
};

class TransformInfo {
   public:
    double xRotate;
    double yRotate;
    double zRotate;
    int oversampleFactor;
    bool interpolate;
    Projection *srcProjection;
    Projection *dstProjection;

    TransformInfo() {
        xRotate = yRotate = zRotate = 0;
        oversampleFactor = 1;
        interpolate = true;
        srcProjection = dstProjection = NULL;
    }
};

void transformArray(const pfs::Array2Df *in, pfs::Array2Df *out,
                    TransformInfo *transformInfo);

#endif  // PFS_PROJECTION_H
