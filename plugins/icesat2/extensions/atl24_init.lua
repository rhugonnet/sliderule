--
-- Initialize the environment needed for ATL24 processing
--
--  NOTES:  1. Downloads the ATL24 bathy mask from S3
--          2. Downloads the ATL24 uncertainty lookup tables from S3
--

local bathy_mask_complete = false
local uncertainty_lut_complete = true -- logic below needs it to be seeded to true

while sys.alive() and (not bathy_mask_complete) and (not uncertainty_lut_complete) do

    sys.log(core.INFO, "Initializing ATL24 processing environment...")

    local bathy_mask_local = "/data/ATL24_Mask_v5_Raster.tif"
    local bathy_mask_remote = "config/ATL24_Mask_v5_Raster.tif"
    bathy_mask_complete = sys.fileexists(bathy_mask_local) or aws.s3download("sliderule", bathy_mask_remote, aws.DEFAULT_REGION, aws.DEFAULT_IDENTITY, bathy_mask_local)

    for deg = 0,5 do
        for _,dir in ipairs({"THU", "TVU"}) do
            local uncertainty_lut_local = string.format("/data/ICESat2_%ddeg_500000_AGL_0.022_mrad_%s.csv", deg, dir)
            local uncertainty_lut_remote = string.format("config/ICESat2_%ddeg_500000_AGL_0.022_mrad_%s.csv", deg, dir)
            local uncertainty_lut_status = sys.fileexists(uncertainty_lut_local) or aws.s3download("sliderule", uncertainty_lut_remote, aws.DEFAULT_REGION, aws.DEFAULT_IDENTITY, uncertainty_lut_local)
            uncertainty_lut_complete = uncertainty_lut_complete and uncertainty_lut_status
        end
    end
    if (not bathy_mask_complete) or (not uncertainty_lut_complete) then
        sys.log(core.CRITICAL, string.format("Failed to initialize ATL24 processing environment: bathy_mask=%s, uncertainty_lut=%s", tostring(bathy_mask_complete), tostring(uncertainty_lut_complete)))
        sys.wait(30)
    end

end

sys.log(core.INFO, "Successfully initialized ATL24 processing environment")
