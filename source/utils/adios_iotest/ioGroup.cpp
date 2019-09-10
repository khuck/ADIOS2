/*
 * ioGroup.cpp
 *
 *  Created on: Nov 2018
 *      Author: Norbert Podhorszki
 */
#include "ioGroup.h"
#include "adios2/common/ADIOSConfig.h"

#ifdef ADIOS2_HAVE_HDF5
#include "hdf5.h"
#endif
#include "adios2/toolkit/profiling/taustubs/tautimer.hpp"

std::shared_ptr<ioGroup> createGroup(const std::string &name, IOLib iolib,
                                     adios2::ADIOS &adiosobj)
{
    TAU_SCOPED_TIMER_FUNC();
    std::shared_ptr<ioGroup> gp;
    switch (iolib)
    {
    case IOLib::ADIOS:
        gp = std::make_shared<adiosIOGroup>(name, adiosobj);
        break;
#ifdef ADIOS2_HAVE_HDF5
    case IOLib::HDF5:
        gp = std::make_shared<hdf5IOGroup>(name);
        break;
#endif
    }
    return gp;
}
