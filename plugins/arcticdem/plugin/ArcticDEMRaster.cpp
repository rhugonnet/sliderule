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

#include "core.h"
#include "ArcticDEMRaster.h"

#include <uuid/uuid.h>
#include <ogr_geometry.h>
#include <ogrsf_frmts.h>
#include <gdal.h>
#include <gdalwarper.h>
#include <ogr_spatialref.h>
#include <gdal_priv.h>

#include "cpl_minixml.h"
#include "cpl_string.h"
#include "gdal.h"
#include "ogr_spatialref.h"

/******************************************************************************
 * LOCAL DEFINES AND MACROS
 ******************************************************************************/

#define CHECKPTR(p)                                                           \
do                                                                            \
{                                                                             \
    assert(p);                                                                \
    if ((p) == nullptr)                                                       \
    {                                                                         \
        throw RunTimeException(CRITICAL, RTE_ERROR,                           \
              "NULL pointer detected (%s():%d)", __FUNCTION__, __LINE__);     \
    }                                                                         \
} while (0)


#define CHECK_GDALERR(e)                                                      \
do                                                                            \
{                                                                             \
    if ((e))   /* CPLErr and OGRErr types have 0 for no error  */             \
    {                                                                         \
        throw RunTimeException(CRITICAL, RTE_ERROR,                           \
              "GDAL ERROR detected: %d (%s():%d)", e, __FUNCTION__, __LINE__);\
    }                                                                         \
} while (0)


/******************************************************************************
 * PRIVATE IMPLEMENTATION
 ******************************************************************************/
/******************************************************************************
 * STATIC DATA
 ******************************************************************************/

const char* ArcticDEMRaster::LuaMetaName = "ArcticDEMRaster";
const struct luaL_Reg ArcticDEMRaster::LuaMetaTable[] = {
    {"dim",         luaDimensions},
    {"bbox",        luaBoundingBox},
    {"cell",        luaCellSize},
    {"samples",     luaSamples},
    {NULL,          NULL}
};


/******************************************************************************
 * PUBLIC METHODS
 ******************************************************************************/

/*----------------------------------------------------------------------------
 * init
 *----------------------------------------------------------------------------*/
void ArcticDEMRaster::init( void )
{
    /* Register all gdal drivers */
    GDALAllRegister();
}

/*----------------------------------------------------------------------------
 * deinit
 *----------------------------------------------------------------------------*/
void ArcticDEMRaster::deinit( void )
{
    GDALDestroy();
}

/*----------------------------------------------------------------------------
 * luaCreate
 *----------------------------------------------------------------------------*/
int ArcticDEMRaster::luaCreate( lua_State* L )
{
    try
    {
        return createLuaObject(L, create(L, 1));
    }
    catch( const RunTimeException& e )
    {
        mlog(e.level(), "Error creating %s: %s", LuaMetaName, e.what());
        return returnLuaStatus(L, false);
    }
}

/*----------------------------------------------------------------------------
 * create
 *----------------------------------------------------------------------------*/
ArcticDEMRaster* ArcticDEMRaster::create( lua_State* L, int index )
{
    const int radius = getLuaInteger(L, -1);
    lua_pop(L, 1);
    const char* dem_sampling = getLuaString(L, -1);
    lua_pop(L, 1);
    const char* dem_type = getLuaString(L, -1);
    lua_pop(L, 1);
    return new ArcticDEMRaster(L, dem_type, dem_sampling, radius);
}



static void getVrtName( double lon, double lat, std::string& vrtFile )
{
    int ilat = floor(lat);
    int ilon = floor(lon);

    vrtFile = "/data/ArcticDem/strips/n" +
               std::to_string(ilat)      +
               ((ilon < 0) ? "w" : "e")  +
               std::to_string(abs(ilon)) +
               ".vrt";
}


/*----------------------------------------------------------------------------
 * containsPoint
 *----------------------------------------------------------------------------*/
inline bool ArcticDEMRaster::containsPoint(GDALDataset *dset, ArcticDEMRaster::bbox_t *bbox, OGRPoint *p)
{
    return (dset && (p->getX() >= bbox->lon_min) && (p->getX() <= bbox->lon_max) &&
                    (p->getY() >= bbox->lat_min) && (p->getY() <= bbox->lat_max));
}


/*----------------------------------------------------------------------------
 * samplesMosaic
 *----------------------------------------------------------------------------*/
double ArcticDEMRaster::sampleMosaic(double lon, double lat)
{
    double sample = ARCTIC_DEM_INVALID_ELELVATION;

    OGRPoint p = {lon, lat};
    if (p.transform(transf) == OGRERR_NONE)
    {
        /* Is point in big mosaic VRT dataset? */
        if (containsPoint(vrtDset, &vrtBbox, &p))
        {
            bool foundPoint = false;
            if (rasterList.length() > 0)
            {
                rasterList[0].point = &p;
                foundPoint = readRaster(&rasterList[0]);
            }

            if (!foundPoint)
            {
                if (findRasters(&p))
                {
                    if (rasterList.length() > 0)
                    {
                        rasterList[0].point = &p;
                        readRaster(&rasterList[0]);
                        sample = rasterList[0].value;
                    }
                }
            }
        }
        else
        {
            throw RunTimeException(CRITICAL, RTE_ERROR, "point: lon: %lf, lat: %lf not in mosaic VRT", lon, lat);
        }
    }

    return sample;
}


/*----------------------------------------------------------------------------
 * samplesStrips
 *----------------------------------------------------------------------------*/
void ArcticDEMRaster::sampleStrips(double lon, double lat)
{
    OGRPoint p = {lon, lat};
    if (p.transform(transf) == OGRERR_NONE)
    {
        /* If point is not in current scene VRT dataset, open new scene */
        if (!containsPoint(vrtDset, &vrtBbox, &p))
        {
            std::string newVrtFile;
            getVrtName(lon, lat, newVrtFile);

            if (!openVrtDset(newVrtFile.c_str()))
                throw RunTimeException(CRITICAL, RTE_ERROR, "point: lon: %lf, lat:%lf not in strip VRT", lon, lat);
        }

        /*
         * No smarts here, always get list of rasters with this point in the scene.
         * Maybe only 28 rasters contained previous point but there may be thousands of rasters in a scene.
         */
        if (findRasters(&p))
            readRasters(&p);
    }
}


/*----------------------------------------------------------------------------
 * samples
 *----------------------------------------------------------------------------*/
void ArcticDEMRaster::samples(double lon, double lat)
{
    if      (demType == MOSAIC) sampleMosaic(lon, lat);
    else if (demType == STRIPS) sampleStrips(lon, lat);
}



/*----------------------------------------------------------------------------
 * Destructor
 *----------------------------------------------------------------------------*/
ArcticDEMRaster::~ArcticDEMRaster(void)
{
    if (vrtDset) GDALClose((GDALDatasetH)vrtDset);
    for (int i = 0; i < rasterList.length(); i++)
    {
        raster_info_t rinfo = rasterList.get(i);
        if (rinfo.dset) GDALClose((GDALDatasetH)rinfo.dset);
    }
    for (int i = 0; i < threadCount; i++)
    {
        pthread_t tid = rasterRreader[i];
        pthread_join(tid, nullptr);
    }

    if (transf) OGRCoordinateTransformation::DestroyCT(transf);
}

/******************************************************************************
 * PROTECTED METHODS
 ******************************************************************************/

/*----------------------------------------------------------------------------
 * findRasters
 *----------------------------------------------------------------------------*/
bool ArcticDEMRaster::findRasters(OGRPoint *p)
{
    bool foundRaster = false;

    try
    {
        /* Close existing rasters */
        for (int i = 0; i < rasterList.length(); i++)
        {
            raster_info_t rinfo = rasterList.get(i);
            if (rinfo.dset) GDALClose((GDALDatasetH)rinfo.dset);
        }
        rasterList.clear();

        const int32_t col = static_cast<int32_t>(floor(invgeot[0] + invgeot[1] * p->getX() + invgeot[2] * p->getY()));
        const int32_t row = static_cast<int32_t>(floor(invgeot[3] + invgeot[4] * p->getX() + invgeot[5] * p->getY()));

        if (col >= 0 && row >= 0 && col < vrtDset->GetRasterXSize() && row < vrtDset->GetRasterYSize())
        {
            CPLString str;
            str.Printf("Pixel_%d_%d", col, row);

            const char *mdata = vrtBand->GetMetadataItem(str, "LocationInfo");
            if (mdata)
            {
                CPLXMLNode *root = CPLParseXMLString(mdata);
                if (root && root->psChild && root->eType == CXT_Element && EQUAL(root->pszValue, "LocationInfo"))
                {
                    for (CPLXMLNode *psNode = root->psChild; psNode; psNode = psNode->psNext)
                    {
                        if (psNode->eType == CXT_Element && EQUAL(psNode->pszValue, "File") && psNode->psChild)
                        {
                            raster_info_t rinfo;
                            bzero(&rinfo, sizeof(rinfo));

                            rinfo.obj = this;
                            rinfo.point = p;
                            rinfo.value = ARCTIC_DEM_INVALID_ELELVATION;

                            char *fname = CPLUnescapeString(psNode->psChild->pszValue, nullptr, CPLES_XML);
                            CHECKPTR(fname);
                            rinfo.fileName = fname;
                            CPLFree(fname);
                            rasterList.add(rinfo);
                            foundRaster = true;
                        }
                    }
                }
                CPLDestroyXMLNode(root);
            }
        }
    }
    catch (const RunTimeException &e)
    {
        mlog(e.level(), "Error creating raster: %s", e.what());
    }

    return foundRaster;
}


/*----------------------------------------------------------------------------
 * readRasters
 *----------------------------------------------------------------------------*/
bool ArcticDEMRaster::readRasters(OGRPoint *p)
{
    bool pointInDset = false;
    try
    {
        for (int i = 0; i < rasterList.length(); i++)
        {
            if(threadCount < MAX_READER_THREADS)
            {
                raster_info_t *rinfo = &(rasterList.get(i));

                pthread_attr_t pthread_attr;
                pthread_attr_init(&pthread_attr);
                if( pthread_create(&rasterRreader[threadCount], &pthread_attr, readingThread, rinfo) )
                {
                    perror("pthread_create() error");
                }
                threadCount++;
            }
            else
            {
                throw RunTimeException(CRITICAL, RTE_ERROR, "list of rasters to read: %d, is greater than max reading threads %d\n",
                                       rasterList.length(), MAX_READER_THREADS);
            }
        }

        /* Wait for all readers to finish */
        for (int i = 0; i < threadCount; i++ )
        {
            pthread_t tid = rasterRreader[i];
            if( pthread_join(tid, nullptr) )
            {
                perror("pthread_join() error");
            }
        }
        threadCount = 0;

#if 0
        for (int i = 0; i < rasterList.length(); i++)
        {
            print2term("%d, sample: %lf, readTime: %lf\n", i, rasterList[i].value, rasterList[i].readTime);
        }
#endif

    }
    catch (const RunTimeException &e)
    {
        mlog(e.level(), "Error reading rasters: %s", e.what());
    }

    return pointInDset;
}

/*----------------------------------------------------------------------------
 * readingThread
 *----------------------------------------------------------------------------*/
void* ArcticDEMRaster::readingThread(void *param)
{
    raster_info_t *rinfo = (raster_info_t*)param;
    rinfo->obj->readRaster(rinfo);
    return nullptr;
}


/*----------------------------------------------------------------------------
 * readingThread
 *----------------------------------------------------------------------------*/
bool ArcticDEMRaster::readRaster(raster_info_t* rinfo)
{
    bool foundPoint = false;

    try
    {
        OGRPoint* p = rinfo->point;
        double startTime = TimeLib::latchtime();

        if(rinfo->dset == nullptr)
        {
            rinfo->dset = (GDALDataset *)GDALOpenEx(rinfo->fileName.c_str(), GDAL_OF_RASTER | GDAL_OF_READONLY, NULL, NULL, NULL);
            CHECKPTR(rinfo->dset);

            /* Store information about raster */
            rinfo->cols = rinfo->dset->GetRasterXSize();
            rinfo->rows = rinfo->dset->GetRasterYSize();

            /* Get raster boundry box */
            double geot[6] = {0};
            CPLErr err = rinfo->dset->GetGeoTransform(geot);
            CHECK_GDALERR(err);
            rinfo->bbox.lon_min = geot[0];
            rinfo->bbox.lon_max = geot[0] + rinfo->cols * geot[1];
            rinfo->bbox.lat_max = geot[3];
            rinfo->bbox.lat_min = geot[3] + rinfo->rows * geot[5];

            rinfo->cellSize = geot[1];

            /* Get raster block size */
            rinfo->band = rinfo->dset->GetRasterBand(1);
            CHECKPTR(rinfo->band);
            rinfo->band->GetBlockSize(&rinfo->xBlockSize, &rinfo->yBlockSize);
            mlog(DEBUG, "Raster xBlockSize: %d, yBlockSize: %d", rinfo->xBlockSize, rinfo->yBlockSize);
        }

        /* Is point in this raster? */
        if (containsPoint(rinfo->dset, &rinfo->bbox, p))
        {
            /* Raster row, col for point */
            const int32_t col = static_cast<int32_t>(floor((p->getX() - rinfo->bbox.lon_min) / rinfo->cellSize));
            const int32_t row = static_cast<int32_t>(floor((rinfo->bbox.lat_max - p->getY()) / rinfo->cellSize));

            foundPoint = true;

            /* Use fast 'lookup' method for nearest neighbour. */
            if (rinfo->obj->sampleAlg == GRIORA_NearestNeighbour)
            {
                /* Raster offsets to block of interest */
                uint32_t xblk = col / rinfo->xBlockSize;
                uint32_t yblk = row / rinfo->yBlockSize;

                GDALRasterBlock *block = nullptr;
                int cnt = 2;
                do
                {
                    /* Retry read if error */
                    block = rinfo->band->GetLockedBlockRef(xblk, yblk, false);
                } while (block == nullptr && cnt--);
                CHECKPTR(block);

                float *fp = (float *)block->GetDataRef();
                CHECKPTR(fp);

                /* col, row inside of block */
                uint32_t _col = col % rinfo->xBlockSize;
                uint32_t _row = row % rinfo->yBlockSize;
                uint32_t offset = _row * rinfo->xBlockSize + _col;

                rinfo->value = fp[offset];
                block->DropLock();
                mlog(DEBUG, "Elevation: %f, col: %u, row: %u, xblk: %u, yblk: %u, bcol: %u, brow: %u, offset: %u\n",
                     rinfo->value, col, row, xblk, yblk, _col, _row, offset);
            }
            else
            {
#if 0
     FROM gdal.h
     What are these kernels? How do I use them to read one pixel with resampling?
     Below is an attempt to do it but I am 99.999% sure it is wrong.

    /*! Nearest neighbour */                            GRIORA_NearestNeighbour = 0,
    /*! Bilinear (2x2 kernel) */                        GRIORA_Bilinear = 1,
    /*! Cubic Convolution Approximation (4x4 kernel) */ GRIORA_Cubic = 2,
    /*! Cubic B-Spline Approximation (4x4 kernel) */    GRIORA_CubicSpline = 3,
    /*! Lanczos windowed sinc interpolation (6x6 kernel) */ GRIORA_Lanczos = 4,
    /*! Average */                                      GRIORA_Average = 5,
    /*! Mode (selects the value which appears most often of all the sampled points) */
                                                        GRIORA_Mode = 6,
    /*! Gauss blurring */                               GRIORA_Gauss = 7
#endif

                float rbuf[1] = {0};
                int _cellsize = rinfo->cellSize;
                int radius_in_meters = ((rinfo->obj->radius + _cellsize - 1) / _cellsize) * _cellsize; // Round to multiple of cellSize
                int radius_in_pixels = (radius_in_meters == 0) ? 1 : radius_in_meters / _cellsize;
                int _col = col - radius_in_pixels;
                int _row = row - radius_in_pixels;
                int size = radius_in_pixels + 1 + radius_in_pixels;

                /* If 8 pixels around pixel of interest are not in the raster boundries return pixel value. */
                if (_col < 0 || _row < 0)
                {
                    _col = col;
                    _row = row;
                    size = 1;
                    rinfo->obj->sampleAlg = GRIORA_NearestNeighbour;
                }

                GDALRasterIOExtraArg args;
                INIT_RASTERIO_EXTRA_ARG(args);
                args.eResampleAlg = rinfo->obj->sampleAlg;
                CPLErr err = CE_None;
                int cnt = 2;
                do
                {
                    /* Retry read if error */
                    err = rinfo->band->RasterIO(GF_Read, _col, _row, size, size, rbuf, 1, 1, GDT_Float32, 0, 0, &args);
                } while (err != CE_None && cnt--);
                CHECK_GDALERR(err);
                rinfo->value = rbuf[0];
                mlog(DEBUG, "Resampled elevation:  %f, radiusMeters: %d, radiusPixels: %d, size: %d\n", rbuf[0], rinfo->obj->radius, radius_in_pixels, size);
                // print2term("Resampled elevation:  %f, radiusMeters: %d, radiusPixels: %d, size: %d\n", rbuf[0], radius, radius_in_pixels, size);
            }
        }
        else
        {
            rinfo->value = ARCTIC_DEM_INVALID_ELELVATION;
        }

        rinfo->readTime = TimeLib::latchtime() - startTime;
    }
    catch (const RunTimeException &e)
    {
        mlog(e.level(), "Error reading raster: %s", e.what());
    }

    return foundPoint;
}


bool ArcticDEMRaster::openVrtDset(const char *fileName)
{
    bool objCreated = false;

    try
    {
        /* Cleanup previous vrtDset */
        if (vrtDset)
        {
            GDALClose((GDALDatasetH)vrtDset);
            vrtDset = nullptr;
        }

        if (transf)
        {
            OGRCoordinateTransformation::DestroyCT(transf);
            transf = nullptr;
        }

        /* New vrtDset */
        vrtDset = (VRTDataset *)GDALOpenEx(fileName, GDAL_OF_READONLY | GDAL_OF_VERBOSE_ERROR, NULL, NULL, NULL);
        CHECKPTR(vrtDset);
        vrtFileName = fileName;

        vrtBand = vrtDset->GetRasterBand(1);
        CHECKPTR(vrtBand);

        /* Get inverted geo transfer for vrt */
        double geot[6] = {0};
        CPLErr err = GDALGetGeoTransform(vrtDset, geot);
        CHECK_GDALERR(err);
        if (!GDALInvGeoTransform(geot, invgeot))
        {
            CPLError(CE_Failure, CPLE_AppDefined, "Cannot invert geotransform");
            CHECK_GDALERR(CE_Failure);
        }

        /* Store information about vrt raster */
        vrtCols = vrtDset->GetRasterXSize();
        vrtRows = vrtDset->GetRasterYSize();

        /* Get raster boundry box */
        bzero(geot, sizeof(geot));
        err = vrtDset->GetGeoTransform(geot);
        CHECK_GDALERR(err);
        vrtBbox.lon_min = geot[0];
        vrtBbox.lon_max = geot[0] + vrtCols * geot[1];
        vrtBbox.lat_max = geot[3];
        vrtBbox.lat_min = geot[3] + vrtRows * geot[5];

        vrtCellSize = geot[1];

        OGRErr ogrerr = srcSrs.importFromEPSG(RASTER_PHOTON_CRS);
        CHECK_GDALERR(ogrerr);
        const char *projref = vrtDset->GetProjectionRef();
        if (projref)
        {
            mlog(DEBUG, "%s", projref);
            ogrerr = trgSrs.importFromProj4(projref);
        }
        else
        {
            /* In case vrt file does not have projection info, use default */
            ogrerr = trgSrs.importFromEPSG(RASTER_ARCTIC_DEM_CRS);
        }
        CHECK_GDALERR(ogrerr);

        /* Force traditional axis order to avoid lat,lon and lon,lat API madness */
        trgSrs.SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);
        srcSrs.SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);

        /* Create coordinates transformation */
        transf = OGRCreateCoordinateTransformation(&srcSrs, &trgSrs);
        CHECKPTR(transf);
        objCreated = true;
#if 0
        {
            /* Get list of rasters in this vrt file */
            char **fileLists = GDALGetFileList((GDALDatasetH)vrtDset);
            int i;
            for (i=1; fileLists[i] != nullptr; i++)
            {
                // print2term("%s\n", fileLists[i]);
            }

            print2term("First raster: %s\n", fileLists[1]);
            print2term("Last  raster: %s\n", fileLists[i-1]);
            print2term("There are %d rasters in %s\n", i, fileLists[0]);
            CSLDestroy(fileLists);
        }
#endif
    }
    catch (const RunTimeException &e)
    {
        mlog(e.level(), "Error creating new VRT dataset: %s", e.what());
    }

    return objCreated;
}

/* Utilitiy function to get UUID string */
static const char *getUuid(char *uuid_str)
{
    uuid_t uuid;
    uuid_generate(uuid);
    uuid_unparse_lower(uuid, uuid_str);
    return uuid_str;
}

/*----------------------------------------------------------------------------
 * Constructor
 *----------------------------------------------------------------------------*/
ArcticDEMRaster::ArcticDEMRaster(lua_State *L, const char *dem_type, const char *dem_sampling, const int sampling_radius):
    LuaObject(L, BASE_OBJECT_TYPE, LuaMetaName, LuaMetaTable)
{
    char uuid_str[UUID_STR_LEN] = {0};
    std::string fname;

    CHECKPTR(dem_type);
    CHECKPTR(dem_sampling);
    demType = INVALID;

    if (!strcasecmp(dem_type, "mosaic"))
    {
        // fname = "/data/ArcticDem/mosaic_short.vrt";
        fname = "/data/ArcticDem/mosaic.vrt";
        demType = MOSAIC;
    }
    else if (!strcasecmp(dem_type, "strip"))
    {
        // fname = "/data/ArcticDem/strips/n51w178.vrt";
        fname = "/data/ArcticDem/strips/n51e156.vrt"; /* First strip file in catalog */
        demType = STRIPS;
    }
    else throw RunTimeException(CRITICAL, RTE_ERROR, "Invalid dem_type: %s:", dem_type);

    if      (!strcasecmp(dem_sampling, "NearestNeighbour")) sampleAlg = GRIORA_NearestNeighbour;
    else if (!strcasecmp(dem_sampling, "Bilinear"))         sampleAlg = GRIORA_Bilinear;
    else if (!strcasecmp(dem_sampling, "Cubic"))            sampleAlg = GRIORA_Cubic;
    else if (!strcasecmp(dem_sampling, "CubicSpline"))      sampleAlg = GRIORA_CubicSpline;
    else if (!strcasecmp(dem_sampling, "Lanczos"))          sampleAlg = GRIORA_Lanczos;
    else if (!strcasecmp(dem_sampling, "Average"))          sampleAlg = GRIORA_Average;
    else if (!strcasecmp(dem_sampling, "Mode"))             sampleAlg = GRIORA_Mode;
    else if (!strcasecmp(dem_sampling, "Gauss"))            sampleAlg = GRIORA_Gauss;
    else
        throw RunTimeException(CRITICAL, RTE_ERROR, "Invalid sampling algorithm: %s:", dem_sampling);

    if (sampling_radius >= 0)
        radius = sampling_radius;
    else throw RunTimeException(CRITICAL, RTE_ERROR, "Invalid sampling radius: %d:", sampling_radius);

    /* Initialize Class Data Members */
    vrtDset = nullptr;
    vrtBand = nullptr;
    rasterList.clear();
    bzero(rasterRreader, sizeof(rasterRreader));
    bzero(invgeot, sizeof(invgeot));
    threadCount = 0;
    vrtRows = vrtCols = vrtCellSize = 0;
    bzero(&vrtBbox, sizeof(vrtBbox));
    transf = nullptr;
    srcSrs.Clear();
    trgSrs.Clear();

    if (!openVrtDset(fname.c_str()))
        throw RunTimeException(CRITICAL, RTE_ERROR, "ArcticDEMRaster constructor failed");
}

/******************************************************************************
 * PRIVATE METHODS
 ******************************************************************************/

/*----------------------------------------------------------------------------
 * luaDimensions - :dim() --> rows, cols
 *----------------------------------------------------------------------------*/
int ArcticDEMRaster::luaDimensions(lua_State *L)
{
    bool status = false;
    int num_ret = 1;

    try
    {
        /* Get Self */
        ArcticDEMRaster *lua_obj = (ArcticDEMRaster *)getLuaSelf(L, 1);

        /* Set Return Values */
        lua_pushinteger(L, lua_obj->vrtRows);
        lua_pushinteger(L, lua_obj->vrtCols);
        num_ret += 2;

        /* Set Return Status */
        status = true;
    }
    catch (const RunTimeException &e)
    {
        mlog(e.level(), "Error getting dimensions: %s", e.what());
    }

    /* Return Status */
    return returnLuaStatus(L, status, num_ret);
}

/*----------------------------------------------------------------------------
 * luaBoundingBox - :bbox() --> (lon_min, lat_min, lon_max, lat_max)
 *----------------------------------------------------------------------------*/
int ArcticDEMRaster::luaBoundingBox(lua_State *L)
{
    bool status = false;
    int num_ret = 1;

    try
    {
        /* Get Self */
        ArcticDEMRaster *lua_obj = (ArcticDEMRaster *)getLuaSelf(L, 1);

        /* Set Return Values */
        lua_pushnumber(L, lua_obj->vrtBbox.lon_min);
        lua_pushnumber(L, lua_obj->vrtBbox.lat_min);
        lua_pushnumber(L, lua_obj->vrtBbox.lon_max);
        lua_pushnumber(L, lua_obj->vrtBbox.lat_max);
        num_ret += 4;

        /* Set Return Status */
        status = true;
    }
    catch (const RunTimeException &e)
    {
        mlog(e.level(), "Error getting bounding box: %s", e.what());
    }

    /* Return Status */
    return returnLuaStatus(L, status, num_ret);
}

/*----------------------------------------------------------------------------
 * luaCellSize - :cell() --> cell size
 *----------------------------------------------------------------------------*/
int ArcticDEMRaster::luaCellSize(lua_State *L)
{
    bool status = false;
    int num_ret = 1;

    try
    {
        /* Get Self */
        ArcticDEMRaster *lua_obj = (ArcticDEMRaster *)getLuaSelf(L, 1);

        /* Set Return Values */
        lua_pushnumber(L, lua_obj->vrtCellSize);
        num_ret += 1;

        /* Set Return Status */
        status = true;
    }
    catch (const RunTimeException &e)
    {
        mlog(e.level(), "Error getting cell size: %s", e.what());
    }

    /* Return Status */
    return returnLuaStatus(L, status, num_ret);
}

/*----------------------------------------------------------------------------
 * luaSamples - :samples(lon, lat) --> in|out
 *----------------------------------------------------------------------------*/
int ArcticDEMRaster::luaSamples(lua_State *L)
{
    bool status = false;
    int num_ret = 1;

    try
    {
        /* Get Self */
        ArcticDEMRaster *lua_obj = (ArcticDEMRaster *)getLuaSelf(L, 1);

        /* Get Coordinates */
        double lon = getLuaFloat(L, 2);
        double lat = getLuaFloat(L, 3);

        /* Get Elevations */
        lua_obj->samples(lon, lat);

        if (lua_obj->rasterList.length() > 0)
        {
            /* Create Table */
            lua_createtable(L, lua_obj->rasterList.length(), 0);

            for (int i = 0; i < lua_obj->rasterList.length(); i++)
            {
                raster_info_t rinfo = lua_obj->rasterList.get(i);

                lua_createtable(L, 0, 2);
                LuaEngine::setAttrStr(L, "file", rinfo.fileName.c_str());
                LuaEngine::setAttrNum(L, "value", rinfo.value);
                lua_rawseti(L, -2, i + 1);
            }

            num_ret++;
            status = true;
        }
    }
    catch (const RunTimeException &e)
    {
        mlog(e.level(), "Error getting elevation: %s", e.what());
    }

    /* Return Status */
    return returnLuaStatus(L, status, num_ret);
}