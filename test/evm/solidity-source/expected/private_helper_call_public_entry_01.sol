contract Decompiled {
    uint256 public slot_0;
    uint256 public slot_1;
    uint256 public slot_2;
    uint256 public slot_3;

    function one_store(uint256 arg0) public {
        // block_0:
        require(msg.data.length >= 36);
        // block_1:
        slot_3 = private__sum_0x100(1, arg0);
    }

    function two_stores(uint256 arg0) public {
        // block_0:
        uint256 temp_0;
        require(msg.data.length >= 36);
        // block_1:
        temp_0 = private__sum_0x100(5, arg0);
        private__record_0x200(temp_0);
        slot_0 = temp_0;
        slot_1 = temp_0;
    }

    function private__sum_0x100(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + arg1;
    }

    function private__record_0x200(uint256 arg0) internal {
        // block_0:
        slot_2 = arg0;
    }
}
