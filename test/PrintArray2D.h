#ifndef PRINTARRAY2D_H
#define PRINTARRAY2D_H

#include <iostream>
#include "Libpfs/array2d.h"

void print(const pfs::Array2Df& a2d)
{
    using namespace pfs;

    for (int r = 0; r < a2d.getRows(); r++)
    {
        Array2Df::const_iterator currSample = a2d.row_begin(r);
        Array2Df::const_iterator endSample = a2d.row_end(r);

        std::cout << "[";
        while (currSample != endSample)
        {
            std::cout << " " << *currSample++ << " ";
        }
        std::cout << "]" << std::endl;
    }
}

#endif // PRINTARRAY2D_H
