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

/******************************************************************************
 * INCLUDE
 ******************************************************************************/

#include "core.h"
#include "geo.h"
#include "BathyParms.h"

/******************************************************************************
 * STATIC DATA
 ******************************************************************************/

const char* BathyParms::MAX_ALONG_TRACK_SPREAD = "max_along_track_spread";
const char* BathyParms::MAX_DEM_DELTA = "max_dem_delta";
const char* BathyParms::PH_IN_EXTENT = "ph_in_extent";
const char* BathyParms::GENERATE_NDWI = "generate_ndwi";
const char* BathyParms::USE_BATHY_MASK = "use_bathy_mask";
const char* BathyParms::RETURN_INPUTS = "return_inputs";
const char* BathyParms::SPOTS = "spots";
const char* BathyParms::ATL09_RESOURCES = "resources09";

const double BathyParms::DEFAULT_MAX_ALONG_TRACK_SPREAD = 10000.0;
const double BathyParms::DEFAULT_MAX_DEM_DELTA = 10000.0;

/******************************************************************************
 * PUBLIC METHODS
 ******************************************************************************/

/*----------------------------------------------------------------------------
 * luaCreate - create(<parameter table>)
 *----------------------------------------------------------------------------*/
int BathyParms::luaCreate (lua_State* L)
{
    try
    {
        /* Check if Lua Table */
        if(lua_type(L, 1) != LUA_TTABLE)
        {
            throw RunTimeException(CRITICAL, RTE_ERROR, "Requests parameters must be supplied as a lua table");
        }

        /* Return Request Parameter Object */
        return createLuaObject(L, new BathyParms(L, 1));
    }
    catch(const RunTimeException& e)
    {
        mlog(e.level(), "Error creating %s: %s", LUA_META_NAME, e.what());
        return returnLuaStatus(L, false);
    }
}

/*----------------------------------------------------------------------------
 * getATL09Key
 *----------------------------------------------------------------------------*/
void BathyParms::getATL09Key (char* key, const char* name)
{
    // Example
    //  Name: ATL09_20230601012940_10951901_006_01.h5
    //  Key:                       10951901
    if(StringLib::size(name) != ATL09_RESOURCE_NAME_LEN)
    {
        throw RunTimeException(CRITICAL, RTE_ERROR, "Unable to process ATL09 resource name: %s", name);
    }
    StringLib::copy(key, &name[21], ATL09_RESOURCE_KEY_LEN + 1);
    key[ATL09_RESOURCE_KEY_LEN] = '\0';
}

/*----------------------------------------------------------------------------
 * luaSpotEnabled - :spoton(<spot>) --> true|false
 *----------------------------------------------------------------------------*/
int BathyParms::luaSpotEnabled (lua_State* L)
{
    bool status = false;
    BathyParms* lua_obj = NULL;

    try
    {
        lua_obj = dynamic_cast<BathyParms*>(getLuaSelf(L, 1));
        int spot = getLuaInteger(L, 2);
        if(spot >= 1 && spot <= NUM_SPOTS)
        {
            status = lua_obj->spots[spot-1];
        }
    }
    catch(const RunTimeException& e)
    {
        mlog(e.level(), "Error retrieving spot status: %s", e.what());
    }

    lua_pushboolean(L, status);
    return 1;
}

/*----------------------------------------------------------------------------
 * Constructor
 *----------------------------------------------------------------------------*/
BathyParms::BathyParms(lua_State* L, int index):
    Icesat2Parms (L, index),
    max_along_track_spread (DEFAULT_MAX_ALONG_TRACK_SPREAD),
    max_dem_delta (DEFAULT_MAX_DEM_DELTA),
    ph_in_extent (DEFAULT_PH_IN_EXTENT),
    generate_ndwi(true),
    use_bathy_mask(true),
    return_inputs(false),
    spots{true, true, true, true, true, true}
{
    bool provided = false;

    /* Set Meta Table Functions */
    luaL_getmetatable(L, LUA_META_NAME);
    LuaEngine::setAttrFunc(L, "spoton", luaSpotEnabled);
    lua_pop(L, 1);

    try
    {
        /* maximum along track spread */
        lua_getfield(L, index, BathyParms::MAX_ALONG_TRACK_SPREAD);
        max_along_track_spread = LuaObject::getLuaFloat(L, -1, true, max_along_track_spread, &provided);
        if(provided) mlog(DEBUG, "Setting %s to %lf", BathyParms::MAX_ALONG_TRACK_SPREAD, max_along_track_spread);
        lua_pop(L, 1);

        /* maximum DEM delta */
        lua_getfield(L, index, BathyParms::MAX_DEM_DELTA);
        max_dem_delta = LuaObject::getLuaFloat(L, -1, true, max_dem_delta, &provided);
        if(provided) mlog(DEBUG, "Setting %s to %lf", BathyParms::MAX_DEM_DELTA, max_dem_delta);
        lua_pop(L, 1);

        /* photons in extent */
        lua_getfield(L, index, BathyParms::PH_IN_EXTENT);
        ph_in_extent = LuaObject::getLuaInteger(L, -1, true, ph_in_extent, &provided);
        if(provided) mlog(DEBUG, "Setting %s to %d", BathyParms::PH_IN_EXTENT, ph_in_extent);
        lua_pop(L, 1);

        /* generate ndwi */
        lua_getfield(L, index, BathyParms::GENERATE_NDWI);
        generate_ndwi = LuaObject::getLuaBoolean(L, -1, true, generate_ndwi, &provided);
        if(provided) mlog(DEBUG, "Setting %s to %d", BathyParms::GENERATE_NDWI, generate_ndwi);
        lua_pop(L, 1);

        /* use bathy mask */
        lua_getfield(L, index, BathyParms::USE_BATHY_MASK);
        use_bathy_mask = LuaObject::getLuaBoolean(L, -1, true, use_bathy_mask, &provided);
        if(provided) mlog(DEBUG, "Setting %s to %d", BathyParms::USE_BATHY_MASK, use_bathy_mask);
        lua_pop(L, 1);

        /* return inputs */
        lua_getfield(L, index, BathyParms::RETURN_INPUTS);
        return_inputs = LuaObject::getLuaBoolean(L, -1, true, return_inputs, &provided);
        if(provided) mlog(DEBUG, "Setting %s to %d", BathyParms::RETURN_INPUTS, return_inputs);
        lua_pop(L, 1);

        /* atl09 resources */
        lua_getfield(L, index, BathyParms::ATL09_RESOURCES);
        get_atl09_list(L, -1, &provided);
        if(provided) mlog(DEBUG, "ATL09 resources set");
        lua_pop(L, 1);

        /* spot selection */
        lua_getfield(L, index, BathyParms::SPOTS);
        get_spot_list(L, -1, &provided);
        if(provided) mlog(DEBUG, "Spots selected");
        lua_pop(L, 1);
    }
    catch(const RunTimeException& e)
    {
        cleanup();
        throw; // rethrow exception
    }
}

/*----------------------------------------------------------------------------
 * Destructor
 *----------------------------------------------------------------------------*/
BathyParms::~BathyParms (void)
{
    cleanup();
}

/*----------------------------------------------------------------------------
 * cleanup
 *----------------------------------------------------------------------------*/
void BathyParms::cleanup (void) const
{
}

/*----------------------------------------------------------------------------
 * get_atl09_list
 *----------------------------------------------------------------------------*/
void BathyParms::get_atl09_list (lua_State* L, int index, bool* provided)
{
    /* Reset provided */
    if(provided) *provided = false;

    /* Must be table of strings */
    if(lua_istable(L, index))
    {
        /* Get number of item in table */
        int num_strings = lua_rawlen(L, index);
        if(num_strings > 0 && provided) *provided = true;

        /* Iterate through each item in table */
        for(int i = 0; i < num_strings; i++)
        {
            /* Get item */
            lua_rawgeti(L, index, i+1);
            if(lua_isstring(L, -1))
            {
                // Example
                //  Name: ATL09_20230601012940_10951901_006_01.h5
                //  Key:                       10951901
                const char* str = LuaObject::getLuaString(L, -1);
                char key[ATL09_RESOURCE_KEY_LEN + 1];
                getATL09Key(key, str); // throws on error
                string name(str);
                if(!alt09_index.add(key, name, true))
                {
                    throw RunTimeException(CRITICAL, RTE_ERROR, "Duplicate ATL09 key detected: %s", str);
                }
                mlog(DEBUG, "Adding %s to ATL09 index with key: %s", str, key);
            }
            else
            {
                mlog(ERROR, "Invalid ATL09 item specified - must be a string");
            }

            /* Clean up stack */
            lua_pop(L, 1);
        }
    }
    else if(!lua_isnil(L, index))
    {
        mlog(ERROR, "ATL09 lists must be provided as a table");
    }
}

/*----------------------------------------------------------------------------
 * get_spot_list
 *----------------------------------------------------------------------------*/
void BathyParms::get_spot_list (lua_State* L, int index, bool* provided)
{
    /* Reset Provided */
    if(provided) *provided = false;

    /* Must be table of spots or a single spot */
    if(lua_istable(L, index))
    {
        /* Clear spot table (sets all to false) */
        memset(spots, 0, sizeof(spots));
        if(provided) *provided = true;

        /* Iterate through each spot in table */
        int num_spots = lua_rawlen(L, index);
        for(int i = 0; i < num_spots; i++)
        {
            /* Get spot */
            lua_rawgeti(L, index, i+1);

            /* Set spot */
            if(lua_isinteger(L, -1))
            {
                int spot = LuaObject::getLuaInteger(L, -1);
                if(spot >= 1 && spot <= NUM_SPOTS)
                {
                    spots[spot-1] = true;
                    mlog(DEBUG, "Selecting spot %d", spot);
                }
                else
                {
                    mlog(ERROR, "Invalid spot: %d", spot); 
                }
            }

            /* Clean up stack */
            lua_pop(L, 1);
        }
    }
    else if(lua_isinteger(L, index))
    {
        /* Clear spot table (sets all to false) */
        memset(spots, 0, sizeof(spots));

        /* Set spot */
        int spot = LuaObject::getLuaInteger(L, -1);
        if(spot >= 1 && spot <= NUM_SPOTS)
        {
            spots[spot-1] = true;
            mlog(DEBUG, "Selecting spot %d", spot);
        }
        else
        {
            mlog(ERROR, "Invalid spot: %d", spot); 
        }
    }
    else if(!lua_isnil(L, index))
    {
        mlog(ERROR, "Spot selection must be provided as a table or integer");
    }
}
