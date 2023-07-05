from sliderule import sliderule, earthdata
import sys

geojson = sys.argv[1]
region = sliderule.toregion(geojson)
granules = earthdata.cmr(short_name="GEDI02_A", polygon=region["poly"])

for granule in granules:
    print(f'./lpdaac_transfer.sh {granule}')