# python

import itertools
import json
import ssl
import urllib.request
import datetime
import numpy
import logging
import concurrent.futures
import sliderule

###############################################################################
# GLOBALS
###############################################################################

# create logger
logger = logging.getLogger(__name__)
# output dictionary keys
keys = ['segment_id','spot','delta_time','lat','lon','h_mean','dh_fit_dx','dh_fit_dy','rgt','cycle']
# output variable data types
dtypes = ['i','u1','f','f','f','f','f','f','f','u2','u2']
# parameters
CNF_POSSIBLE_TEP = -2
CNF_NOT_CONSIDERED = -1
CNF_BACKGROUND = 0
CNF_WITHIN_10M = 1
CNF_SURFACE_LOW = 2
CNF_SURFACE_MEDIUM = 3
CNF_SURFACE_HIGH = 4
SRT_LAND = 0
SRT_OCEAN = 1
SRT_SEA_ICE = 2
SRT_LAND_ICE = 3
SRT_INLAND_WATER = 4

###############################################################################
# NSIDC UTILITIES
###############################################################################
# The functions below have been adapted from the NSIDC download script and
# carry the following notice:
#
# Copyright (c) 2020 Regents of the University of Colorado
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.

CMR_URL = 'https://cmr.earthdata.nasa.gov'
CMR_PAGE_SIZE = 2000
CMR_FILE_URL = ('{0}/search/granules.json?provider=NSIDC_ECS'
                '&sort_key[]=start_date&sort_key[]=producer_granule_id'
                '&scroll=true&page_size={1}'.format(CMR_URL, CMR_PAGE_SIZE))

def __build_version_query_params(version):
    desired_pad_length = 3
    if len(version) > desired_pad_length:
        raise RuntimeError('Version string too long: "{0}"'.format(version))

    version = str(int(version))  # Strip off any leading zeros
    query_params = ''

    while len(version) <= desired_pad_length:
        padded_version = version.zfill(desired_pad_length)
        query_params += '&version={0}'.format(padded_version)
        desired_pad_length -= 1
    return query_params

def __cmr_filter_urls(search_results):
    """Select only the desired data files from CMR response."""
    if 'feed' not in search_results or 'entry' not in search_results['feed']:
        return []

    entries = [e['links']
               for e in search_results['feed']['entry']
               if 'links' in e]
    # Flatten "entries" to a simple list of links
    links = list(itertools.chain(*entries))

    urls = []
    unique_filenames = set()
    for link in links:
        if 'href' not in link:
            # Exclude links with nothing to download
            continue
        if 'inherited' in link and link['inherited'] is True:
            # Why are we excluding these links?
            continue
        if 'rel' in link and 'data#' not in link['rel']:
            # Exclude links which are not classified by CMR as "data" or "metadata"
            continue

        if 'title' in link and 'opendap' in link['title'].lower():
            # Exclude OPeNDAP links--they are responsible for many duplicates
            # This is a hack; when the metadata is updated to properly identify
            # non-datapool links, we should be able to do this in a non-hack way
            continue

        filename = link['href'].split('/')[-1]
        if filename in unique_filenames:
            # Exclude links with duplicate filenames (they would overwrite)
            continue

        unique_filenames.add(filename)

        if ".h5" in link['href'][-3:]:
            resource = link['href'].split("/")[-1]
            urls.append(resource)

    return urls

def __cmr_search(short_name, version, time_start, time_end, polygon=None):
    """Perform a scrolling CMR query for files matching input criteria."""
    params = '&short_name={0}'.format(short_name)
    params += __build_version_query_params(version)
    params += '&temporal[]={0},{1}'.format(time_start, time_end)
    if polygon:
        params += '&polygon={0}'.format(polygon)
    cmr_query_url = CMR_FILE_URL + params
    logger.debug('cmr request={0}\n'.format(cmr_query_url))

    cmr_scroll_id = None
    ctx = ssl.create_default_context()
    ctx.check_hostname = False
    ctx.verify_mode = ssl.CERT_NONE

    urls = []
    while True:
        req = urllib.request.Request(cmr_query_url)
        if cmr_scroll_id:
            req.add_header('cmr-scroll-id', cmr_scroll_id)
        response = urllib.request.urlopen(req, context=ctx)
        if not cmr_scroll_id:
            # Python 2 and 3 have different case for the http headers
            headers = {k.lower(): v for k, v in dict(response.info()).items()}
            cmr_scroll_id = headers['cmr-scroll-id']
            hits = int(headers['cmr-hits'])
        search_page = response.read()
        search_page = json.loads(search_page.decode('utf-8'))
        url_scroll_results = __cmr_filter_urls(search_page)
        if not url_scroll_results:
            break
        urls += url_scroll_results

    return urls

###############################################################################
# SLIDERULE UTILITIES
###############################################################################

#
#  __flatten_atl06
#
def __flatten_atl06(rsps):
    """
    rsps: array of responses from streaming source call to atl06 endpoint
    """
    global keys, dtypes
    # total length of flattened response
    flatten = numpy.sum([len(r['elevation']) for i,r in enumerate(rsps)]).astype(numpy.int)
    # python dictionary with flattened variables
    flattened = {}
    for key,dtype in zip(keys,dtypes):
        flattened[key] = numpy.zeros((flatten),dtype=dtype)

    # counter variable for flattening responses
    c = 0
    # flatten response
    for i,r in enumerate(rsps):
        for j,v in enumerate(r['elevation']):
            # add each variable
            for key,dtype in zip(keys,dtypes):
                flattened[key][c] = numpy.array(v[key],dtype=dtype)
            # add to counter
            c += 1

    return flattened

#
#  __get_values
#
def __get_values(data, dtype, size):
    """
    data:   tuple of bytes
    dtype:  element of datatypes OR basictypes
    size:   bytes in data
    """

    datatype2nptype = {
        sliderule.datatypes["TEXT"]:      numpy.byte,
        sliderule.datatypes["REAL"]:      numpy.double,
        sliderule.datatypes["INTEGER"]:   numpy.int32,
        sliderule.datatypes["DYNAMIC"]:   numpy.byte
    }

    basictype2nptype = {
        "INT8":     numpy.int8,
        "INT16":    numpy.int16,
        "INT32":    numpy.int32,
        "INT64":    numpy.int64,
        "UINT8":    numpy.uint8,
        "UINT16":   numpy.uint16,
        "UINT32":   numpy.uint32,
        "UINT64":   numpy.uint64,
        "BITFIELD": numpy.byte,
        "FLOAT":    numpy.single,
        "DOUBLE":   numpy.double,
        "TIME8":    numpy.byte,
        "STRING":   numpy.byte
    }

    if type(dtype) == int:
        datatype = datatype2nptype[dtype]
    else:
        datatype = basictype2nptype[dtype]

    raw = bytes(data)
    num_elements = int(size / numpy.dtype(datatype).itemsize)
    slicesize = num_elements * numpy.dtype(datatype).itemsize # truncates partial bytes
    values = numpy.frombuffer(raw[:slicesize], dtype=datatype, count=num_elements)

    return values

###############################################################################
# APIs
###############################################################################

#
#  INIT
#
def init (url, verbose=False, max_errors=3):
    sliderule.set_url(url)
    sliderule.set_verbose(verbose)
    sliderule.set_max_errors(max_errors)

#
#  COMMON METADATA REPOSITORY
#
def cmr (polygon=None, time_start=None, time_end=None, version='003', short_name='ATL03'):
    """
    polygon: list of longitude,latitude in counter-clockwise order with first and last point matching;
             three formats are supported:
             1. string - e.g. '-115.43,37.40,-109.55,37.58,-109.38,43.28,-115.29,43.05,-115.43,37.40'
             1. list - e.g. [-115.43,37.40,-109.55,37.58,-109.38,43.28,-115.29,43.05,-115.43,37.40]
             1. dictionary - e.g. [ {"lon": -115.43, "lat": 37.40},
                                    {"lon": -109.55, "lat": 37.58},
                                    {"lon": -109.38, "lat": 43.28},
                                    {"lon": -115.29, "lat": 43.05},
                                    {"lon": -115.43, "lat": 37.40} ]
    time_*: UTC time (i.e. "zulu" or "gmt");
            expressed in the following format: <year>-<month>-<day>T<hour>:<minute>:<second>Z
    """
    # set default start time to start of ICESat-2 mission
    if not time_start:
        time_start = '2018-10-13T00:00:00Z'
    # set default stop time to current time
    if not time_end:
        now = datetime.datetime.utcnow()
        time_end = now.strftime("%Y-%m-%dT%H:%M:%SZ")
    # flatten polygon if structure list of lat/lon provided
    if polygon:
        if type(polygon) == list:
            # if polygon list of dictionaries ("lat", "lon"), then flatten
            if type(polygon[0]) == dict:
                flatpoly = []
                for p in polygon:
                    flatpoly.append(p["lon"])
                    flatpoly.append(p["lat"])
                polygon = flatpoly
            # convert list into string
            polygon = str(polygon)[1:-1]

        # remove all spaces as this will be embedded in a url
        polygon = polygon.replace(" ", "")

    # call into NSIDC routines to make CMR request
    try:
        url_list = __cmr_search(short_name, version, time_start, time_end, polygon)
    except urllib.error.HTTPError as e:
        url_list = []
        logger.error("HTTP Request Error:", e)
    except RuntimeError as e:
        url_list = []
        logger.error("Runtime Error:", e)

    return url_list

#
#  ATL06
#
def atl06 (parm, resource, asset="atl03-cloud", track=0, as_numpy=False):

    # Build ATL06 Request
    rqst = {
        "atl03-asset" : asset,
        "resource": resource,
        "track": track,
        "parms": parm
    }

    # Execute ATL06 Algorithm
    rsps = sliderule.source("atl06", rqst, stream=True)

    # Flatten Responses
    if as_numpy:
        rsps = __flatten_atl06(rsps)
    else:
        flattened = {}
        if (len(rsps) > 0) and ("elevation" in rsps[0]) and (len(rsps[0]["elevation"]) > 0):
            # atl06rec
            for element in rsps[0]["elevation"][0].keys():
                flattened[element] = [rsps[r]["elevation"][i][element] for r in range(len(rsps)) for i in range(len(rsps[r]["elevation"]))]
        elif (len(rsps) > 0) and ("track" in rsps[0]) and ("segment_id" in rsps[0]):
            # atl03rec
            for element in rsps[0].keys():
                if type(rsps[0][element]) == tuple:
                    flattened[element] = [rsps[r][element][i] for r in range(len(rsps)) for i in range(2)]                    
                elif type(rsps[0][element]) == int:
                    flattened[element] = [rsps[r][element] for r in range(len(rsps)) for i in range(2)]
        else:
            # Unrecognized
            logger.warning("unable to process resource %s: no elements", resource)
        rsps = flattened

    # Return Responses
    return rsps

#
#  PARALLEL ATL06
#
def atl06p(parm, asset="atl03-cloud", track=0, as_numpy=False, max_workers=4, block=True):

    # Check Parameters are Valid
    if ("poly" not in parm) and ("t0" not in parm) and ("t1" not in parm):
        logger.error("Must supply some bounding parameters with request (poly, t0, t1)")
        return

    # Pull Out Polygon #
    polygon = None
    if "poly" in parm:
        polygon = parm["poly"]

    # Pull Out Time Period #
    time_start = None
    time_end = None
    if "t0" in parm:
        time_start = parm["t0"]
    if "t1" in parm:
        time_start = parm["t1"]

    # Make CMR Request #
    resources = cmr(polygon, time_start, time_end)

    # For Blocking Calls
    if block:

        # Make Parallel Processing Requests
        results = {}
        with concurrent.futures.ThreadPoolExecutor(max_workers=max_workers) as executor:
            futures = [executor.submit(atl06, parm, resource, asset, track, as_numpy) for resource in resources]

            # Wait for Results
            result_cnt = 0
            for future in concurrent.futures.as_completed(futures):
                result_cnt += 1
                logger.info("Results returned for %d out of %d resources", result_cnt, len(resources))
                if len(results) == 0:
                    results = future.result()
                else:
                    result = future.result()
                    for element in result:
                        if element not in results:
                            logger.error("Unable to construct results with element: %s", element)
                            continue
                        results[element] += result[element]

        # Return Results
        return results

    # For Non-Blocking Calls
    else:

        # Create Thread Pool
        executor = concurrent.futures.ThreadPoolExecutor(max_workers=max_workers)

        # Return List of Futures for Parallel Processing Request
        return [executor.submit(atl06, parm, resource, asset, track, as_numpy) for resource in resources]

#
#  H5
#
def h5 (dataset, resource, asset="atl03-cloud", datatype=sliderule.datatypes["REAL"]):

    # Handle Request Datatype Options
    rqst_datatype = sliderule.datatypes["DYNAMIC"]
    if type(datatype) == int:
        rqst_datatype = datatype

    # Baseline Request
    rqst = {
        "asset" : asset,
        "resource": resource,
        "dataset": dataset,
        "datatype": rqst_datatype,
        "id": 0
    }

    # Read H5 File
    rsps = sliderule.source("h5", rqst, stream=True)

    # Build Record Data
    rsps_datatype = rsps[0]["datatype"]
    data = ()
    size = 0
    for d in rsps:
        data = data + d["data"]
        size = size + d["size"]

    # Handle Response Datatype Options
    if rsps_datatype == sliderule.datatypes["DYNAMIC"]:
        rsps_datatype = datatype

    # Get Values
    values = __get_values(data, rsps_datatype, size)

    # Return Response
    return values


#
#  LOG
#
def log (level, duration):
    # Build Logging Request
    rqst = {
        "level" : level,
        "duration": duration
    }

    # Initiate Connection for Logging
    rsps = sliderule.source("log", rqst, stream=True)
