contract Decompiled {
    uint256 public slot_0;

    function calldata_min_size(uint256 arg0) public {
        // block_0:
        require(msg.data.length >= 36);
        // block_1:
        slot_0 = arg0;
    }
}
