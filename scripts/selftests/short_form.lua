local runner = require("test_executive")
local td = runner.rootdir(arg[0]) -- root directory
local icesat2_td = td .. "../../datasets/icesat2/selftests/"
local opendata_td = td .. "../../datasets/opendata/selftests/"

runner.script(td .. "monitor.lua")
runner.script(td .. "http_server.lua")
runner.script(td .. "lua_script.lua")
runner.script(td .. "earth_data.lua")
runner.script(td .. "message_queue.lua")
runner.script(td .. "list.lua")
runner.script(td .. "ordering.lua")
runner.script(td .. "dictionary.lua")
runner.script(td .. "table.lua")
runner.script(td .. "timelib.lua")
runner.script(td .. "tcp_socket.lua")
runner.script(td .. "udp_socket.lua")
runner.script(td .. "multicast_device_writer.lua")
runner.script(td .. "cluster_socket.lua")
runner.script(td .. "http_client.lua")
runner.script(td .. "http_faults.lua")
runner.script(td .. "http_rqst.lua")
runner.script(td .. "ccsds_packetizer.lua")
runner.script(td .. "record_dispatcher.lua")
runner.script(td .. "limit_dispatch.lua")
runner.script(td .. "asset_index.lua")
runner.script(td .. "credential_store.lua")
runner.script(td .. "asset_loaddir.lua")
runner.script(td .. "hdf5_file.lua")
runner.script(td .. "parquet_sampler.lua")
runner.script(td .. "geojson_raster.lua")
runner.script(td .. "geouser_raster.lua")
runner.script(td .. "parms_tojson.lua")
runner.script(icesat2_td .. "plugin_unittest.lua")
runner.script(icesat2_td .. "atl03_reader.lua")
runner.script(icesat2_td .. "atl03_viewer.lua")
runner.script(icesat2_td .. "atl03_indexer.lua")
runner.script(icesat2_td .. "atl03_ancillary.lua")
runner.script(icesat2_td .. "h5_file.lua")
runner.script(icesat2_td .. "h5_element.lua")
runner.script(icesat2_td .. "s3_driver.lua")
runner.script(opendata_td .. "worldcover_reader.lua")
runner.script(opendata_td .. "globalcanopy_reader.lua")

-- Report Results --
local errors = runner.report()

-- Cleanup and Exit --
sys.quit( errors )
