contract Decompiled {
    function fallback() public {
        // block_0:
        revert(); // empty
    }

    function run(uint256 arg0, uint256 arg1) public {
        // block_0:
        require(msg.data.length > 67);
        // block_1:
        if (arg0 == arg1) {
          // block_2:
        }
        else {
          // block_3:
          revert(); // custom_error_candidate, selector=0x14f8b682, args=2
        }
    }
}
