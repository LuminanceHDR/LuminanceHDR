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

#include "projection.h"

#include <boost/math/constants/constants.hpp>

#include <cmath>
#include <cstring>
// #include <stdio.h>
// #include <stdlib.h>

#include "Libpfs/array2d.h"
#include "arch/math.h"

using namespace std;

ProjectionFactory ProjectionFactory::singleton(true);

PolarProjection PolarProjection::singleton(true);
CylindricalProjection CylindricalProjection::singleton(true);
AngularProjection AngularProjection::singleton(true);
MirrorBallProjection MirrorBallProjection::singleton(true);

const double EPSILON = 1e-7;

class Vector3D {
   public:
    double x, y, z;

    Vector3D(double phi, double theta) {
        x = cos(phi) * sin(theta);
        y = sin(phi) * sin(theta);
        z = cos(theta);
    }

    Vector3D(double x, double y, double z) {
        this->x = x;
        this->y = y;
        this->z = z;

        normalize();
    }

    double magnitude(void) { return sqrt(x * x + y * y + z * z); }

    void normalize(void) {
        double len = magnitude();

        x = x / len;
        y = y / len;
        z = z / len;
    }

    double dot(Vector3D *v) { return x * v->x + y * v->y + z * v->z; }

    // TODO: optimize rotations by precomputing sines and cosines
    void rotateX(double angle) {
        angle *= boost::math::double_constants::degree;

        double c = cos(angle);
        double s = sin(angle);

        double y2 = c * y + -s * z;
        double z2 = s * y + c * z;

        y = y2;
        z = z2;
    }

    void rotateY(double angle) {
        angle *= boost::math::double_constants::degree;

        double c = cos(angle);
        double s = sin(angle);

        double x2 = c * x + s * z;
        double z2 = -s * x + c * z;

        x = x2;
        z = z2;
    }

    void rotateZ(double angle) {
        angle *= boost::math::double_constants::degree;

        double c = cos(angle);
        double s = sin(angle);

        double x2 = c * x + -s * y;
        double y2 = s * x + c * y;

        x = x2;
        y = y2;
    }
};

class Point2D {
   public:
    double x, y;

    Point2D(double x, double y) {
        this->x = x;
        this->y = y;
    }
};

/// PROJECTIONFACTORY
ProjectionFactory::ProjectionFactory(bool) {}

void ProjectionFactory::registerProjection(const char *name,
                                           ProjectionCreator ptr) {
    singleton.projections[string(name)] = ptr;
}

// TODO: check this function
Projection *ProjectionFactory::getProjection(char *name) {
    char *opts;
    Projection *projection = NULL;

    if ((opts = strchr(name, '/'))) {
        *opts++ = '\0';
    }

    ProjectionCreator projectionCreator =
        singleton.projections.find(string(name))->second;

    if (projectionCreator != NULL) {
        projection = projectionCreator();

        if (opts != NULL) projection->setOptions(opts);
    }

    return projection;
}

// FIXME: Lame. Should return an iterator over the names. No time for this now.
// :/
void ProjectionFactory::listProjectionNames(void) {
    map<string, ProjectionCreator>::iterator i = singleton.projections.begin();

    while (i != singleton.projections.end()) {
        fprintf(stderr, "%s\n", (*i).first.c_str());
        ++i;
    }
}
/// END PROJECTIONFACTORY

/// MIRRORBALL
MirrorBallProjection::MirrorBallProjection(bool initialization) {
    name = "mirrorball";

    if (initialization)
        ProjectionFactory::registerProjection(name, this->create);
}

Projection *MirrorBallProjection::create() {
    return new MirrorBallProjection(false);
}

const char *MirrorBallProjection::getName(void) { return name; }

double MirrorBallProjection::getSizeRatio(void) { return 1; }

bool MirrorBallProjection::isValidPixel(double u, double v) {
    // check if we are not in a boundary region (outside a circle)
    if ((u - 0.5) * (u - 0.5) + (v - 0.5) * (v - 0.5) > 0.25)
        return false;
    else
        return true;
}

Vector3D *MirrorBallProjection::uvToDirection(double u, double v) {
    u = 2 * u - 1;
    v = 2 * v - 1;

    double phi = atan2(v, u);
    double theta = 2 * asin(sqrt(u * u + v * v));

    Vector3D *direction = new Vector3D(phi, theta);
    //     double t;

    direction->y = -direction->y;

    return direction;
}

Point2D *MirrorBallProjection::directionToUV(Vector3D *direction) {
    double u, v;

    direction->y = -direction->y;

    if (fabs(direction->x) > 0 || fabs(direction->y) > 0) {
        double distance =
            sqrt(direction->x * direction->x + direction->y * direction->y);

        double r = 0.5 * (sin(acos(direction->z) / 2)) / distance;

        u = direction->x * r + 0.5;
        v = direction->y * r + 0.5;
    } else {
        u = v = 0.5;
    }

    return new Point2D(u, v);
}
/// END MIRRORBALL

/// ANGULAR
AngularProjection::AngularProjection(bool initialization) {
    name = "angular";
    totalAngle = 360;
    if (initialization)
        ProjectionFactory::registerProjection(name, this->create);
}

Projection *AngularProjection::create() {
    AngularProjection *p = new AngularProjection(false);
    p->totalAngle = 360;

    return static_cast<Projection *>(p);
}

void AngularProjection::setOptions(char *opts) {
    char *delimiter;
    static const char *OPTION_ANGLE = "angle";

    while (*opts) {
        // fprintf(stderr,"option: %s\n", opts);
        // if(delimiter = strchr(name, '/'))
        //*delimiter++ = '\0';

        if (strncmp(opts, OPTION_ANGLE, strlen(OPTION_ANGLE)) == 0) {
            totalAngle = strtod(opts + strlen(OPTION_ANGLE) + 1, &delimiter);
            //  fprintf(stderr,"angle: %g\n", totalAngle);

            if (0 >= totalAngle || totalAngle > 360) {
                throw "error: angular projection: angle must be in (0,360] "
                      "degrees range.\n";
            }
        } else {
            throw " error: angular projection: unknown option.\n";
        }

        opts = delimiter + 1;
    }
}

const char *AngularProjection::getName(void) { return name; }

double AngularProjection::getSizeRatio(void) { return 1; }

bool AngularProjection::isValidPixel(double u, double v) {
    // check if we are not in a boundary region (outside a circle)
    if ((u - 0.5) * (u - 0.5) + (v - 0.5) * (v - 0.5) > 0.25)
        return false;
    else
        return true;
}

Vector3D *AngularProjection::uvToDirection(double u, double v) {
    u = 2 * u - 1;
    v = 2 * v - 1;

    u *= totalAngle / 360;
    v *= totalAngle / 360;

    double phi = atan2(v, u);
    double theta = boost::math::double_constants::pi * sqrt(u * u + v * v);

    Vector3D *direction = new Vector3D(phi, theta);
    //     double t;

    direction->y = -direction->y;

    return direction;
}

Point2D *AngularProjection::directionToUV(Vector3D *direction) {
    double u, v;

    direction->y = -direction->y;

    if (fabs(direction->x) > 0 || fabs(direction->y) > 0) {
        double distance =
            sqrt(direction->x * direction->x + direction->y * direction->y);

        double r =
            (boost::math::double_constants::one_div_two_pi)*acos(direction->z) /
            distance;

        u = direction->x * r + 0.5;
        v = direction->y * r + 0.5;
    } else {
        u = v = 0.5;
    }

    return new Point2D(u, v);
}
/// END ANGULAR

/// CYLINDRICAL
CylindricalProjection::CylindricalProjection(bool initialization) {
    name = "cylindrical";

    if (initialization)
        ProjectionFactory::registerProjection(name, this->create);

    pole = new Vector3D(0, 1, 0);
    equator = new Vector3D(0, 0, -1);
    cross = new Vector3D(1, 0, 0);
}

Projection *CylindricalProjection::create() {
    return new CylindricalProjection(false);
}

CylindricalProjection::~CylindricalProjection() {
    delete pole;
    delete equator;
    delete cross;
}

double CylindricalProjection::getSizeRatio(void) { return 2; }

bool CylindricalProjection::isValidPixel(double /*u*/, double /*v*/) {
    return true;
}

Vector3D *CylindricalProjection::uvToDirection(double u, double v) {
    u = 0.75 - u;

    u *= boost::math::double_constants::two_pi;

    v = acos(1 - 2 * v);

    Vector3D *direction = new Vector3D(u, v);

    double temp = direction->z;
    direction->z = direction->y;
    direction->y = temp;

    return direction;
}

Point2D *CylindricalProjection::directionToUV(Vector3D *direction) {
    double u, v;
    double lat = direction->dot(pole);

    v = (1 - lat) / 2;

    if (v < EPSILON || fabs(1 - v) < EPSILON)
        u = 0;
    else {
        double ratio = equator->dot(direction) / sin(acos(lat));

        if (ratio < -1)
            ratio = -1;
        else if (ratio > 1)
            ratio = 1;

        double lon = acos(ratio) / (boost::math::double_constants::two_pi);

        if (cross->dot(direction) < 0)
            u = lon;
        else
            u = 1 - lon;

        if (u == 1) u = 0;

        if (v == 1) v = 0;
    }

    //  if ( 0 > v || v >= 1 ) fprintf(stderr, "u: %f (%f,%f,%f)\n", v,
    //  direction->x, direction->y, direction->z);
    //  assert ( -0. <= u && u < 1 );
    //  assert ( -0. <= v && v < 1 );
    return new Point2D(u, v);
}
/// END CYLINDRICAL

/// POLAR
PolarProjection::PolarProjection(bool initialization) {
    name = "polar";

    if (initialization)
        ProjectionFactory::registerProjection(name, this->create);

    pole = new Vector3D(0, 1, 0);
    equator = new Vector3D(0, 0, -1);
    cross = new Vector3D(1, 0, 0);
}

Projection *PolarProjection::create() { return new PolarProjection(false); }

PolarProjection::~PolarProjection() {
    delete pole;
    delete equator;
    delete cross;
}

double PolarProjection::getSizeRatio(void) { return 2; }

bool PolarProjection::isValidPixel(double /*u*/, double /*v*/) { return true; }

Vector3D *PolarProjection::uvToDirection(double u, double v) {
    u = 0.75 - u;

    u *= boost::math::double_constants::two_pi;
    v *= boost::math::double_constants::pi;

    Vector3D *direction = new Vector3D(u, v);

    double temp = direction->z;
    direction->z = direction->y;
    direction->y = temp;

    return direction;
}

Point2D *PolarProjection::directionToUV(Vector3D *direction) {
    double u, v;
    double lat = acos(direction->dot(pole));

    v = lat * (1 / boost::math::double_constants::pi);

    if (v < EPSILON || fabs(1 - v) < EPSILON)
        u = 0;
    else {
        double ratio = equator->dot(direction) / sin(lat);

        if (ratio < -1)
            ratio = -1;
        else if (ratio > 1)
            ratio = 1;

        double lon = acos(ratio) / (boost::math::double_constants::two_pi);

        if (cross->dot(direction) < 0)
            u = lon;
        else
            u = 1 - lon;

        if (u == 1) u = 0;

        if (v == 1) v = 0;
    }

    //  if ( 0 > v || v >= 1 ) fprintf(stderr, "u: %f (%f,%f,%f)\n", v,
    //  direction->x, direction->y, direction->z);
    //  assert ( -0. <= u && u < 1 );
    //  assert ( -0. <= v && v < 1 );
    return new Point2D(u, v);
}
/// END POLAR

void transformArray(const pfs::Array2Df *in, pfs::Array2Df *out,
                    TransformInfo *transformInfo) {
    const double delta = 1. / transformInfo->oversampleFactor;
    const double offset = 0.5 / transformInfo->oversampleFactor;
    const double scaler = 1. / (transformInfo->oversampleFactor *
                                transformInfo->oversampleFactor);

    const int outRows = out->getRows();
    const int outCols = out->getCols();

    const int inRows = in->getRows();
    const int inCols = in->getCols();

    for (int y = 0; y < outRows; y++)
        for (int x = 0; x < outCols; x++) {
            double pixVal = 0;

            if (transformInfo->dstProjection->isValidPixel(
                    (x + 0.5) / outCols, (y + 0.5) / outCols) == true) {
                for (double sy = 0, oy = 0;
                     oy < transformInfo->oversampleFactor; sy += delta, oy++)
                    for (double sx = 0, ox = 0;
                         ox < transformInfo->oversampleFactor;
                         sx += delta, ox++) {
                        Vector3D *direction =
                            transformInfo->dstProjection->uvToDirection(
                                (x + offset + sx) / outCols,
                                (y + offset + sy) / outRows);

                        if (direction == NULL) continue;

                        // angles below are negated, because we want to rotate
                        // the environment around us, not us within the
                        // environment.
                        if (transformInfo->xRotate != 0)
                            direction->rotateX(-transformInfo->xRotate);

                        if (transformInfo->yRotate != 0)
                            direction->rotateY(-transformInfo->yRotate);

                        if (transformInfo->zRotate != 0)
                            direction->rotateZ(-transformInfo->zRotate);

                        Point2D *p =
                            transformInfo->srcProjection->directionToUV(
                                direction);

                        p->x *= inCols;
                        p->y *= inRows;

                        if (transformInfo->interpolate == true) {
                            int ix = (int)floor(p->x);
                            int iy = (int)floor(p->y);

                            double i = p->x - ix;
                            double j = p->y - iy;

                            // compute pixel weights for interpolation
                            double w1 = i * j;
                            double w2 = (1 - i) * j;
                            double w3 = (1 - i) * (1 - j);
                            double w4 = i * (1 - j);

                            int dx = ix + 1;
                            if (dx >= inCols) dx = inCols - 1;

                            int dy = iy + 1;
                            if (dy >= inRows) dy = inRows - 1;

                            pixVal += w3 * (*in)(ix, iy) + w4 * (*in)(dx, iy) +
                                      w1 * (*in)(dx, dy) + w2 * (*in)(ix, dy);
                        } else {
                            int ix = (int)floor(p->x + 0.5);
                            int iy = (int)floor(p->y + 0.5);

                            if (ix >= inCols) ix = inCols - 1;

                            if (iy >= inRows) iy = inRows - 1;

                            pixVal += (*in)(ix, iy);
                        }

                        delete direction;
                        delete p;

                        (*out)(x, y) = pixVal * scaler;
                    }
            }
        }
}
