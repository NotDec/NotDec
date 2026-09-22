contract Decompiled {
    event Event_0x8C5BE1E5EBEC7D5BD14F71427D1E84F3DD0314C0F7B2291E5B200AC8C7C3B925(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF(uint256 indexed arg0, uint256 indexed arg1);

    uint256 public slot_0;
    uint256 public slot_1;
    uint256 public slot_2;
    uint256 public slot_3;
    mapping(uint256 => uint256) public slot_4;
    mapping(uint256 => uint256) public slot_5;

    function allowance(address arg0, address arg1) public {
        // block_0:
    }

    function approve(address arg0, uint256 arg1) public {
        // block_0:
        private__0x7c6_0x7c6(arg1, uint256(uint160(arg0)), uint256(uint160(msg.sender)), 784);
    }

    function balanceOf(address arg0) public {
        // block_0:
    }

    function decimals() public {
        // block_0:
    }

    function fallback() public {
        // block_0:
        revert(); // empty
    }

    function name() public {
        // block_0:
    }

    function openTrading(address arg0) public {
        // block_0:
    }

    function pancakePair() public {
        // block_0:
    }

    function removeLimits(uint256 arg0) public {
        // block_0:
        private__0x6f4_0x6f4(arg0, 626);
    }

    function symbol() public {
        // block_0:
    }

    function totalSupply() public {
        // block_0:
    }

    function transferFrom(address arg0, address arg1, uint256 arg2) public {
        // block_0:
        private__0x989_0x989(arg2, uint256(uint160(msg.sender)), uint256(uint160(arg0)), 818);
        private__0xa1d_0xa1d(arg2, uint256(uint160(arg1)), uint256(uint160(arg0)), 829);
    }

    function transfer(address arg0, uint256 arg1) public {
        // block_0:
        private__0xa1d_0xa1d(arg1, uint256(uint160(arg0)), uint256(uint160(msg.sender)), 1089);
    }

    function private__0x106d_0x106d(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal returns (uint256 ret0) {
        // block_0:
        private__0xf7f_0xf7f(arg2, arg0, 4224);
        private__0xf7f_0xf7f(arg1, arg0 + uint256(32), 4237);
        return arg0 + uint256(64);
    }

    function private__0x14e1_0x14e1(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        temp_0 = private__0xd06_0xd06(arg0 + uint256(32), 38, 5323);
        return temp_0 + uint256(64);
    }

    function private__0x1279_0x1279(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        uint256 temp_1;
        uint256 temp_2;
        temp_0 = private__0xe04_0xe04(arg0, 4739);
        temp_1 = private__0xe04_0xe04(arg1, 4750);
        temp_2 = private__0xe04_0xe04(temp_0 * temp_1, 4764);
        if (temp_0 == 0 || temp_1 == temp_2 / temp_0) {
          // block_1:
          return temp_2;
        }
        else {
          // block_2:
          revert(); // panic, panic=17, selector=0x4e487b71
        }
    }

    function private__0x132a_0x132a(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        temp_0 = private__0xd06_0xd06(arg0 + uint256(32), 36, 4884);
        return temp_0 + uint256(64);
    }

    function private__0x13b8_0x13b8(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        temp_0 = private__0xd06_0xd06(arg0 + uint256(32), 34, 5026);
        return temp_0 + uint256(64);
    }

    function private__0x1420_0x1420(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        temp_0 = private__0xd06_0xd06(arg0 + uint256(32), 29, 5130);
        return temp_0 + uint256(32);
    }

    function private__0x143e_0x143e(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        uint256 temp_1;
        temp_0 = private__0xe04_0xe04(arg0, 5192);
        temp_1 = private__0xe04_0xe04(arg1, 5203);
        return temp_0 - temp_1;
    }

    function private__0x989_0x989(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
        // block_0:
        uint256 temp_0;
        temp_0 = private__0x672_0x672(arg1, arg2, 2452);
        if (!(temp_0 == type(uint256).max)) {
          // block_2:
          if (temp_0 < arg0) {
            // block_4:
            revert(); // error_string, selector=0x8c379a0
          }
          // block_3:
          private__0x7c6_0x7c6(private__0x143e_0x143e(temp_0, arg0, 2577), arg1, arg2, 2582);
        }
        // block_1:
    }

    function private__0x156f_0x156f(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        temp_0 = private__0xd06_0xd06(arg0 + uint256(32), 37, 5465);
        return temp_0 + uint256(64);
    }

    function private__0x15fd_0x15fd(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        temp_0 = private__0xd06_0xd06(arg0 + uint256(32), 35, 5607);
        return temp_0 + uint256(64);
    }

    function private__0x161b_0x161b(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        uint256 temp_1;
        temp_0 = private__0xe04_0xe04(arg0, 5669);
        temp_1 = private__0xe04_0xe04(arg1, 5680);
        return temp_0 + temp_1;
    }

    function private__0xf20_0xf20(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 & uint256(255);
    }

    function private__0xf3b_0xf3b(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(32);
    }

    function private__0xf7f_0xf7f(uint256 arg0, uint256 arg1, uint256 arg2) internal {
        // block_0:
    }

    function private__0xf8e_0xf8e(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        private__0xf7f_0xf7f(arg1, arg0, 4001);
        return arg0 + uint256(32);
    }

    function private__0x672_0x672(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return slot_5[arg1 & uint256(1461501637330902918203684832716283019655932542975)];
    }

    function private__0x6f4_0x6f4(uint256 arg0, uint256 arg1) internal {
        // block_0:
        if ((slot_3 / uint256(256) ** uint256(0) ^ uint256(uint160(msg.sender))) & uint256(1461501637330902918203684832716283019655932542975) == 0) {
          // block_2:
        }
        // block_1:
    }

    function private__0x7c6_0x7c6(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
        // block_0:
        if (arg2 & uint256(1461501637330902918203684832716283019655932542975) == 0) {
          // block_4:
          revert(); // error_string, selector=0x8c379a0
        }
        else {
          // block_1:
          if (arg1 & uint256(1461501637330902918203684832716283019655932542975) == 0) {
            // block_3:
            revert(); // error_string, selector=0x8c379a0
          }
          else {
            // block_2:
            emit Event_0x8C5BE1E5EBEC7D5BD14F71427D1E84F3DD0314C0F7B2291E5B200AC8C7C3B925(arg2 & uint256(1461501637330902918203684832716283019655932542975), arg1 & uint256(1461501637330902918203684832716283019655932542975)); // TODO: recover event signature
          }
        }
    }

    function private__0xa1d_0xa1d(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
        // block_0:
        if (slot_4[arg2 & uint256(1461501637330902918203684832716283019655932542975)] < arg0) {
          // block_6:
          revert(); // error_string, selector=0x8c379a0
        }
        else {
          // block_1:
          if (arg2 & uint256(1461501637330902918203684832716283019655932542975) == 0) {
            // block_5:
            revert(); // error_string, selector=0x8c379a0
          }
          else {
            // block_2:
            if (arg1 & uint256(1461501637330902918203684832716283019655932542975) == 0) {
              // block_4:
              revert(); // error_string, selector=0x8c379a0
            }
            else {
              // block_3:
              emit Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF(arg2 & uint256(1461501637330902918203684832716283019655932542975), arg1 & uint256(1461501637330902918203684832716283019655932542975)); // TODO: recover event signature
            }
          }
        }
    }

    function private__0xd06_0xd06(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(32);
    }

    function private__0xdc9_0xdc9(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 & uint256(1461501637330902918203684832716283019655932542975);
    }

    function private__0xdda_0xdda(uint256 arg0, uint256 arg1) internal {
        // block_0:
        require(arg0 == private__0xdc9_0xdc9(arg0, 3555));
        // block_1:
    }

    function private__0xe04_0xe04(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0;
    }

    function private__0xe0d_0xe0d(uint256 arg0, uint256 arg1) internal {
        // block_0:
        require(arg0 == private__0xe04_0xe04(arg0, 3606));
        // block_1:
    }

    function private__0xe8f_0xe8f(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(32);
    }

    function private__0xeb7_0xeb7(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(32);
    }
}
