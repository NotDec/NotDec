contract Decompiled {
    uint256[] public slot_0;

    function at(uint256 arg0) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 35);
        // block_1:
        return slot_0[arg0];
    }

    function fallback() public {
        // block_0:
        revert(); // empty
    }
}
