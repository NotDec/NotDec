contract Decompiled {
    function add(uint256 arg0, uint256 arg1) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 67);
        // block_1:
        return arg1 + arg0;
    }

    function fallback() public {
        // block_0:
        revert(); // empty
    }
}
