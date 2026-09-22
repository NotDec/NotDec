contract Decompiled {
    event Event_0x8C5BE1E5EBEC7D5BD14F71427D1E84F3DD0314C0F7B2291E5B200AC8C7C3B925(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0x17BBFB9A6069321B6DED73BD96327C9E6B7212A5CD51FF219CD61370ACAFB561();
    event Event_0x8BE0079C531659141344CD1FD0A4F28419497F9722A3DAAFE3B4186F6B6457E0(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0xF7EDD1A72D399EB95C56C07C5A26F00A9096735269C96C75CAA8FC4E15BCD5D2();

    uint256 public slot_0;
    mapping(uint256 => uint256) public slot_1;
    uint256 public slot_10;
    uint256 public slot_11;
    uint256 public slot_12;
    uint256 public slot_13;
    uint256 public slot_14;
    uint256 public slot_15;
    uint256 public slot_16;
    uint256 public slot_17;
    uint256 public slot_18;
    uint256 public slot_19;
    mapping(uint256 => uint256) public slot_2;
    uint256 public slot_20;
    uint256 public slot_21;
    uint256 public slot_22;
    mapping(uint256 => uint256) public slot_3;
    mapping(uint256 => uint256) public slot_4;
    mapping(uint256 => uint256) public slot_5;
    uint256[] public slot_6;
    uint256 public slot_7;
    uint256 public slot_8;
    uint256 public slot_9;

    function MAX_FEE() public returns (uint256 ret0) {
        // block_0:
        return 2500;
    }

    function VERSION() public returns (uint256 ret0) {
        // block_0:
        return 2;
    }

    function public_0xeeeeeeee() public {
        // block_0:
    }

    function charityAddress() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_21 & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function charityFee() public returns (uint256 ret0) {
        // block_0:
        return slot_17;
    }

    function liquidityFee() public returns (uint256 ret0) {
        // block_0:
        return slot_15;
    }

    function taxFee() public returns (uint256 ret0) {
        // block_0:
        return slot_13;
    }

    function allowance(address arg0, address arg1) public returns (uint256 ret0) {
        // block_0:
        return slot_3[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0))];
    }

    function approve(address arg0, uint256 arg1) public returns (bool ret0) {
        // block_0:
        return private__0x80a_0x80a(arg1, uint256(uint160(arg0)), 253733) != 0;
    }

    function balanceOf(address arg0) public returns (uint256 ret0) {
        // block_0:
        return 0 /* TODO: unresolved value: private__0xe4f_0xe4f */;
    }

    function decimals() public returns (uint256 ret0) {
        // block_0:
        return slot_12;
    }

    function decreaseAllowance(address arg0, uint256 arg1) public returns (bool ret0) {
        // block_0:
        return private__0x1032_0x1032(arg1, uint256(uint160(arg0)), 254985) != 0;
    }

    function deliver(uint256 arg0) public {
        // block_0:
        if (slot_5[uint256(uint160(msg.sender))] == 0) {
          // block_1:
          slot_9 = private__0x14e0_0x14e0(arg0, slot_9, 3095);
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0, string_length=44
        }
    }

    function excludeFromFee(address arg0) public {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0) {
          // block_1:
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function excludeFromReward(address arg0) public {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0) {
          // goto block_1
        }
        // goto block_6
        // block_1:
        if (!(slot_5[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_5
        }
        // goto block_2
        // block_2:
        if (!(slot_1[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // block_3:
        }
        // block_4:
        // block_5:
        revert(); // error_string, selector=0x8c379a0, string_length=27
        // block_6:
        revert(); // error_string, selector=0x8c379a0
    }

    function includeInReward(address arg0) public {
        // block_0:
    }

    function increaseAllowance(address arg0, uint256 arg1) public returns (bool ret0) {
        // block_0:
        return private__0xafd_0xafd(arg1, uint256(uint160(arg0)), 254220) != 0;
    }

    function isExcludedFromFee(address arg0) public returns (bool ret0) {
        // block_0:
        return slot_4[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0))] != 0;
    }

    function isExcludedFromReward(address arg0) public returns (bool ret0) {
        // block_0:
        return slot_5[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0))] != 0;
    }

    function name() public {
        // block_0:
    }

    function owner() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0 & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function reflectionFromToken(uint256 arg0, bool arg1) public returns (uint256 ret0) {
        // block_0:
        return 0 /* TODO: unresolved value: private__0xc6d_0xc6d */;
    }

    function renounceOwnership() public {
        // block_0:
        private__0xeae_0xeae(254740);
    }

    function setCharityFeePercent(uint256 arg0) public {
        // block_0:
        private__0x108e_0x108e(arg0, 255079);
    }

    function setLiquidityFeePercent(uint256 arg0) public {
        // block_0:
        private__0xfde_0xfde(arg0, 254908);
    }

    function setSwapBackSettings(uint256 arg0) public {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0) {
          // block_1:
          if (arg0 < private__0x116b_0x116b(10000, private__0xf22_0xf22(slot_7, 5, 392839), 3886)) {
            // block_3:
            revert(); // error_string, selector=0x8c379a0, string_length=56
          }
          else {
            // block_2:
            slot_22 = arg0;
            emit Event_0xF7EDD1A72D399EB95C56C07C5A26F00A9096735269C96C75CAA8FC4E15BCD5D2(); // TODO: recover event signature
          }
        }
        else {
          // block_4:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function setTaxFeePercent(uint256 arg0) public {
        // block_0:
        private__0x6d8_0x6d8(arg0, 253616);
    }

    function swapAndLiquifyEnabled() public returns (bool ret0) {
        // block_0:
        return slot_21 / (uint256(1) << uint256(168)) & uint256(255) != 0;
    }

    function symbol() public {
        // block_0:
    }

    function tokenFromReflection(uint256 arg0) public returns (uint256 ret0) {
        // block_0:
        return 0 /* TODO: unresolved value: private__0x88a_0x88a */;
    }

    function totalFees() public returns (uint256 ret0) {
        // block_0:
        return slot_9;
    }

    function totalSupply() public returns (uint256 ret0) {
        // block_0:
        return slot_7;
    }

    function transferFrom(address arg0, address arg1, uint256 arg2) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 99);
        // block_1:
        private__0x24cc_0x24cc(uint256(uint160(arg0)), 8769);
        private__0x24cc_0x24cc(uint256(uint160(arg1)), 8785);
        return 1;
    }

    function transferOwnership(address arg0) public {
        // block_0:
        private__0x10d3_0x10d3(uint256(uint160(arg0)), 255202);
    }

    function transfer(address arg0, uint256 arg1) public returns (bool ret0) {
        // block_0:
        return private__0x1081_0x1081(arg1, uint256(uint160(arg0)), 255032) != 0;
    }

    function uniswapV2Pair() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_20 & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function uniswapV2Router() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_19 & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function private__0x1081_0x1081(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return 1;
    }

    function private__0x108e_0x108e(uint256 arg0, uint256 arg1) internal {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0) {
          // block_1:
          slot_17 = arg0;
          require(private__0x23f2_0x23f2(private__0x23f2_0x23f2(slot_13, slot_15, 529974), arg0, 1839) < 2501, "Total fee is over 25%");
          // block_2:
        }
        else {
          // block_4:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function private__0x116b_0x116b(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg1 / arg0;
    }

    function private__0x1032_0x1032(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return 1;
    }

    function private__0x10d3_0x10d3(uint256 arg0, uint256 arg1) internal {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0) {
          // block_1:
          if (arg0 & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
            // block_3:
            revert(); // error_string, selector=0x8c379a0, string_length=38
          }
          else {
            // block_2:
            private__0x1553_0x1553(arg0, 530055);
          }
        }
        else {
          // block_4:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function private__0x1177_0x1177(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
        // block_0:
        if (arg2 & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // block_4:
          revert(); // error_string, selector=0x8c379a0, string_length=36
        }
        else {
          // block_1:
          if (arg1 & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
            // block_3:
            revert(); // error_string, selector=0x8c379a0, string_length=34
          }
          else {
            // block_2:
            emit Event_0x8C5BE1E5EBEC7D5BD14F71427D1E84F3DD0314C0F7B2291E5B200AC8C7C3B925((uint256(1) << uint256(160)) + uint256(type(uint256).max) & arg2, arg1 & (uint256(1) << uint256(160)) + uint256(type(uint256).max)); // TODO: recover event signature
          }
        }
    }

    function private__0x1491_0x1491(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal returns (uint256 ret0) {
        // block_0:
        if (arg1 > arg2) {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
        else {
          // block_1:
          return arg2 - arg1;
        }
    }

    function private__0x14e0_0x14e0(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return private__0x23f2_0x23f2(arg1, arg0, 530206);
    }

    function private__0x1547_0x1547(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return private__0x2449_0x2449(arg1, arg0, 530244);
    }

    function private__0x1553_0x1553(uint256 arg0, uint256 arg1) internal {
        // block_0:
        slot_0 = (uint256(1) << uint256(160)) + uint256(type(uint256).max) & arg0 | slot_0 & uint256(0) - (uint256(1) << uint256(160));
        emit Event_0x8BE0079C531659141344CD1FD0A4F28419497F9722A3DAAFE3B4186F6B6457E0(slot_0 & (uint256(1) << uint256(160)) + uint256(type(uint256).max), (uint256(1) << uint256(160)) + uint256(type(uint256).max) & arg0); // TODO: recover event signature
    }

    function private__0x15a3_0x15a3(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg1 * arg0;
    }

    function private__0x1f04_0x1f04(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
        // block_0:
        emit Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF((uint256(1) << uint256(160)) + uint256(type(uint256).max) & arg2, (uint256(1) << uint256(160)) + uint256(type(uint256).max) & arg1); // TODO: recover event signature
    }

    function private__0x1ca5_0x1ca5(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
        // block_0:
        emit Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF((uint256(1) << uint256(160)) + uint256(type(uint256).max) & arg2, (uint256(1) << uint256(160)) + uint256(type(uint256).max) & arg1); // TODO: recover event signature
    }

    function private__0x1deb_0x1deb(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
        // block_0:
        emit Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF((uint256(1) << uint256(160)) + uint256(type(uint256).max) & arg2, (uint256(1) << uint256(160)) + uint256(type(uint256).max) & arg1); // TODO: recover event signature
    }

    function private__0x1eaa_0x1eaa(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
        // block_0:
        emit Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF((uint256(1) << uint256(160)) + uint256(type(uint256).max) & arg2, (uint256(1) << uint256(160)) + uint256(type(uint256).max) & arg1); // TODO: recover event signature
    }

    function private__0x2086_0x2086(uint256 arg0, uint256 arg1) internal {
        // block_0:
        if (!(arg0 == 0)) {
          // block_2:
          if (!(slot_5[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_21] == 0)) {
            // block_3:
          }
          // block_4:
          emit Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF((uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(msg.sender)), (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_21); // TODO: recover event signature
        }
        // block_1:
    }

    function private__0x218e_0x218e(uint256 arg0, uint256 arg1, uint256 arg2) internal {
        // block_0:
        slot_8 = private__0x1547_0x1547(arg1, slot_8, 8603);
        slot_9 = private__0x14e0_0x14e0(arg0, slot_9, 8619);
    }

    function private__0x234d_0x234d(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(96);
    }

    function private__0x23f2_0x23f2(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + arg1;
    }

    function private__0x2449_0x2449(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 - arg1;
    }

    function private__0x249b_0x249b(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(1);
    }

    function private__0x24cc_0x24cc(uint256 arg0, uint256 arg1) internal {
        // block_0:
        require(arg0 & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_1:
    }

    function private__0x6d8_0x6d8(uint256 arg0, uint256 arg1) internal {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0) {
          // block_1:
          slot_13 = arg0;
          require(private__0x23f2_0x23f2(private__0x23f2_0x23f2(arg0, slot_15, 255320), slot_17, 1839) < 2501, "Total fee is over 25%");
          // block_2:
        }
        else {
          // block_4:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function private__0x80a_0x80a(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        private__0x1177_0x1177(arg0, arg1, uint256(uint160(msg.sender)), 255552);
        return 1;
    }

    function private__0xafd_0xafd(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        private__0x1177_0x1177(private__0x14e0_0x14e0(arg0, slot_3[uint256(uint160(msg.sender))], 392532), arg1, uint256(uint160(msg.sender)), 324099);
        return 1;
    }

    function private__0xeae_0xeae(uint256 arg0) internal {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0) {
          // block_1:
          private__0x1553_0x1553(0, 3810);
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function private__0xf22_0xf22(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 * arg1;
    }

    function private__0xfde_0xfde(uint256 arg0, uint256 arg1) internal {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0) {
          // block_1:
          slot_15 = arg0;
          require(private__0x23f2_0x23f2(private__0x23f2_0x23f2(slot_13, arg0, 392916), slot_17, 1839) < 2501, "Total fee is over 25%");
          // block_2:
        }
        else {
          // block_4:
          revert(); // error_string, selector=0x8c379a0
        }
    }
}
