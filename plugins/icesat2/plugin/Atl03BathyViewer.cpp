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

/******************************************************************************
 * INCLUDES
 ******************************************************************************/

#include <cmath>
#include <numeric>
#include <algorithm>
#include <stdarg.h>

#include "core.h"
#include "h5.h"
#include "icesat2.h"
#include "GeoLib.h"
#include "RasterObject.h"

#include "Atl03BathyViewer.h"

/******************************************************************************
 * STATIC DATA
 ******************************************************************************/

const char* Atl03BathyViewer::GLOBAL_BATHYMETRY_MASK_FILE_PATH = "/data/ATL24_Mask_v5_Raster.tif";
const double Atl03BathyViewer::GLOBAL_BATHYMETRY_MASK_MAX_LAT = 84.25;
const double Atl03BathyViewer::GLOBAL_BATHYMETRY_MASK_MIN_LAT = -79.0;
const double Atl03BathyViewer::GLOBAL_BATHYMETRY_MASK_MAX_LON = 180.0;
const double Atl03BathyViewer::GLOBAL_BATHYMETRY_MASK_MIN_LON = -180.0;
const double Atl03BathyViewer::GLOBAL_BATHYMETRY_MASK_PIXEL_SIZE = 0.25;
const uint32_t Atl03BathyViewer::GLOBAL_BATHYMETRY_MASK_OFF_VALUE = 0xFFFFFFFF;

const char* Atl03BathyViewer::OBJECT_TYPE = "Atl03BathyViewer";
const char* Atl03BathyViewer::LUA_META_NAME = "Atl03BathyViewer";
const struct luaL_Reg Atl03BathyViewer::LUA_META_TABLE[] = {
    {"counts",      luaCounts},
    {NULL,          NULL}
};

/******************************************************************************
 * ATL03 READER CLASS
 ******************************************************************************/

/*----------------------------------------------------------------------------
 * luaCreate - create(<asset>, <resource>, <parms>)
 *----------------------------------------------------------------------------*/
int Atl03BathyViewer::luaCreate (lua_State* L)
{
    Asset* asset = NULL;
    BathyParms* parms = NULL;

    try
    {
        /* Get Parameters */
        asset = dynamic_cast<Asset*>(getLuaObject(L, 1, Asset::OBJECT_TYPE));
        const char* resource = getLuaString(L, 2);
        parms = dynamic_cast<BathyParms*>(getLuaObject(L, 4, BathyParms::OBJECT_TYPE));

        /* Return Reader Object */
        return createLuaObject(L, new Atl03BathyViewer(L, asset, resource, parms));
    }
    catch(const RunTimeException& e)
    {
        if(asset) asset->releaseLuaObject();
        if(parms) parms->releaseLuaObject();
        mlog(e.level(), "Error creating Atl03BathyViewer: %s", e.what());
        return returnLuaStatus(L, false);
    }
}

/*----------------------------------------------------------------------------
 * init
 *----------------------------------------------------------------------------*/
void Atl03BathyViewer::init (void)
{
}

/*----------------------------------------------------------------------------
 * Constructor
 *----------------------------------------------------------------------------*/
Atl03BathyViewer::Atl03BathyViewer (lua_State* L, Asset* _asset, const char* _resource, BathyParms* _parms):
    LuaObject(L, OBJECT_TYPE, LUA_META_NAME, LUA_META_TABLE),
    read_timeout_ms(_parms->read_timeout * 1000),
    bathyMask(NULL),
    totalPhotonsInMask(0)
{
    assert(_asset);
    assert(_resource);
    assert(_parms);

    /* Initialize Thread Count */
    threadCount = 0;

    /* Save Info */
    asset = _asset;
    resource = StringLib::duplicate(_resource);
    parms = _parms;
    bathyMask = new GeoLib::TIFFImage(NULL, GLOBAL_BATHYMETRY_MASK_FILE_PATH);

    /* Initialize Readers */
    active = true;
    numComplete = 0;
    memset(readerPid, 0, sizeof(readerPid));

    /* Read Global Resource Information */
    try
    {
        /* Create Readers */
        for(int track = 1; track <= Icesat2Parms::NUM_TRACKS; track++)
        {
            for(int pair = 0; pair < Icesat2Parms::NUM_PAIR_TRACKS; pair++)
            {
                const int gt_index = (2 * (track - 1)) + pair;
                if(parms->beams[gt_index] && (parms->track == Icesat2Parms::ALL_TRACKS || track == parms->track))
                {
                    info_t* info = new info_t;
                    info->reader = this;
                    info->track = track;
                    info->pair = pair;
                    StringLib::format(info->prefix, 7, "/gt%d%c", info->track, info->pair == 0 ? 'l' : 'r');
                    readerPid[threadCount++] = new Thread(subsettingThread, info);
                }
            }
        }

        /* Check if Readers Created */
        if(threadCount == 0)
        {
            throw RunTimeException(CRITICAL, RTE_ERROR, "No reader threads were created, invalid track specified: %d\n", parms->track);
        }
    }
    catch(const RunTimeException& e)
    {
        /* Generate Exception Record */
        mlog(e.level(), "Failure on resource %s: %s", resource, e.what());

        /* Indicate End of Data */
        signalComplete();
    }
}

/*----------------------------------------------------------------------------
 * Destructor
 *----------------------------------------------------------------------------*/
Atl03BathyViewer::~Atl03BathyViewer (void)
{
    active = false;

    for(int pid = 0; pid < threadCount; pid++)
    {
        delete readerPid[pid];
    }

    delete bathyMask;

    parms->releaseLuaObject();

    delete [] resource;

    asset->releaseLuaObject();
}

/*----------------------------------------------------------------------------
 * Region::Constructor
 *----------------------------------------------------------------------------*/
Atl03BathyViewer::Region::Region (info_t* info):
    segment_lat    (info->reader->asset, info->reader->resource, FString("%s/%s", info->prefix, "geolocation/reference_photon_lat").c_str(), &info->reader->context),
    segment_lon    (info->reader->asset, info->reader->resource, FString("%s/%s", info->prefix, "geolocation/reference_photon_lon").c_str(), &info->reader->context),
    segment_ph_cnt (info->reader->asset, info->reader->resource, FString("%s/%s", info->prefix, "geolocation/segment_ph_cnt").c_str(), &info->reader->context)
{
    /* Join Reads */
    segment_lat.join(info->reader->read_timeout_ms, true);
    segment_lon.join(info->reader->read_timeout_ms, true);
    segment_ph_cnt.join(info->reader->read_timeout_ms, true);
}

/*----------------------------------------------------------------------------
 * Region::Destructor
 *----------------------------------------------------------------------------*/
Atl03BathyViewer::Region::~Region (void)
{
}

/*----------------------------------------------------------------------------
 * subsettingThread
 *----------------------------------------------------------------------------*/
void* Atl03BathyViewer::subsettingThread (void* parm)
{
    /* Get Thread Info */
    info_t* info = static_cast<info_t*>(parm);
    Atl03BathyViewer* reader = info->reader;

    /* Initialize Count of Photons */
    long photons_in_mask = 0;

    try
    {
        /* Region of Interest */
        const Region region(info);

        /* Traverse All Segments In Dataset */
        int32_t current_segment = 0; // index into the segment rate variables
        while(reader->active && (current_segment < region.segment_ph_cnt.size))
        {
            /* Get Y Coordinate */
            const double degrees_of_latitude = region.segment_lat[current_segment] - GLOBAL_BATHYMETRY_MASK_MIN_LAT;
            const double latitude_pixels = degrees_of_latitude / GLOBAL_BATHYMETRY_MASK_PIXEL_SIZE;
            const uint32_t y = static_cast<uint32_t>(latitude_pixels);

            /* Get X Coordinate */
            const double degrees_of_longitude =  region.segment_lon[current_segment] - GLOBAL_BATHYMETRY_MASK_MIN_LON;
            const double longitude_pixels = degrees_of_longitude / GLOBAL_BATHYMETRY_MASK_PIXEL_SIZE;
            const uint32_t x = static_cast<uint32_t>(longitude_pixels);
        
            /* Get Pixel and Count Photons */
            const uint32_t pixel = reader->bathyMask->getPixel(x, y);
            if(pixel == GLOBAL_BATHYMETRY_MASK_OFF_VALUE)
            {
                photons_in_mask += region.segment_ph_cnt[current_segment];
            }
        }
    }
    catch(const RunTimeException& e)
    {
        mlog(e.level(), "Failure on resource %s track %d: %s", reader->resource, info->track, e.what());
    }

    /* Handle Global Reader Updates */
    reader->threadMut.lock();
    {
        /* Count Completion */
        reader->numComplete++;
        if(reader->numComplete == reader->threadCount)
        {
            mlog(INFO, "Completed processing resource %s", reader->resource);

            /* Sum Total */
            reader->totalPhotonsInMask += photons_in_mask;

            /* Indicate End of Data */
            reader->signalComplete();
        }
    }
    reader->threadMut.unlock();

    /* Clean Up */
    delete info;

    /* Return */
    return NULL;
}

/*----------------------------------------------------------------------------
 * luaCounts - :counts()
 *----------------------------------------------------------------------------*/
int Atl03BathyViewer::luaCounts (lua_State* L)
{
    bool status = false;
    int num_obj_to_return = 1;
    Atl03BathyViewer* lua_obj = NULL;

    try
    {
        /* Get Self */
        lua_obj = dynamic_cast<Atl03BathyViewer*>(getLuaSelf(L, 1));

        /* Create Statistics Table */
        lua_newtable(L);
        LuaEngine::setAttrInt(L, "photons_in_mask", lua_obj->totalPhotonsInMask);

        /* Set Success */
        status = true;
        num_obj_to_return = 2;
    }
    catch(const RunTimeException& e)
    {
        mlog(e.level(), "Error returning stats %s: %s", lua_obj->getName(), e.what());
    }

    /* Return Status */
    return returnLuaStatus(L, status, num_obj_to_return);
}
