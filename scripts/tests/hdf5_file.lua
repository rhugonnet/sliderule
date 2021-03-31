local runner = require("test_executive")
local console = require("console")

-- Unit Test --

print('\n------------------\nTest01: File\n------------------')

f1 = h5.file("file:///data/ATLAS/ATL03_20200304065203_10470605_003_01.h5")
runner.check(f1:dir(2, "gt2l"), "failed to traverse hdf5 file")

rsps1 = msg.subscribe("h5testq")
f1:read({{dataset="ancillary_data/atlas_sdp_gps_epoch", valtype=core.REAL}}, "h5testq")
recdata = rsps1:recvrecord(3000)
data = string.char( recdata:getvalue("data[0]"),
                    recdata:getvalue("data[1]"),
                    recdata:getvalue("data[2]"),
                    recdata:getvalue("data[3]"),
                    recdata:getvalue("data[4]"),
                    recdata:getvalue("data[5]"),
                    recdata:getvalue("data[6]"),
                    recdata:getvalue("data[7]") )
epoch = string.unpack("d", data)
runner.check(epoch == 1198800018.0, "failed to read correct epoch")

rsps1:destroy()
f1:destroy()


print('\n------------------\nTest02: Read Dataset\n------------------')

dataq = "dataq"
rsps2 = msg.subscribe(dataq)

f2 = h5.dataset(core.READER, "file:///data/ATLAS/ATL03_20200304065203_10470605_003_01.h5", "ancillary_data/atlas_sdp_gps_epoch")
r2 = core.reader(f2, dataq)

vals = rsps2:recvstring(3000)
epoch = string.unpack('d', vals)

runner.check(epoch == 1198800018.0, "failed to read correct epoch")

rsps2:destroy()
r2:destroy()


print('\n------------------\nTest03: Read Dataset as Record\n------------------')

recq = "recq"
rsps3 = msg.subscribe(recq)

f3 = h5.dataset(core.READER, "file:///data/ATLAS/ATL03_20200304065203_10470605_003_01.h5", "gt2l/heights/dist_ph_along", 5, false)
r3 = core.reader(f3, recq)

recdata = rsps3:recvrecord(3000)

rectable = recdata:tabulate()
print("ID:     "..rectable.id)
print("OFFSET: "..rectable.offset)
print("SIZE:   "..rectable.size)

runner.check(rectable.id == 5)

rsps3:destroy()
r3:destroy()

print('\n------------------\nTest04: Read Dataset Raw\n------------------')

segment_file = "segment_id.bin"
o4 = core.writer(core.file(core.WRITER, core.BINARY, segment_file, core.FLUSHED), "h5rawq")
f4 = h5.dataset(core.READER, "file:///data/ATLAS/ATL03_20190111063212_02110206_003_01.h5", "gt1l/geolocation/segment_id", 0, true)
r4 = core.reader(f4, "h5rawq")

r4:waiton()
r4:destroy()

o4:waiton()
o4:destroy()

local expected_segment_id = 671087 -- starting segment id in file
local status = true
local f = assert(io.open(segment_file, "rb"))
while status do
    local bytes = f:read(4)
    if not bytes then break end
    local segment_id = string.unpack("<i4", bytes)
    status = runner.check(segment_id == expected_segment_id, string.format("unexpected segment id, %d != %d", segment_id, expected_segment_id))
    expected_segment_id = expected_segment_id + 1
end

f:close()
os.remove(segment_file)

-- Report Results --

runner.report()

