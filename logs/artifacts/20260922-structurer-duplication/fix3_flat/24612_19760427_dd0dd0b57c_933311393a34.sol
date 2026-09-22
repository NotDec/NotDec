contract Decompiled {
    uint256[] public slot_1;
    uint256[] public slot_10;
    uint256[] public slot_11;
    uint256[] public slot_13;
    uint256[] public slot_16;
    uint256[] public slot_20;
    uint256[] public slot_22;
    uint256[] public slot_23;
    uint256[] public slot_3;
    uint256[] public slot_4;
    uint256[] public slot_5;
    uint256[] public slot_6;
    uint256[] public slot_7;
    uint256[] public slot_8;
    uint256[] public slot_9;

    function public_0x0b417370(uint256 arg0, uint256 arg1) public {
        // block_0:
        require(arg1 == 0);
        // block_1:
        require(msg.data.length > 67);
        // block_2:
        require(arg1 < 281474976710656);
        // block_5:
    }

    function public_0x19b75522(uint256 arg0, uint256 arg1) public {
        // block_0:
        require(arg1 == 0);
        // block_1:
        require(msg.data.length > 35);
        // block_2:
        if (slot_4[arg0] / (uint256(1) << uint256(168)) & uint256(255) == 0) {
          // block_3:
          if (false /* TODO: evm.iszero22.not */) {
            // block_5:
            revert(); // returndata_forward
          }
          // block_4:
        }
        else {
          // block_6:
          if (!false /* TODO: evm.eq */) {
            // block_7:
          }
        }
        // block_8:
        if (!false /* TODO: evm.iszero75.not */) {
          // goto block_9
        }
        // goto block_10
        // block_10:
        revert(); // returndata_forward
        // block_9:
        // TODO: recover remaining body
    }

    function public_0xb67e4a5e(uint256 arg0, uint256 arg1) public {
        // block_0:
        require(arg1 == 0);
        // block_1:
        require(msg.data.length > 67);
        // block_3:
        private__0x1150_0x1150(arg1, 121895);
    }

    function public_0xb85f2556(uint256 arg0, uint256 arg1) public {
        // block_0:
        require(arg1 == 0);
        // block_1:
        require(msg.data.length > 99);
        // block_3:
        private__0x1150_0x1150(arg1, 4532);
    }

    function public_0xf9beefad(uint256 arg0, uint256 arg1) public {
        // block_0:
        uint256 temp_0;
        uint256 temp_1;
        uint256 temp_2;
        require(arg1 == 0);
        // block_1:
        require(msg.data.length > 67);
        // block_2:
        require(arg1 != 0, "CHE0033");
        // block_3:
        if (!(slot_6[arg0] == 0)) {
          // block_4:
          if (arg1 > slot_7[arg0]) {
            // goto block_12
          }
          // goto block_5
        }
        // block_5:
        if (slot_6[arg0] == 0) {
          // block_6:
        }
        else {
          // block_7:
          temp_0 = private__0x1268_0x1268(private__0x1251_0x1251(slot_10[arg0], slot_6[arg0], 3129), 10000, 3139);
          temp_1 = private__0x1268_0x1268(private__0x1251_0x1251(temp_0, slot_9[arg0], 3164), slot_7[arg0], 3174);
          temp_2 = private__0x1268_0x1268(private__0x1251_0x1251(temp_0, arg1, 3202), slot_7[arg0], 3212);
          if (temp_2 > temp_1) {
            // block_10:
          }
          else {
            // block_8:
            if (temp_2 < temp_1) {
              // block_9:
            }
          }
        }
        // block_11:
        // TODO: recover remaining body
    }

    function config(uint256 arg0, uint256 arg1) public {
        // block_0:
        require(arg1 == 0);
        // block_1:
    }

    function fallback() public {
        // block_0:
        revert(); // empty
    }

    function private__0x1150_0x1150(uint256 arg0, uint256 arg1) internal {
        // block_0:
        require(arg0 & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_1:
    }

    function private__0x1251_0x1251(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg1 * arg0;
    }

    function private__0x1268_0x1268(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 / arg1;
    }

    function private__0x128a_0x128a(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 - arg1;
    }

    function private__0x129d_0x129d(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg1 + arg0;
    }

    function private__0xce7_0xce7(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
        // block_0:
    }

    function private__0xe5f_0xe5f(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3, uint256 arg4) internal {
        // block_0:
    }
}
