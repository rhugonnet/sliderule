/*
 * Copyright (c) 2021, University of Washington
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
 * 3. Neither the name of the University of Washington nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY OF WASHINGTON AND CONTRIBUTORS
 * “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE UNIVERSITY OF WASHINGTON OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __pgcdem_strips_raster__
#define __pgcdem_strips_raster__

/******************************************************************************
 * INCLUDES
 ******************************************************************************/

#include "GeoIndexedRaster.h"

/******************************************************************************
 * PGC DEM STRIPS RASTER CLASS
 ******************************************************************************/

class PgcDemStripsRaster: public GeoIndexedRaster
{
    protected:

        /*--------------------------------------------------------------------
         * Methods
         *--------------------------------------------------------------------*/

                 PgcDemStripsRaster (lua_State* L, GeoParms* _parms, const char* dem_name, const char* geo_suffix, GdalRaster::overrideCRS_t cb);
                ~PgcDemStripsRaster (void) override;
        bool     getFeatureDate     (const OGRFeature* feature, TimeLib::gmt_time_t& gmtDate) final;
        void     getIndexFile       (const OGRGeometry* geo, std::string& file, const std::vector<point_info_t>* points) final;
        bool     findRasters        (finder_t* finder) final;

    private:
        void    _getIndexFile       (double lon, double lat, std::string& file);
        bool    combineGeoJSONFiles (const std::vector<std::string>& inputFiles);

        /*--------------------------------------------------------------------
         * Data
         *--------------------------------------------------------------------*/
        std::string filePath;
        std::string demName;
        std::string path2geocells;
        std::string combinedGeoJSON;

};

#endif  /* __pgcdem_strips_raster__ */
