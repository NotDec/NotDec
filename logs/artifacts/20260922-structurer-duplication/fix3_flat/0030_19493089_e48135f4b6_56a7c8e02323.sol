contract Decompiled {
    event Event_0x8C5BE1E5EBEC7D5BD14F71427D1E84F3DD0314C0F7B2291E5B200AC8C7C3B925(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF(uint256 indexed arg0, uint256 indexed arg1);

    mapping(uint256 => uint256) public slot_0;
    mapping(uint256 => uint256) public slot_1;
    uint256 public slot_2;
    uint256 public slot_3;
    uint256 public slot_4;
    uint256 public slot_5;

    function public_0xeeeeeeee() public {
        // block_0:
    }

    function allowance(address arg0, address arg1) public {
        // block_0:
        require(msg.data.length > 67);
        // block_2:
        private__0xd25_0xd25(uint256(uint160(arg0)), 3403);
        private__0xd25_0xd25(uint256(uint160(arg1)), 3403);
    }

    function approve(address arg0, uint256 arg1) public {
        // block_0:
        require(msg.data.length > 67);
        // block_2:
        private__0xd25_0xd25(uint256(uint160(arg0)), 3403);
        private__0xd5b_0xd5b(arg1, 3457);
        private__0x7ea_0x7ea(arg1, uint256(uint160(arg0)), private__0x7e2_0x7e2(930), 937);
    }

    function balanceOf(address arg0) public {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        private__0xd25_0xd25(uint256(uint160(arg0)), 3403);
    }

    function decimals() public {
        // block_0:
    }

    function decreaseAllowance(address arg0, uint256 arg1) public {
        // block_0:
        require(msg.data.length > 67);
        // block_2:
        private__0xd25_0xd25(uint256(uint160(arg0)), 3403);
        private__0xd5b_0xd5b(arg1, 3457);
        if (slot_1[private__0x7e2_0x7e2(1633) & uint256(1461501637330902918203684832716283019655932542975)] < arg1) {
          // block_4:
          revert(); // error_string, selector=0x8c379a0
        }
        else {
          // block_3:
          private__0x7ea_0x7ea(slot_1[private__0x7e2_0x7e2(1633) & uint256(1461501637330902918203684832716283019655932542975)] - arg1, uint256(uint160(arg0)), private__0x7e2_0x7e2(1833), 1842);
        }
    }

    function increaseAllowance(address arg0, uint256 arg1) public {
        // block_0:
        require(msg.data.length > 67);
        // block_2:
        private__0xd25_0xd25(uint256(uint160(arg0)), 3403);
        private__0xd5b_0xd5b(arg1, 3457);
        private__0x7ea_0x7ea(private__0x103f_0x103f(slot_1[private__0x7e2_0x7e2(1255) & uint256(1461501637330902918203684832716283019655932542975)], arg1, 1385), uint256(uint160(arg0)), private__0x7e2_0x7e2(1241), 1390);
    }

    function name() public {
        // block_0:
    }

    function symbol() public {
        // block_0:
    }

    function totalSupply() public {
        // block_0:
    }

    function transferFrom(address arg0, address arg1, uint256 arg2) public {
        // block_0:
        require(msg.data.length > 99);
        // block_2:
        private__0xd25_0xd25(uint256(uint160(arg0)), 3403);
        private__0xd25_0xd25(uint256(uint160(arg1)), 3403);
        private__0xd5b_0xd5b(arg2, 3457);
        private__0x9b3_0x9b3(arg2, uint256(uint160(arg1)), uint256(uint160(arg0)), 970);
        if (slot_1[uint256(uint160(arg0)) & uint256(1461501637330902918203684832716283019655932542975)] < arg2) {
          // block_4:
          revert(); // error_string, selector=0x8c379a0
        }
        else {
          // block_3:
          private__0x7ea_0x7ea(slot_1[uint256(uint160(arg0)) & uint256(1461501637330902918203684832716283019655932542975)] - arg2, private__0x7e2_0x7e2(1185), uint256(uint160(arg0)), 1193);
        }
    }

    function transfer(address arg0, uint256 arg1) public {
        // block_0:
        require(msg.data.length > 67);
        // block_2:
        private__0xd25_0xd25(uint256(uint160(arg0)), 3403);
        private__0xd5b_0xd5b(arg1, 3457);
        private__0x9b3_0x9b3(arg1, uint256(uint160(arg0)), private__0x7e2_0x7e2(1866), 1873);
    }

    function private__0x103f_0x103f(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        uint256 temp_1;
        temp_0 = private__0xd51_0xd51(arg0, 4170);
        temp_1 = private__0xd51_0xd51(arg1, 4181);
        return temp_0 + temp_1;
    }

    function private__0x10e5_0x10e5(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        temp_0 = private__0xc47_0xc47(arg0 + uint256(32), 37, 4303);
        return temp_0 + uint256(64);
    }

    function private__0x1177_0x1177(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        temp_0 = private__0xc47_0xc47(arg0 + uint256(32), 36, 4449);
        return temp_0 + uint256(64);
    }

    function private__0x1209_0x1209(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        temp_0 = private__0xc47_0xc47(arg0 + uint256(32), 34, 4595);
        return temp_0 + uint256(64);
    }

    function private__0x129b_0x129b(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        temp_0 = private__0xc47_0xc47(arg0 + uint256(32), 37, 4741);
        return temp_0 + uint256(64);
    }

    function private__0x132d_0x132d(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        temp_0 = private__0xc47_0xc47(arg0 + uint256(32), 35, 4887);
        return temp_0 + uint256(64);
    }

    function private__0x13bf_0x13bf(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        temp_0 = private__0xc47_0xc47(arg0 + uint256(32), 38, 5033);
        return temp_0 + uint256(64);
    }

    function private__0x7ea_0x7ea(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
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

    function private__0xde2_0xde2(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(32);
    }

    function private__0xe0c_0xe0c(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(32);
    }

    function private__0x7e2_0x7e2(uint256 arg0) internal returns (uint256 ret0) {
        // block_0:
        return uint256(uint160(msg.sender));
    }

    function private__0x9b3_0x9b3(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
        // block_0:
        if (arg2 & uint256(1461501637330902918203684832716283019655932542975) == 0) {
          // block_6:
          revert(); // error_string, selector=0x8c379a0
        }
        else {
          // block_1:
          if (arg1 & uint256(1461501637330902918203684832716283019655932542975) == 0) {
            // block_5:
            revert(); // error_string, selector=0x8c379a0
          }
          else {
            // block_2:
            private__0xc32_0xc32(arg0, arg1, arg2, 2716);
            if (slot_0[arg2 & uint256(1461501637330902918203684832716283019655932542975)] < arg0) {
              // block_4:
              revert(); // error_string, selector=0x8c379a0
            }
            else {
              // block_3:
              emit Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF(arg2 & uint256(1461501637330902918203684832716283019655932542975), arg1 & uint256(1461501637330902918203684832716283019655932542975)); // TODO: recover event signature
              private__0xc37_0xc37(arg0, arg1, arg2, 3116);
            }
          }
        }
    }

    function private__0xc32_0xc32(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
        // block_0:
    }

    function private__0xc37_0xc37(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
        // block_0:
    }

    function private__0xc47_0xc47(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(32);
    }

    function private__0xd13_0xd13(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 & uint256(1461501637330902918203684832716283019655932542975);
    }

    function private__0xd25_0xd25(uint256 arg0, uint256 arg1) internal {
        // block_0:
        require(arg0 == private__0xd13_0xd13(arg0, 3374));
        // block_1:
    }

    function private__0xd51_0xd51(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0;
    }

    function private__0xd5b_0xd5b(uint256 arg0, uint256 arg1) internal {
        // block_0:
        require(arg0 == private__0xd51_0xd51(arg0, 3428));
        // block_1:
    }

    function private__0xe96_0xe96(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(32);
    }

    function private__0xff0_0xff0(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        temp_0 = private__0xc47_0xc47(arg0 + uint256(32), 40, 4058);
        return temp_0 + uint256(64);
    }
}
