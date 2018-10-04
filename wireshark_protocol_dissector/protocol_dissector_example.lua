-- creates a Proto object, but doesn't register it yet
local test_proto = Proto("test", "Test Protocol")

-- a table of all of our Protocol's fields
local test_fields = 
{
    msgNo = ProtoField.uint32("test.msgNo", "msgNo", base.DEC),
    msgType = ProtoField.uint8("test.msgType", "msgType", base.HEX),
    subMsgType = ProtoField.uint8("test.subMsgType", "subMsgType", base.HEX),
    dataLen = ProtoField.uint16("test.dataLen", "dataLen", base.DEC),
    data = ProtoField.bytes("test.data", "data"),
}

-- register the ProtoFields
test_proto.fields = test_fields

-- a table of our default settings - these can be changed by changing
-- the preferences through the GUI or command-line; the Lua-side of that
-- preference handling is at the end of this script file
local default_settings = 
{
    enabled      = true, -- whether this dissector is enabled or not
    port         = 5001, -- default TCP port number for Test
}

--------------------------------------------------------------------------------
-- The following creates the callback function for the dissector.
-- It's the same as doing "test_proto.dissector = function (tvbuf,pkt,root)"
-- The 'tvbuf' is a Tvb object, 'pktinfo' is a Pinfo object, and 'root' is a TreeItem object.
-- Whenever Wireshark dissects a packet that our Proto is hooked into, it will call
-- this function and pass it these arguments for the packet it's dissecting.
function test_proto.dissector(tvbuf, pktinfo, root)

    -- set the protocol column to show our protocol name
    pktinfo.cols.protocol:set("Test")

    -- get the length of the packet buffer (Tvb).
    local pktlen = tvbuf:len()
    
    local offset = 0

    -- We start by adding our protocol to the dissection display tree.
    local tree = root:add(test_proto, tvbuf:range(offset, pktlen))
	
    tree:add(test_fields.msgNo, tvbuf(offset, 4))
    offset = offset + 4
    tree:add(test_fields.msgType, tvbuf(offset, 1))
    offset = offset + 1
    tree:add(test_fields.subMsgType, tvbuf(offset, 1))
    offset = offset + 1
    tree:add(test_fields.dataLen, tvbuf(offset, 2))
    offset = offset + 2
    tree:add(test_fields.data, tvbuf(offset, pktlen-offset))
end

--------------------------------------------------------------------------------
-- We want to have our protocol dissection invoked for a specific TCP port,
-- so get the TCP dissector table and add our protocol to it.
local function enableDissector()
    -- using DissectorTable:set() removes existing dissector(s), whereas the
    -- DissectorTable:add() one adds ours before any existing ones, but
    -- leaves the other ones alone, which is better
    DissectorTable.get("tcp.port"):add(default_settings.port, test_proto)
end

local function disableDissector()
    DissectorTable.get("tcp.port"):remove(default_settings.port, test_proto)
end

-- call it now
enableDissector()