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
 * INCLUDE
 ******************************************************************************/

#include "core.h"
#include "geo.h"
#include "Icesat2Parms.h"


/******************************************************************************
 * STATIC DATA
 ******************************************************************************/

const char* Icesat2Parms::SURFACE_TYPE                 = "srt";
const char* Icesat2Parms::ATL03_CNF                    = "cnf";
const char* Icesat2Parms::YAPC                         = "yapc";
const char* Icesat2Parms::YAPC_SCORE                   = "score";
const char* Icesat2Parms::YAPC_KNN                     = "knn";
const char* Icesat2Parms::YAPC_MIN_KNN                 = "min_knn";
const char* Icesat2Parms::YAPC_WIN_H                   = "win_h";
const char* Icesat2Parms::YAPC_WIN_X                   = "win_x";
const char* Icesat2Parms::YAPC_VERSION                 = "version";
const char* Icesat2Parms::ATL08_CLASS                  = "atl08_class";
const char* Icesat2Parms::QUALITY                      = "quality_ph";
const char* Icesat2Parms::TRACK                        = "track";
const char* Icesat2Parms::STAGES                       = "stages";
const char* Icesat2Parms::COMPACT                      = "compact";
const char* Icesat2Parms::ALONG_TRACK_SPREAD           = "ats";
const char* Icesat2Parms::MIN_PHOTON_COUNT             = "cnt";
const char* Icesat2Parms::EXTENT_LENGTH                = "len";
const char* Icesat2Parms::EXTENT_STEP                  = "res";
const char* Icesat2Parms::MAX_ITERATIONS               = "maxi";
const char* Icesat2Parms::MIN_WINDOW                   = "H_min_win";
const char* Icesat2Parms::MAX_ROBUST_DISPERSION        = "sigma_r_max";
const char* Icesat2Parms::PASS_INVALID                 = "pass_invalid";
const char* Icesat2Parms::DISTANCE_IN_SEGMENTS         = "dist_in_seg";
const char* Icesat2Parms::ATL03_GEO_FIELDS             = "atl03_geo_fields";
const char* Icesat2Parms::ATL03_PH_FIELDS              = "atl03_ph_fields";
const char* Icesat2Parms::PHOREAL                      = "phoreal";
const char* Icesat2Parms::PHOREAL_BINSIZE              = "binsize";
const char* Icesat2Parms::PHOREAL_GEOLOC               = "geoloc";
const char* Icesat2Parms::PHOREAL_USE_ABS_H            = "use_abs_h";
const char* Icesat2Parms::PHOREAL_WAVEFORM             = "send_waveform";
const char* Icesat2Parms::PHOREAL_ABOVE                = "above_classifier";

/******************************************************************************
 * PUBLIC METHODS
 ******************************************************************************/

/*----------------------------------------------------------------------------
 * luaCreate - create(<parameter table>)
 *----------------------------------------------------------------------------*/
int Icesat2Parms::luaCreate (lua_State* L)
{
    try
    {
        /* Check if Lua Table */
        if(lua_type(L, 1) != LUA_TTABLE)
        {
            throw RunTimeException(CRITICAL, RTE_ERROR, "Requests parameters must be supplied as a lua table");
        }

        /* Return Request Parameter Object */
        return createLuaObject(L, new Icesat2Parms(L, 1));
    }
    catch(const RunTimeException& e)
    {
        mlog(e.level(), "Error creating %s: %s", LuaMetaName, e.what());
        return returnLuaStatus(L, false);
    }
}

/*----------------------------------------------------------------------------
 * getSpotNumber
 *----------------------------------------------------------------------------*/
uint8_t Icesat2Parms::getSpotNumber(sc_orient_t sc_orient, track_t track, int pair)
{
    if(sc_orient == SC_BACKWARD)
    {
        if(track == RPT_1)
        {
            if(pair == Icesat2Parms::RPT_L) return SPOT_1;
            else if(pair == Icesat2Parms::RPT_R) return SPOT_2;
        }
        else if(track == RPT_2)
        {
            if(pair == Icesat2Parms::RPT_L) return SPOT_3;
            else if(pair == Icesat2Parms::RPT_R) return SPOT_4;
        }
        else if(track == RPT_3)
        {
            if(pair == Icesat2Parms::RPT_L) return SPOT_5;
            else if(pair == Icesat2Parms::RPT_R) return SPOT_6;
        }
    }
    else if(sc_orient == SC_FORWARD)
    {
        if(track == RPT_1)
        {
            if(pair == Icesat2Parms::RPT_L) return SPOT_6;
            else if(pair == Icesat2Parms::RPT_R) return SPOT_5;
        }
        else if(track == RPT_2)
        {
            if(pair == Icesat2Parms::RPT_L) return SPOT_4;
            else if(pair == Icesat2Parms::RPT_R) return SPOT_3;
        }
        else if(track == RPT_3)
        {
            if(pair == Icesat2Parms::RPT_L) return SPOT_2;
            else if(pair == Icesat2Parms::RPT_R) return SPOT_1;
        }
    }

    return 0;
}

/*----------------------------------------------------------------------------
 * getGroundTrack
 *----------------------------------------------------------------------------*/
uint8_t Icesat2Parms::getGroundTrack (sc_orient_t sc_orient, track_t track, int pair)
{
    if(sc_orient == SC_BACKWARD)
    {
        if(track == RPT_1)
        {
            if(pair == Icesat2Parms::RPT_L) return GT1L;
            else if(pair == Icesat2Parms::RPT_R) return GT1R;
        }
        else if(track == RPT_2)
        {
            if(pair == Icesat2Parms::RPT_L) return GT2L;
            else if(pair == Icesat2Parms::RPT_R) return GT2R;
        }
        else if(track == RPT_3)
        {
            if(pair == Icesat2Parms::RPT_L) return GT3L;
            else if(pair == Icesat2Parms::RPT_R) return GT3R;
        }
    }
    else if(sc_orient == SC_FORWARD)
    {
        if(track == RPT_1)
        {
            if(pair == Icesat2Parms::RPT_L) return GT1L;
            else if(pair == Icesat2Parms::RPT_R) return GT1R;
        }
        else if(track == RPT_2)
        {
            if(pair == Icesat2Parms::RPT_L) return GT2L;
            else if(pair == Icesat2Parms::RPT_R) return GT2R;
        }
        else if(track == RPT_3)
        {
            if(pair == Icesat2Parms::RPT_L) return GT3L;
            else if(pair == Icesat2Parms::RPT_R) return GT3R;
        }
    }

    return 0;
}

/*----------------------------------------------------------------------------
 * str2atl03cnf
 *----------------------------------------------------------------------------*/
Icesat2Parms::signal_conf_t Icesat2Parms::str2atl03cnf (const char* confidence_str)
{
    if     (StringLib::match(confidence_str, "atl03_tep")               ||  StringLib::match(confidence_str, "tep"))            return CNF_POSSIBLE_TEP;
    else if(StringLib::match(confidence_str, "atl03_not_considered")    ||  StringLib::match(confidence_str, "not_considered")) return CNF_NOT_CONSIDERED;
    else if(StringLib::match(confidence_str, "atl03_background")        ||  StringLib::match(confidence_str, "background"))     return CNF_BACKGROUND;
    else if(StringLib::match(confidence_str, "atl03_within_10m")        ||  StringLib::match(confidence_str, "within_10m"))     return CNF_WITHIN_10M;
    else if(StringLib::match(confidence_str, "atl03_low")               ||  StringLib::match(confidence_str, "low"))            return CNF_SURFACE_LOW;
    else if(StringLib::match(confidence_str, "atl03_medium")            ||  StringLib::match(confidence_str, "medium"))         return CNF_SURFACE_MEDIUM;
    else if(StringLib::match(confidence_str, "atl03_high")              ||  StringLib::match(confidence_str, "high"))           return CNF_SURFACE_HIGH;
    else                                                                                                                        return ATL03_INVALID_CONFIDENCE;
}

/*----------------------------------------------------------------------------
 * str2atl03quality
 *----------------------------------------------------------------------------*/
Icesat2Parms::quality_ph_t Icesat2Parms::str2atl03quality (const char* quality_ph_str)
{
    if     (StringLib::match(quality_ph_str, "atl03_quality_nominal")           || StringLib::match(quality_ph_str, "nominal"))             return QUALITY_NOMINAL;
    else if(StringLib::match(quality_ph_str, "atl03_quality_afterpulse")        || StringLib::match(quality_ph_str, "afterpulse"))          return QUALITY_POSSIBLE_AFTERPULSE;
    else if(StringLib::match(quality_ph_str, "atl03_quality_impulse_response")  || StringLib::match(quality_ph_str, "impulse_response"))    return QUALITY_POSSIBLE_IMPULSE_RESPONSE;
    else if(StringLib::match(quality_ph_str, "atl03_quality_tep")               || StringLib::match(quality_ph_str, "tep"))                 return QUALITY_POSSIBLE_TEP;
    else                                                                                                                                    return ATL03_INVALID_QUALITY;
}

/*----------------------------------------------------------------------------
 * str2atl08class
 *----------------------------------------------------------------------------*/
Icesat2Parms::atl08_classification_t Icesat2Parms::str2atl08class (const char* classifiction_str)
{
    if     (StringLib::match(classifiction_str, "atl08_noise")          || StringLib::match(classifiction_str, "noise"))           return ATL08_NOISE;
    else if(StringLib::match(classifiction_str, "atl08_ground")         || StringLib::match(classifiction_str, "ground"))          return ATL08_GROUND;
    else if(StringLib::match(classifiction_str, "atl08_canopy")         || StringLib::match(classifiction_str, "canopy"))          return ATL08_CANOPY;
    else if(StringLib::match(classifiction_str, "atl08_top_of_canopy")  || StringLib::match(classifiction_str, "top_of_canopy"))   return ATL08_TOP_OF_CANOPY;
    else if(StringLib::match(classifiction_str, "atl08_unclassified")   || StringLib::match(classifiction_str, "unclassified"))    return ATL08_UNCLASSIFIED;
    else                                                                                                                           return ATL08_INVALID_CLASSIFICATION;
}

/*----------------------------------------------------------------------------
 * str2geoloc
 *----------------------------------------------------------------------------*/
Icesat2Parms::phoreal_geoloc_t Icesat2Parms::str2geoloc (const char* fmt_str)
{
    if     (StringLib::match(fmt_str, "mean"))      return PHOREAL_MEAN;
    else if(StringLib::match(fmt_str, "median"))    return PHOREAL_MEDIAN;
    else if(StringLib::match(fmt_str, "center"))    return PHOREAL_CENTER;
    else                                            return PHOREAL_UNSUPPORTED;
}

/*----------------------------------------------------------------------------
 * deltatime2timestamp - returns nanoseconds since Unix epoch, no leap seconds
 *----------------------------------------------------------------------------*/
int64_t Icesat2Parms::deltatime2timestamp (double delta_time)
{
    return TimeLib::gps2systimeex(delta_time + (double)ATLAS_SDP_EPOCH_GPS);
}

/******************************************************************************
 * PRIVATE METHODS
 ******************************************************************************/

/*----------------------------------------------------------------------------
 * Constructor
 *----------------------------------------------------------------------------*/
Icesat2Parms::Icesat2Parms(lua_State* L, int index):
    NetsvcParms                 (L, index),
    surface_type                (SRT_LAND_ICE),
    pass_invalid                (false),
    dist_in_seg                 (false),
    compact                     (false),
    atl03_cnf                   { false, false, true, true, true, true, true },
    quality_ph                  { true, false, false, false },
    atl08_class                 { false, false, false, false, false },
    stages                      { true, false, false, false },
    yapc                        { .score      = 0,
                                  .version    = 3,
                                  .knn        = 0, // calculated by default
                                  .min_knn    = 5,
                                  .win_h      = 6.0,
                                  .win_x      = 15.0 },
    track                       (ALL_TRACKS),
    max_iterations              (5),
    minimum_photon_count        (10),
    along_track_spread          (20.0),
    minimum_window              (3.0),
    maximum_robust_dispersion   (5.0),
    extent_length               (40.0),
    extent_step                 (20.0),
    atl03_geo_fields            (NULL),
    atl03_ph_fields             (NULL),
    phoreal                     { .binsize          = 1.0,
                                  .geoloc           = PHOREAL_MEDIAN,
                                  .use_abs_h        = false,
                                  .send_waveform    = false,
                                  .above_classifier = false }
{
    bool provided = false;

    try
    {
        /* Surface Type */
        lua_getfield(L, index, Icesat2Parms::SURFACE_TYPE);
        surface_type = (surface_type_t)LuaObject::getLuaInteger(L, -1, true, surface_type, &provided);
        if(provided) mlog(DEBUG, "Setting %s to %d", Icesat2Parms::SURFACE_TYPE, (int)surface_type);
        lua_pop(L, 1);

        /* Confidence Level */
        lua_getfield(L, index, Icesat2Parms::ATL03_CNF);
        get_lua_atl03_cnf(L, -1, &provided);
        lua_pop(L, 1);

        /* Quality Flag */
        lua_getfield(L, index, Icesat2Parms::QUALITY);
        get_lua_atl03_quality(L, -1, &provided);
        lua_pop(L, 1);

        /* YAPC */
        lua_getfield(L, index, Icesat2Parms::YAPC);
        get_lua_yapc(L, -1, &provided);
        if(provided) stages[STAGE_YAPC] = true;
        lua_pop(L, 1);

        /* Pass Invalid Flag */
        lua_getfield(L, index, Icesat2Parms::PASS_INVALID);
        pass_invalid = LuaObject::getLuaBoolean(L, -1, true, pass_invalid, &provided);
        if(provided) mlog(DEBUG, "Setting %s to %s", Icesat2Parms::PASS_INVALID, pass_invalid ? "true" : "false");
        lua_pop(L, 1);

        /* Distance in Segments Flag */
        lua_getfield(L, index, Icesat2Parms::DISTANCE_IN_SEGMENTS);
        dist_in_seg = LuaObject::getLuaBoolean(L, -1, true, dist_in_seg, &provided);
        if(provided) mlog(DEBUG, "Setting %s to %s", Icesat2Parms::DISTANCE_IN_SEGMENTS, dist_in_seg ? "true" : "false");
        lua_pop(L, 1);

        /* ATL08 Classification */
        lua_getfield(L, index, Icesat2Parms::ATL08_CLASS);
        get_lua_atl08_class(L, -1, &provided);
        if(provided) stages[STAGE_ATL08] = true;
        lua_pop(L, 1);

        /* Track */
        lua_getfield(L, index, Icesat2Parms::TRACK);
        track = LuaObject::getLuaInteger(L, -1, true, track, &provided);
        if(provided) mlog(DEBUG, "Setting %s to %d", Icesat2Parms::TRACK, track);
        lua_pop(L, 1);

        /* Compact Flag */
        lua_getfield(L, index, Icesat2Parms::COMPACT);
        compact = LuaObject::getLuaBoolean(L, -1, true, compact, &provided);
        if(provided) mlog(DEBUG, "Setting %s to %s", Icesat2Parms::COMPACT, compact ? "true" : "false");
        lua_pop(L, 1);

        /* Maximum Iterations */
        lua_getfield(L, index, Icesat2Parms::MAX_ITERATIONS);
        max_iterations = LuaObject::getLuaInteger(L, -1, true, max_iterations, &provided);
        if(provided) mlog(DEBUG, "Setting %s to %d", Icesat2Parms::MAX_ITERATIONS, (int)max_iterations);
        lua_pop(L, 1);

        /* Along Track Spread */
        lua_getfield(L, index, Icesat2Parms::ALONG_TRACK_SPREAD);
        along_track_spread = LuaObject::getLuaFloat(L, -1, true, along_track_spread, &provided);
        if(provided) mlog(DEBUG, "Setting %s to %lf", Icesat2Parms::ALONG_TRACK_SPREAD, along_track_spread);
        lua_pop(L, 1);

        /* Minimum Photon Count */
        lua_getfield(L, index, Icesat2Parms::MIN_PHOTON_COUNT);
        minimum_photon_count = LuaObject::getLuaInteger(L, -1, true, minimum_photon_count, &provided);
        if(provided) mlog(DEBUG, "Setting %s to %d", Icesat2Parms::MIN_PHOTON_COUNT, minimum_photon_count);
        lua_pop(L, 1);

        /* Minimum Window */
        lua_getfield(L, index, Icesat2Parms::MIN_WINDOW);
        minimum_window = LuaObject::getLuaFloat(L, -1, true, minimum_window, &provided);
        if(provided) mlog(DEBUG, "Setting %s to %lf", Icesat2Parms::MIN_WINDOW, minimum_window);
        lua_pop(L, 1);

        /* Maximum Robust Dispersion */
        lua_getfield(L, index, Icesat2Parms::MAX_ROBUST_DISPERSION);
        maximum_robust_dispersion = LuaObject::getLuaFloat(L, -1, true, maximum_robust_dispersion, &provided);
        if(provided) mlog(DEBUG, "Setting %s to %lf", Icesat2Parms::MAX_ROBUST_DISPERSION, maximum_robust_dispersion);
        lua_pop(L, 1);

        /* Extent Length */
        lua_getfield(L, index, Icesat2Parms::EXTENT_LENGTH);
        extent_length = LuaObject::getLuaFloat(L, -1, true, extent_length, &provided);
        if(provided) mlog(DEBUG, "Setting %s to %lf", Icesat2Parms::EXTENT_LENGTH, extent_length);
        lua_pop(L, 1);

        /* Extent Step */
        lua_getfield(L, index, Icesat2Parms::EXTENT_STEP);
        extent_step = LuaObject::getLuaFloat(L, -1, true, extent_step, &provided);
        if(provided) mlog(DEBUG, "Setting %s to %lf", Icesat2Parms::EXTENT_STEP, extent_step);
        lua_pop(L, 1);

        /* ATL03 Geolocaiont and Physical Correction Fields */
        lua_getfield(L, index, Icesat2Parms::ATL03_GEO_FIELDS);
        get_lua_string_list (L, -1, &atl03_geo_fields, &provided);
        if(provided) mlog(DEBUG, "ATL03 geo field array supplied");
        lua_pop(L, 1);

        /* ATL03 Photon Fields */
        lua_getfield(L, index, Icesat2Parms::ATL03_PH_FIELDS);
        get_lua_string_list (L, -1, &atl03_ph_fields, &provided);
        if(provided) mlog(DEBUG, "ATL03 photon field array supplied");
        lua_pop(L, 1);

        /* PhoREAL */
        lua_getfield(L, index, Icesat2Parms::PHOREAL);
        get_lua_phoreal(L, -1, &provided);
        if(provided)
        {
            stages[STAGE_PHOREAL] = true;
            if(!stages[STAGE_ATL08])
            {
                stages[STAGE_ATL08] = true;
                atl08_class[ATL08_NOISE] = false;
                atl08_class[ATL08_GROUND] = true;
                atl08_class[ATL08_CANOPY] = true;
                atl08_class[ATL08_TOP_OF_CANOPY] = true;
                atl08_class[ATL08_UNCLASSIFIED] = false;
            }
        }
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
Icesat2Parms::~Icesat2Parms (void)
{
    cleanup();
}

/*----------------------------------------------------------------------------
 * cleanup
 *----------------------------------------------------------------------------*/
void Icesat2Parms::cleanup (void)
{
    if(atl03_geo_fields) delete atl03_geo_fields;
    if(atl03_ph_fields) delete atl03_ph_fields;
}

/*----------------------------------------------------------------------------
 * get_lua_atl03_cnf
 *----------------------------------------------------------------------------*/
void Icesat2Parms::get_lua_atl03_cnf (lua_State* L, int index, bool* provided)
{
    /* Reset Provided */
    if(provided) *provided = false;

    /* Must be table of classifications or a single classification as a string */
    if(lua_istable(L, index))
    {
        /* Clear confidence table (sets all to false) */
        memset(atl03_cnf, 0, sizeof(atl03_cnf));

        /* Get number of classifications in table */
        int num_cnf = lua_rawlen(L, index);
        if(num_cnf > 0 && provided) *provided = true;

        /* Iterate through each classification in table */
        for(int i = 0; i < num_cnf; i++)
        {
            /* Get confidence */
            lua_rawgeti(L, index, i+1);

            /* Set confidence */
            if(lua_isinteger(L, -1))
            {
                int confidence = LuaObject::getLuaInteger(L, -1);
                if(confidence >= CNF_POSSIBLE_TEP && confidence <= CNF_SURFACE_HIGH)
                {
                    atl03_cnf[confidence + SIGNAL_CONF_OFFSET] = true;
                    mlog(DEBUG, "Selecting confidence %d", confidence);
                }
                else
                {
                    mlog(ERROR, "Invalid ATL03 confidence: %d", confidence);
                }
            }
            else if(lua_isstring(L, -1))
            {
                const char* confidence_str = LuaObject::getLuaString(L, -1);
                signal_conf_t confidence = str2atl03cnf(confidence_str);
                if(confidence != ATL03_INVALID_CONFIDENCE)
                {
                    atl03_cnf[confidence + SIGNAL_CONF_OFFSET] = true;
                    mlog(DEBUG, "Selecting %s confidence", confidence_str);
                }
                else
                {
                    mlog(ERROR, "Invalid ATL03 confidence: %s", confidence_str);
                }
            }

            /* Clean up stack */
            lua_pop(L, 1);
        }
    }
    else if(lua_isinteger(L, index))
    {
        /* Clear confidence table (sets all to false) */
        memset(atl03_cnf, 0, sizeof(atl03_cnf));

        /* Set Confidences */
        int confidence = LuaObject::getLuaInteger(L, index);
        if(confidence >= CNF_POSSIBLE_TEP && confidence <= CNF_SURFACE_HIGH)
        {
            if(provided) *provided = true;
            for(int c = confidence; c <= CNF_SURFACE_HIGH; c++)
            {
                atl03_cnf[c + SIGNAL_CONF_OFFSET] = true;
                mlog(DEBUG, "Selecting confidence %d", c);
            }
        }
        else
        {
            mlog(ERROR, "Invalid ATL03 confidence: %d", confidence);
        }
    }
    else if(lua_isstring(L, index))
    {
        /* Clear confidence table (sets all to false) */
        memset(atl03_cnf, 0, sizeof(atl03_cnf));

        /* Set Confidences */
        const char* confidence_str = LuaObject::getLuaString(L, index);
        signal_conf_t confidence = str2atl03cnf(confidence_str);
        if(confidence != ATL03_INVALID_CONFIDENCE)
        {
            if(provided) *provided = true;
            for(int c = confidence; c <= CNF_SURFACE_HIGH; c++)
            {
                atl03_cnf[c + SIGNAL_CONF_OFFSET] = true;
                mlog(DEBUG, "Selecting confidence %d", c);
            }
        }
        else
        {
            mlog(ERROR, "Invalid ATL03 confidence: %s", confidence_str);
        }
    }
    else if(!lua_isnil(L, index))
    {
        mlog(ERROR, "ATL03 confidence must be provided as a table or string");
    }
}

/*----------------------------------------------------------------------------
 * get_lua_atl03_quality
 *----------------------------------------------------------------------------*/
void Icesat2Parms::get_lua_atl03_quality (lua_State* L, int index, bool* provided)
{
    /* Reset Provided */
    if(provided) *provided = false;

    /* Must be table of photon qualities or a single quality as a string */
    if(lua_istable(L, index))
    {
        /* Clear photon quality table (sets all to false) */
        memset(quality_ph, 0, sizeof(quality_ph));

        /* Get number of photon quality level in table */
        int num_quality = lua_rawlen(L, index);
        if(num_quality > 0 && provided) *provided = true;

        /* Iterate through each photon quality level in table */
        for(int i = 0; i < num_quality; i++)
        {
            /* Get photon quality */
            lua_rawgeti(L, index, i+1);

            /* Set photon quality */
            if(lua_isinteger(L, -1))
            {
                int quality = LuaObject::getLuaInteger(L, -1);
                if(quality >= QUALITY_NOMINAL && quality <= QUALITY_POSSIBLE_TEP)
                {
                    quality_ph[quality] = true;
                    mlog(DEBUG, "Selecting photon quality %d", quality);
                }
                else
                {
                    mlog(ERROR, "Invalid ATL03 photon quality: %d", quality);
                }
            }
            else if(lua_isstring(L, -1))
            {
                const char* quality_ph_str = LuaObject::getLuaString(L, -1);
                quality_ph_t quality = str2atl03quality(quality_ph_str);
                if(quality != ATL03_INVALID_QUALITY)
                {
                    quality_ph[quality] = true;
                    mlog(DEBUG, "Selecting %s photon quality", quality_ph_str);
                }
                else
                {
                    mlog(ERROR, "Invalid ATL03 photon quality: %s", quality_ph_str);
                }
            }

            /* Clean up stack */
            lua_pop(L, 1);
        }
    }
    else if(lua_isinteger(L, index))
    {
        /* Clear photon quality table (sets all to false) */
        memset(quality_ph, 0, sizeof(quality_ph));

        /* Set Photon Quality Level */
        int quality = LuaObject::getLuaInteger(L, index);
        if(quality >= QUALITY_NOMINAL && quality <= QUALITY_POSSIBLE_TEP)
        {
            if(provided) *provided = true;
            for(int q = quality; q <= QUALITY_POSSIBLE_TEP; q++)
            {
                quality_ph[q] = true;
                mlog(DEBUG, "Selecting photon quality %d", q);
            }
        }
        else
        {
            mlog(ERROR, "Invalid ATL03 photon quality: %d", quality);
        }
    }
    else if(lua_isstring(L, index))
    {
        /* Clear photon quality table (sets all to false) */
        memset(quality_ph, 0, sizeof(quality_ph));

        /* Set Photon Quality Level */
        const char* quality_ph_str = LuaObject::getLuaString(L, index);
        quality_ph_t quality = str2atl03quality(quality_ph_str);
        if(quality != ATL03_INVALID_QUALITY)
        {
            if(provided) *provided = true;
            for(int q = quality; q <= QUALITY_POSSIBLE_TEP; q++)
            {
                quality_ph[q] = true;
                mlog(DEBUG, "Selecting photon quality %d", q);
            }
        }
        else
        {
            mlog(ERROR, "Invalid ATL03 photon quality: %s", quality_ph_str);
        }
    }
    else if(!lua_isnil(L, index))
    {
        mlog(ERROR, "ATL03 photon quality must be provided as a table or string");
    }
}

/*----------------------------------------------------------------------------
 * get_lua_atl08_class
 *----------------------------------------------------------------------------*/
void Icesat2Parms::get_lua_atl08_class (lua_State* L, int index, bool* provided)
{
    /* Reset Provided */
    if(provided) *provided = false;

    /* Must be table of classifications or a single classification as a string */
    if(lua_istable(L, index))
    {
        /* Clear classification table (sets all to false) */
        memset(atl08_class, 0, sizeof(atl08_class));

        /* Get number of classifications in table */
        int num_classes = lua_rawlen(L, index);
        if(num_classes > 0 && provided) *provided = true;

        /* Iterate through each classification in table */
        for(int i = 0; i < num_classes; i++)
        {
            /* Get classification */
            lua_rawgeti(L, index, i+1);

            /* Set classification */
            if(lua_isinteger(L, -1))
            {
                int classification = LuaObject::getLuaInteger(L, -1);
                if(classification >= 0 && classification < NUM_ATL08_CLASSES)
                {
                    atl08_class[classification] = true;
                    mlog(DEBUG, "Selecting classification %d", classification);
                }
                else
                {
                    mlog(ERROR, "Invalid ATL08 classification: %d", classification);
                }
            }
            else if(lua_isstring(L, -1))
            {
                const char* classifiction_str = LuaObject::getLuaString(L, -1);
                atl08_classification_t atl08class = str2atl08class(classifiction_str);
                if(atl08class != ATL08_INVALID_CLASSIFICATION)
                {
                    atl08_class[atl08class] = true;
                    mlog(DEBUG, "Selecting %s classification", classifiction_str);
                }
                else
                {
                    mlog(ERROR, "Invalid ATL08 classification: %s", classifiction_str);
                }
            }

            /* Clean up stack */
            lua_pop(L, 1);
        }
    }
    else if(lua_isinteger(L, index))
    {
        /* Clear classification table (sets all to false) */
        memset(atl08_class, 0, sizeof(atl08_class));

        /* Set classification */
        int classification = LuaObject::getLuaInteger(L, -1);
        if(classification >= 0 && classification < NUM_ATL08_CLASSES)
        {
            if(provided) *provided = true;
            atl08_class[classification] = true;
            mlog(DEBUG, "Selecting classification %d", classification);
        }
        else
        {
            mlog(ERROR, "Invalid ATL08 classification: %d", classification);
        }
    }
    else if(lua_isstring(L, index))
    {
        /* Clear classification table (sets all to false) */
        memset(atl08_class, 0, sizeof(atl08_class));

        /* Set classification */
        const char* classifiction_str = LuaObject::getLuaString(L, index);
        atl08_classification_t atl08class = str2atl08class(classifiction_str);
        if(atl08class != ATL08_INVALID_CLASSIFICATION)
        {
            if(provided) *provided = true;
            atl08_class[atl08class] = true;
            mlog(DEBUG, "Selecting %s classification", classifiction_str);
        }
        else
        {
            mlog(ERROR, "Invalid ATL08 classification: %s", classifiction_str);
        }
    }
    else if(!lua_isnil(L, index))
    {
        mlog(ERROR, "ATL08 classification must be provided as a table or string");
    }
}

/*----------------------------------------------------------------------------
 * get_lua_yapc
 *----------------------------------------------------------------------------*/
void Icesat2Parms::get_lua_yapc (lua_State* L, int index, bool* provided)
{
    bool field_provided;

    /* Reset Provided */
    if(provided) *provided = false;

    /* Must be table of coordinates */
    if(lua_istable(L, index))
    {
        if(provided) *provided = true;

        lua_getfield(L, index, Icesat2Parms::YAPC_SCORE);
        yapc.score = (uint8_t)LuaObject::getLuaInteger(L, -1, true, yapc.score, &field_provided);
        if(field_provided) mlog(DEBUG, "Setting %s to %d", Icesat2Parms::YAPC_SCORE, (int)yapc.score);
        lua_pop(L, 1);

        lua_getfield(L, index, Icesat2Parms::YAPC_VERSION);
        yapc.version = (int)LuaObject::getLuaInteger(L, -1, true, yapc.version, &field_provided);
        if(field_provided) mlog(DEBUG, "Setting %s to %d", Icesat2Parms::YAPC_VERSION, (int)yapc.version);
        lua_pop(L, 1);

        lua_getfield(L, index, Icesat2Parms::YAPC_KNN);
        yapc.knn = (int)LuaObject::getLuaInteger(L, -1, true, yapc.knn, &field_provided);
        if(field_provided) mlog(DEBUG, "Setting %s to %d", Icesat2Parms::YAPC_KNN, (int)yapc.knn);
        lua_pop(L, 1);

        lua_getfield(L, index, Icesat2Parms::YAPC_MIN_KNN);
        yapc.min_knn = (int)LuaObject::getLuaInteger(L, -1, true, yapc.min_knn, &field_provided);
        if(field_provided) mlog(DEBUG, "Setting %s to %d", Icesat2Parms::YAPC_MIN_KNN, (int)yapc.min_knn);
        lua_pop(L, 1);

        lua_getfield(L, index, Icesat2Parms::YAPC_WIN_H);
        yapc.win_h = LuaObject::getLuaFloat(L, -1, true, yapc.win_h, &field_provided);
        if(field_provided) mlog(DEBUG, "Setting %s to %.3lf", Icesat2Parms::YAPC_WIN_H, yapc.win_h);
        lua_pop(L, 1);

        lua_getfield(L, index, Icesat2Parms::YAPC_WIN_X);
        yapc.win_x = LuaObject::getLuaFloat(L, -1, true, yapc.win_x, &field_provided);
        if(field_provided) mlog(DEBUG, "Setting %s to %.3lf", Icesat2Parms::YAPC_WIN_X, yapc.win_x);
        lua_pop(L, 1);
    }
}

/*----------------------------------------------------------------------------
 * get_lua_string_list
 *----------------------------------------------------------------------------*/
void Icesat2Parms::get_lua_string_list (lua_State* L, int index, string_list_t** string_list, bool* provided)
{
    /* Reset provided */
    if(provided) *provided = false;

    /* Must be table of strings */
    if(lua_istable(L, index))
    {
        /* Get number of item in table */
        int num_strings = lua_rawlen(L, index);
        if(num_strings > 0 && provided)
        {
            /* Allocate string list */
            *string_list = new string_list_t;
            *provided = true;
        }

        /* Iterate through each item in table */
        for(int i = 0; i < num_strings; i++)
        {
            /* Get item */
            lua_rawgeti(L, index, i+1);

            if(lua_isstring(L, -1))
            {
                const char* item_str = LuaObject::getLuaString(L, -1);
                SafeString item("%s", item_str);
                (*string_list)->add(item);
                mlog(DEBUG, "Adding %s to list of strings", item_str);
            }
            else
            {
                mlog(ERROR, "Invalid item specified - must be a string");
            }

            /* Clean up stack */
            lua_pop(L, 1);
        }
    }
    else if(!lua_isnil(L, index))
    {
        mlog(ERROR, "Lists must be provided as a table");
    }
}

/*----------------------------------------------------------------------------
 * get_lua_phoreal
 *----------------------------------------------------------------------------*/
void Icesat2Parms::get_lua_phoreal (lua_State* L, int index, bool* provided)
{
    bool field_provided;

    /* Reset Provided */
    if(provided) *provided = false;

    /* Must be table of coordinates */
    if(lua_istable(L, index))
    {
        if(provided) *provided = true;

        /* Binsize */
        lua_getfield(L, index, Icesat2Parms::PHOREAL_BINSIZE);
        phoreal.binsize = LuaObject::getLuaFloat(L, -1, true, phoreal.binsize, &field_provided);
        if(field_provided)
        {
            if(phoreal.binsize <= 0.0) throw RunTimeException(CRITICAL, RTE_ERROR, "Invalid binsize provided to phoreal algorithm: %lf", phoreal.binsize);
            mlog(DEBUG, "Setting %s to %lf", Icesat2Parms::PHOREAL_BINSIZE, phoreal.binsize);
        }
        lua_pop(L, 1);

        /* Geolocation */
        lua_getfield(L, index, Icesat2Parms::PHOREAL_GEOLOC);
        const char* geoloc_str = LuaObject::getLuaString(L, -1, true, NULL, &field_provided);
        if(field_provided)
        {
            phoreal_geoloc_t geoloc = str2geoloc(geoloc_str);
            if(geoloc != PHOREAL_UNSUPPORTED)
            {
                phoreal.geoloc = geoloc;
                mlog(DEBUG, "Setting %s to %d", Icesat2Parms::PHOREAL_GEOLOC, (int)phoreal.geoloc);
            }
        }
        lua_pop(L, 1);

        /* Use Absolute Heights */
        lua_getfield(L, index, Icesat2Parms::PHOREAL_USE_ABS_H);
        phoreal.use_abs_h = LuaObject::getLuaBoolean(L, -1, true, phoreal.use_abs_h, &field_provided);
        if(field_provided) mlog(DEBUG, "Setting %s to %d", Icesat2Parms::PHOREAL_USE_ABS_H, (int)phoreal.use_abs_h);
        lua_pop(L, 1);

        /* Send Waveforms */
        lua_getfield(L, index, Icesat2Parms::PHOREAL_WAVEFORM);
        phoreal.send_waveform = LuaObject::getLuaBoolean(L, -1, true, phoreal.send_waveform, &field_provided);
        if(field_provided) mlog(DEBUG, "Setting %s to %d", Icesat2Parms::PHOREAL_WAVEFORM, (int)phoreal.send_waveform);
        lua_pop(L, 1);

        /* Use ABoVE Classifier */
        lua_getfield(L, index, Icesat2Parms::PHOREAL_ABOVE);
        phoreal.above_classifier = LuaObject::getLuaBoolean(L, -1, true, phoreal.above_classifier, &field_provided);
        if(field_provided) mlog(DEBUG, "Setting %s to %d", Icesat2Parms::PHOREAL_ABOVE, (int)phoreal.above_classifier);
        lua_pop(L, 1);
    }
}
