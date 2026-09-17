contract Decompiled {
    uint256 public slot_0;

    function calldata_min_size_sub(address arg0, address arg1, uint256 arg2) public {
        // block_0:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= 96);
        // block_1:
        slot_0 = arg2;
    }
}
