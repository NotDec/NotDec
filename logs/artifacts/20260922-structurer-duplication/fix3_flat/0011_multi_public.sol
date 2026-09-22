contract Decompiled {
    uint256 public slot_0;
    uint256 public slot_3;
    uint256 public slot_4;
    uint256 public slot_5;
    uint256 public slot_6;
    uint256 public slot_8;

    function Stop() public {
        // block_0:
        slot_4 = slot_4 & uint256((type(uint256).max - 255));
    }

    function Withdraw() public {
        // block_0:
        private__0x107_0x107(3072);
    }

    function public_0x802fe1da() public {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
    }

    function public_0xc0452ef8() public {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
    }

    function public_0xeaf67ab9() public {
        // block_0:
    }

    function public_0xeeeeeeee() public {
        // block_0:
    }

    function public_0xf39d8c65() public returns (uint256 ret0) {
        // block_0:
        return private__0x1b7_0x1b7(245);
    }

    function private__0x107_0x107(uint256 arg0) internal {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0) {
          // block_1:
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0, string_length=32
        }
    }

    function private__0x1b7_0x1b7(uint256 arg0) internal returns (uint256 ret0) {
        // block_0:
        return address(uint160((uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0)).balance - slot_3;
    }
}
