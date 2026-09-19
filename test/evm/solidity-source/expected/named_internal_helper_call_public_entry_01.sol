contract Decompiled {
    uint256 public slot_0;

    function calls_helpers(uint256 arg0) public {
        // block_0:
        require(msg.data.length >= 36);
        // block_1:
        slot_0 = internal_helper_0x200(private_sum_0x100(5, arg0));
    }

    function private_sum_0x100(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + arg1;
    }

    function internal_helper_0x200(uint256 arg0) internal returns (uint256 ret0) {
        // block_0:
        return arg0 * uint256(3);
    }
}
