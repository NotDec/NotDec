contract Decompiled {
    event Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0x78009E5656A5C60B3C047015FB856B2EFBC6F42BEED76119406D7D4E3FC161F4();
    event Event_0x8BE0079C531659141344CD1FD0A4F28419497F9722A3DAAFE3B4186F6B6457E0(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0x29ED388F3DD39B342F312D7B12CBA9E3065871BF0FB668CC5457F217B15DD7C();
    event Event_0x69ADA53ADDDE5123341CE3A822C5F66292103B2771E41E1F3C00C2DE8A63A7F9();
    event Event_0x8C5BE1E5EBEC7D5BD14F71427D1E84F3DD0314C0F7B2291E5B200AC8C7C3B925(uint256 indexed arg0, uint256 indexed arg1);

    uint256 public slot_0;
    mapping(uint256 => uint256) public slot_1;
    uint256 public slot_10;
    uint256 public slot_11;
    mapping(uint256 => uint256) public slot_2;
    mapping(uint256 => uint256) public slot_3;
    uint256 public slot_4;
    uint256 public slot_5;
    uint256 public slot_6;
    uint256 public slot_7;
    uint256 public slot_8;
    uint256 public slot_9;

    function public_0x0faee56f() public returns (uint256 ret0) {
        // block_0:
        return 0 /* TODO: unresolved value: private.ret */;
    }

    function public_0xbf474bed() public returns (uint256 ret0) {
        // block_0:
        return 0 /* TODO: unresolved value: private.ret */;
    }

    function public_0xeeeeeeee() public {
        // block_0:
    }

    function maxTxAmount() public returns (uint256 ret0) {
        // block_0:
        return slot_8;
    }

    function maxWalletSize() public returns (uint256 ret0) {
        // block_0:
        return slot_9;
    }

    function allowance(address arg0, address arg1) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 67);
        // block_2:
        private__0x14e5_0x14e5(uint256(uint160(arg0)), 5563);
        private__0x14e5_0x14e5(uint256(uint160(arg1)), 5579);
        return slot_2[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0))];
    }

    function approve(address arg0, uint256 arg1) public returns (bool ret0) {
        // block_0:
        require(msg.data.length > 67);
        // block_2:
        private__0x14e5_0x14e5(uint256(uint160(arg0)), 5400);
        return private__0x48a_0x48a(arg1, uint256(uint160(arg0)), 192605) != 0;
    }

    function balanceOf(address arg0) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        private__0x14e5_0x14e5(uint256(uint160(arg0)), 266559);
        return slot_1[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0))];
    }

    function decimals() public returns (uint256 ret0) {
        // block_0:
        return 9;
    }

    function initialize() public {
        // block_0:
    }

    function launch() public {
        // block_0:
        private__0x399_0x399(192572);
    }

    function name() public {
        // block_0:
    }

    function owner() public returns (uint256 ret0) {
        // block_0:
        return slot_0 & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function renounceOwnership() public {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0) {
          // block_1:
          emit Event_0x8BE0079C531659141344CD1FD0A4F28419497F9722A3DAAFE3B4186F6B6457E0(slot_0 & (uint256(1) << uint256(160)) + uint256(type(uint256).max), 0); // TODO: recover event signature
          slot_0 = slot_0 & uint256(0) - (uint256(1) << uint256(160));
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function setTax(uint256 arg0, uint256 arg1) public {
        // block_0:
        require(msg.data.length > 67);
        // block_1:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0) {
          // block_2:
          slot_5 = arg0;
          slot_6 = arg1;
          emit Event_0x78009E5656A5C60B3C047015FB856B2EFBC6F42BEED76119406D7D4E3FC161F4(); // TODO: recover event signature
        }
        else {
          // block_3:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function symbol() public {
        // block_0:
    }

    function toggleLimits() public {
        // block_0:
        private__0x4bc_0x4bc(192697);
    }

    function totalSupply() public returns (uint256 ret0) {
        // block_0:
        return 0 /* TODO: unresolved value: private__0x577_0x577 */;
    }

    function transferFrom(address arg0, address arg1, uint256 arg2) public returns (uint256 ret0) {
        // block_0:
        if (!(msg.data.length > 99)) {
          // goto block_55
        }
        // goto block_1
        // block_5:
        if (!((slot_0 ^ uint256(uint160(arg1))) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
          // goto block_6
        }
        // goto block_43
        // block_6:
        if (slot_11 / (uint256(1) << uint256(160)) & uint256(255) == 0) {
          // goto block_7
        }
        // goto block_9
        // block_7:
        if (slot_3[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_8
        }
        // goto block_9
        // block_8:
        if (slot_3[uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_51
        }
        // goto block_9
        // block_18:
        if ((slot_11 ^ uint256(uint160(arg0))) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // goto block_19
        }
        // goto block_27
        // block_19:
        if (!(uint256(uint160(address(this))) == uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max))) {
          // goto block_20
        }
        // goto block_27
        // block_20:
        if (slot_7 > 40) {
          // goto block_21
        }
        // goto block_24
        // block_9:
        if ((slot_11 ^ uint256(uint160(arg0))) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // block_10:
          if (!((slot_10 ^ uint256(uint160(arg1))) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
            // block_11:
            if (slot_3[uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
              // block_12:
              if (!(slot_11 / (uint256(1) << uint256(168)) & uint256(255) == 0)) {
                // block_13:
                if (arg2 > slot_8) {
                  // block_50:
                  revert(); // error_string, selector=0x8c379a0, string_length=25
                }
                // block_14:
                if (!(private__0x1789_0x1789(slot_1[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg1))], arg2, 3568) > slot_9)) {
                  // goto block_15
                }
                // goto block_49
                // block_49:
                revert(); // error_string, selector=0x8c379a0, string_length=26
              }
              // block_15:
              slot_7 = slot_7 + uint256(1);
            }
          }
        }
        // block_16:
        if (!((slot_11 ^ uint256(uint160(arg1))) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
          // goto block_18
        }
        // block_17:
        if (uint256(uint160(address(this))) == uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)) {
          // goto block_18
        }
        // goto block_22
        // block_22:
        if (slot_7 > 40) {
          // block_21:
        }
        // block_24:
        if (!false /* TODO: evm.eq165.i */) {
          // block_25:
          revert(); // error_string, selector=0x8c379a0, string_length=33
        }
        // block_26:
        // goto block_27
        // block_27:
        if (slot_11 / (uint256(1) << uint256(176)) & uint256(255) == 0) {
          // block_28:
        }
        // block_29:
        if (!false /* TODO: _0xf30_0x0.i.in.reload */) {
          // goto block_31
        }
        // block_30:
        // block_31:
        if (!false /* TODO: _0xf45_0x0.i.reload */) {
          // goto block_32
        }
        // goto block_43
        // block_32:
        if (false /* TODO: evm.gt212.i.not */) {
          // goto block_33
        }
        // goto block_43
        // block_33:
        if (slot_7 < 21) {
          // goto block_43
        }
        else {
          // goto block_34
        }
        // block_34:
        if (false /* TODO: evm.gt225.i */) {
          // block_35:
        }
        // block_36:
        slot_11 = uint256(1) << uint256(176) | slot_11 & ~(uint256(255) << uint256(176));
        if (false /* TODO: evm.iszero258.i.not */) {
          // goto block_48
        }
        while (true) {
          // block_37:
          // block_38:
          if (!false /* TODO: evm.lt295.i.not */) {
            break;
          }
          continue;
        }
        // goto block_41
        // block_39:
        require(!false /* TODO: evm.iszero307.i.not */);
        // block_40:
        if (false /* TODO: evm.iszero313.i.not */) {
          // block_45:
          revert(); // returndata_forward
        }
        // block_41:
        slot_11 = slot_11 & ~(uint256(255) << uint256(176));
        if (!(address(this).balance == 0)) {
          // block_42:
        }
        // block_43:
        if (!false /* TODO: evm.iszero326.i */) {
          // block_44:
          emit Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF(uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max), uint256(uint160(address(this)))); // TODO: recover event signature
        }
        // block_54:
        emit Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF(uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max), uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)); // TODO: recover event signature
        return 1;
        // block_48:
        revert(); // returndata_forward
        // block_51:
        revert(); // error_string, selector=0x8c379a0, string_length=23
        // block_1:
        private__0x14e5_0x14e5(uint256(uint160(arg0)), 5443);
        private__0x14e5_0x14e5(uint256(uint160(arg1)), 5459);
        if (uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // block_53:
          revert(); // error_string, selector=0x8c379a0, string_length=32
        }
        // block_2:
        if (uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // goto block_53
        }
        // goto block_3
        // block_3:
        // block_52:
        revert(); // error_string, selector=0x8c379a0, string_length=41
        // block_55:
        revert(); // empty
    }

    function transfer(address arg0, uint256 arg1) public returns (uint256 ret0) {
        // block_0:
        if (!(msg.data.length > 67)) {
          // goto block_1
        }
        // goto block_2
        // block_1:
        revert(); // empty
        // block_6:
        if (!((slot_0 ^ uint256(uint160(arg0))) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
          // goto block_7
        }
        // goto block_44
        // block_7:
        if (slot_11 / (uint256(1) << uint256(160)) & uint256(255) == 0) {
          // goto block_8
        }
        // goto block_10
        // block_8:
        if (slot_3[uint256(uint160(msg.sender)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_9
        }
        // goto block_10
        // block_9:
        if (slot_3[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_52
        }
        // goto block_10
        // block_19:
        if ((slot_11 ^ uint256(uint160(msg.sender))) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // goto block_20
        }
        // goto block_28
        // block_20:
        if (!(uint256(uint160(address(this))) == uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max))) {
          // goto block_21
        }
        // goto block_28
        // block_21:
        if (slot_7 > 40) {
          // goto block_22
        }
        // goto block_25
        // block_10:
        if ((slot_11 ^ uint256(uint160(msg.sender))) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // block_11:
          if (!((slot_10 ^ uint256(uint160(arg0))) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
            // block_12:
            if (slot_3[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
              // block_13:
              if (!(slot_11 / (uint256(1) << uint256(168)) & uint256(255) == 0)) {
                // block_14:
                if (arg1 > slot_8) {
                  // block_51:
                  revert(); // error_string, selector=0x8c379a0, string_length=25
                }
                // block_15:
                if (!(private__0x1789_0x1789(slot_1[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0))], arg1, 3568) > slot_9)) {
                  // goto block_16
                }
                // goto block_50
                // block_50:
                revert(); // error_string, selector=0x8c379a0, string_length=26
              }
              // block_16:
              slot_7 = slot_7 + uint256(1);
            }
          }
        }
        // block_17:
        if (!((slot_11 ^ uint256(uint160(arg0))) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
          // goto block_19
        }
        // block_18:
        if (uint256(uint160(address(this))) == uint256(uint160(msg.sender)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)) {
          // goto block_19
        }
        // goto block_23
        // block_23:
        if (slot_7 > 40) {
          // block_22:
        }
        // block_25:
        if (!false /* TODO: evm.eq165.i.i */) {
          // block_26:
          revert(); // error_string, selector=0x8c379a0, string_length=33
        }
        // block_27:
        // goto block_28
        // block_28:
        if (slot_11 / (uint256(1) << uint256(176)) & uint256(255) == 0) {
          // block_29:
        }
        // block_30:
        if (!false /* TODO: _0xf30_0x0.i.i.in.reload */) {
          // goto block_32
        }
        // block_31:
        // block_32:
        if (!false /* TODO: _0xf45_0x0.i.i.reload */) {
          // goto block_33
        }
        // goto block_44
        // block_33:
        if (false /* TODO: evm.gt212.i.i.not */) {
          // goto block_34
        }
        // goto block_44
        // block_34:
        if (slot_7 < 21) {
          // goto block_44
        }
        else {
          // goto block_35
        }
        // block_35:
        if (false /* TODO: evm.gt225.i.i */) {
          // block_36:
        }
        // block_37:
        slot_11 = uint256(1) << uint256(176) | slot_11 & ~(uint256(255) << uint256(176));
        if (false /* TODO: evm.iszero258.i.i.not */) {
          // goto block_49
        }
        while (true) {
          // block_38:
          // block_39:
          if (!false /* TODO: evm.lt295.i.i.not */) {
            break;
          }
          continue;
        }
        // goto block_42
        // block_40:
        require(!false /* TODO: evm.iszero307.i.i.not */);
        // block_41:
        if (false /* TODO: evm.iszero313.i.i.not */) {
          // block_46:
          revert(); // returndata_forward
        }
        // block_42:
        slot_11 = slot_11 & ~(uint256(255) << uint256(176));
        if (!(address(this).balance == 0)) {
          // block_43:
        }
        // block_44:
        if (!false /* TODO: evm.iszero326.i.i */) {
          // block_45:
          emit Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF(uint256(uint160(msg.sender)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max), uint256(uint160(address(this)))); // TODO: recover event signature
        }
        // block_55:
        emit Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF(uint256(uint160(msg.sender)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max), uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)); // TODO: recover event signature
        return 1;
        // block_49:
        revert(); // returndata_forward
        // block_52:
        revert(); // error_string, selector=0x8c379a0, string_length=23
        // block_2:
        private__0x14e5_0x14e5(uint256(uint160(arg0)), 5400);
        if (uint256(uint160(msg.sender)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // block_54:
          revert(); // error_string, selector=0x8c379a0, string_length=32
        }
        // block_3:
        if (uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // goto block_54
        }
        // goto block_4
        // block_4:
        // block_53:
        revert(); // error_string, selector=0x8c379a0, string_length=41
    }

    function private__0x112a_0x112a(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal returns (uint256 ret0) {
        // block_0:
        if (arg1 > arg2) {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
        else {
          // block_1:
          return private__0x17b4_0x17b4(arg2, arg1, 266215);
        }
    }

    function private__0x1162_0x1162(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        if (arg1 == 0) {
          // block_2:
          return 0;
        }
        else {
          // block_1:
          temp_0 = private__0x170d_0x170d(arg1, arg0, 4476);
          if (private__0x17c7_0x17c7(temp_0, arg1, 4489) == arg0) {
            // block_3:
            return temp_0;
          }
          else {
            // block_4:
            revert(); // error_string, selector=0x8c379a0, string_length=33
          }
        }
    }

    function private__0x1410_0x1410(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        temp_0 = private__0x1789_0x1789(arg1, arg0, 5148);
        if (temp_0 < arg1) {
          // block_2:
          revert(); // error_string, selector=0x8c379a0, string_length=27
        }
        else {
          // block_1:
          return temp_0;
        }
    }

    function private__0x146e_0x146e(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal returns (uint256 ret0) {
        // block_0:
        if (arg1 == 0) {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
        else {
          // block_1:
          return private__0x17c7_0x17c7(arg2, arg1, 266519);
        }
    }

    function private__0x14e5_0x14e5(uint256 arg0, uint256 arg1) internal {
        // block_0:
        require(arg0 & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_1:
    }

    function private__0x15d6_0x15d6(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(96);
    }

    function private__0x170d_0x170d(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg1 * arg0;
    }

    function private__0x1789_0x1789(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg1 + arg0;
    }

    function private__0x17b4_0x17b4(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 - arg1;
    }

    function private__0x17c7_0x17c7(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 / arg1;
    }

    function private__0x399_0x399(uint256 arg0) internal {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0) {
          // block_1:
          require(slot_11 / (uint256(1) << uint256(160)) & uint256(255) == 0, "trading already open");
          // block_2:
          slot_11 = uint256(16777217) << uint256(160) | slot_11 & ~(uint256(4278190335) << uint256(160));
          emit Event_0x29ED388F3DD39B342F312D7B12CBA9E3065871BF0FB668CC5457F217B15DD7C(); // TODO: recover event signature
        }
        else {
          // block_4:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function private__0x48a_0x48a(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return 1;
    }

    function private__0x4bc_0x4bc(uint256 arg0) internal {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0) {
          // block_1:
          slot_11 = slot_11 & ~(uint256(255) << uint256(168));
          emit Event_0x69ADA53ADDDE5123341CE3A822C5F66292103B2771E41E1F3C00C2DE8A63A7F9(); // TODO: recover event signature
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
    }
}
