/*
 * Copyright (c) 2023, University of Texas
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the University of Texas nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY OF TEXAS AND CONTRIBUTORS
 * “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE UNIVERSITY OF TEXAS OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __bathy_openoceans__
#define __bathy_openoceans__

#include "OsApi.h"
#include "BathyFields.h"

using BathyFields::extent_t;

namespace BathyOpenOceans
{
    /* Constants */
    const double DEFAULT_RI_AIR = 1.00029;
    const double DEFAULT_RI_WATER = 1.34116;

    /* Parameters */
    typedef struct {
        double  dem_buffer; // meters
        double  bin_size; // meters
        double  max_range; // meters
        long    max_bins; // bins
        double  signal_threshold_sigmas; // standard deviations
        double  min_peak_separation; // meters
        double  highest_peak_ratio;
        double  surface_width_sigmas; // standard deviations
        bool    model_as_poisson;
    } parms_t;

    /* Functions */
    void findSeaSurface (extent_t* extent, const parms_t& parms);
    void photon_refraction(extent_t& extent, double n1=DEFAULT_RI_AIR, double n2=DEFAULT_RI_WATER);
}

#endif