local runner = require("test_executive")
console = require("console")
asset = require("asset")
csv = require("csv")
json = require("json")

-- Setup --

local assets = asset.loaddir() -- looks for asset_directory.csv in same directory this script is located in
local asset_name = "arcticdem-local"
local arcticdem_local = core.getbyname(asset_name)

-- Unit Test --

print('\n------------------\nTest01: Reading milion points\n------------')

local lat =  82.86
local lon = -74.60
local el, status

local dem = arcticdem.raster()
runner.check(dem ~= nil)

local starttime = time.latch();
for i = 1, 1000000
do
    el, status = dem:elevation(lon, lat)
    if status ~= true then
        print(i, status, el)
    end
    if (i % 1000 == 0) then
        lat =  82.86
        lon = -74.60
    else
        lon = lon + 0.0001
        lat = lat + 0.0001
    end
end
local stoptime = time.latch();
local dtime = stoptime-starttime
print('ExecTime:',dtime*1000, '\n')


-- 'hole' in the raster
lat =  82.898092
lon = -74.418638
el, status = dem:elevation(lon, lat)
print('hole in raster', status, el)







print('\n------------------\nTest02: dim\n------------------')
local rows, cols = dem:dim()
print("rows: ", rows, "cols: ", cols)
runner.check(rows == 25000)
runner.check(cols == 25000)


print('\n------------------\nTest03: bbox\n------------------')
local lon_min, lat_min, lon_max, lat_max = dem:bbox()
print("lon_min: ", lon_min, "lat_min: ", lat_min, "\nlon_max: ", lon_max, "lat_max: ", lat_max)
runner.check(lon_min ~= 0)
runner.check(lat_min ~= 0)
runner.check(lon_max ~= 0)
runner.check(lon_max ~= 0)

print('\n------------------\nTest04: cellsize\n------------------')
local cellsize = dem:cell()
print("cellsize: ", cellsize)
runner.check(cellsize == 2.0)


-- Clean Up --

-- Report Results --

runner.report()

