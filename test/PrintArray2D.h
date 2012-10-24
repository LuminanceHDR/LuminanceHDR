#ifndef PRINTARRAY2D_H
#define PRINTARRAY2D_H

#include <iostream>
#include "Libpfs/array2d.h"

void print(const pfs::Array2D& a2d)
{
    using namespace pfs;

    for (int r = 0; r < a2d.getRows(); r++)
    {
        Array2D::ConstIterator currSample = a2d.beginRow(r);
        Array2D::ConstIterator endSample = a2d.endRow(r);

        std::cout << "[";
        while (currSample != endSample)
        {
            std::cout << " " << *currSample++ << " ";
        }
        std::cout << "]" << std::endl;
    }
}

#endif // PRINTARRAY2D_H
