contract Decompiled {
    event Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0x8BE0079C531659141344CD1FD0A4F28419497F9722A3DAAFE3B4186F6B6457E0(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0x8C5BE1E5EBEC7D5BD14F71427D1E84F3DD0314C0F7B2291E5B200AC8C7C3B925(uint256 indexed arg0, uint256 indexed arg1);

    uint256 public slot_0;
    uint256 public slot_10;
    uint256 public slot_11;
    uint256 public slot_12;
    uint256 public slot_13;
    uint256 public slot_14;
    uint256 public slot_15;
    mapping(uint256 => uint256) public slot_16;
    mapping(uint256 => uint256) public slot_17;
    uint256 public slot_18;
    uint256 public slot_19;
    mapping(uint256 => uint256) public slot_2;
    uint256 public slot_20;
    uint256 public slot_21;
    uint256 public slot_22;
    uint256 public slot_23;
    uint256 public slot_24;
    mapping(uint256 => uint256) public slot_4;
    mapping(uint256 => uint256) public slot_5;
    uint256 public slot_6;
    uint256 public slot_7;
    uint256 public slot_8;
    uint256 public slot_9;

    function public_0xeeeeeeee() public {
        // block_0:
    }

    function buyMap(address arg0) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        private__0x198d_0x198d(uint256(uint160(arg0)), 337005);
        return slot_17[uint256(uint160(arg0))];
    }

    function maxTxAmount() public returns (uint256 ret0) {
        // block_0:
        return slot_22;
    }

    function maxWalletSize() public returns (uint256 ret0) {
        // block_0:
        return slot_23;
    }

    function swapTokensAtAmount() public returns (uint256 ret0) {
        // block_0:
        return slot_24;
    }

    function allowance(address arg0, address arg1) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 67);
        // block_2:
        private__0x198d_0x198d(uint256(uint160(arg0)), 7278);
        private__0x198d_0x198d(uint256(uint160(arg1)), 7294);
        return slot_4[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0))];
    }

    function approve(address arg0, uint256 arg1) public returns (bool ret0) {
        // block_0:
        require(msg.data.length > 67);
        // block_2:
        private__0x198d_0x198d(uint256(uint160(arg0)), 6890);
        return private__0x6b3_0x6b3(arg1, uint256(uint160(arg0)), 230441) != 0;
    }

    function balanceOf(address arg0) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        private__0x198d_0x198d(uint256(uint160(arg0)), 337005);
        return 0 /* TODO: unresolved value: private__0x84f_0x84f */;
    }

    function blockBots(address arg0) public {
        // block_0:
        require(msg.data.length > 35);
        // block_1:
        require(uint256(uint160(arg0)) < 18446744073709551616);
        // block_2:
        require(uint256(uint160(arg0)) + uint256(35) < msg.data.length);
        // block_3:
        // block_7:
        revert(); // empty
        if (false /* TODO: evm.gt27.i.not */) {
          // goto block_7
        }
        while (true) {
          // block_4:
          // block_5:
          if (!false /* TODO: evm.lt33.i.not */) {
            break;
          }
          continue;
        }
        // block_11:
        // TODO: recover remaining body
    }

    function bots(address arg0) public returns (bool ret0) {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        private__0x198d_0x198d(uint256(uint160(arg0)), 337005);
        return slot_16[uint256(uint160(arg0))] != 0;
    }

    function decimals() public returns (uint256 ret0) {
        // block_0:
        return 9;
    }

    function excludeMultipleAccountsFromFees(address arg0, bool arg1) public {
        // block_0:
        require(msg.data.length > 67);
        // block_1:
        require(uint256(uint160(arg0)) < 18446744073709551616);
        // block_2:
        require(uint256(uint160(arg0)) + uint256(35) < msg.data.length);
        // block_3:
        require(false /* TODO: evm.gt11.i */);
        // block_4:
        require(!false /* TODO: evm.gt18.i.not */);
        // block_5:
        require(false /* TODO: evm.eq.i.i */);
        // block_12:
        // block_15:
        revert(); // error_string, selector=0x8c379a0
        if (!(uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0)) {
          // goto block_15
        }
        while (false /* TODO: evm.lt.i.not */) {
          // block_14:
        }
        // block_16:
        // TODO: recover remaining body
    }

    function manualsend() public {
        // block_0:
    }

    function manualswap() public {
        // block_0:
        if ((uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_18) {
          // block_2:
          slot_21 = uint256(1) << uint256(168) | slot_21 & ~(uint256(255) << uint256(168));
          require(!false /* TODO: evm.iszero.i.i.not */);
          // block_3:
          // block_11:
          revert(); // returndata_forward
        }
        if (!((uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_18)) {
          // goto block_1
        }
        // goto block_2
        // block_1:
        if (!((uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_19)) {
          // goto block_13
        }
        // goto block_2
        while (true) {
          // block_4:
          // block_5:
          if (!false /* TODO: evm.lt57.i.i.not */) {
            break;
          }
          continue;
        }
        // block_6:
        require(!false /* TODO: evm.iszero70.i.i.not */);
        // block_7:
        if (false /* TODO: evm.iszero76.i.i.not */) {
          // block_8:
          revert(); // returndata_forward
        }
        else {
          // block_14:
          slot_21 = slot_21 & ~(uint256(255) << uint256(168));
        }
        // block_13:
        revert(); // empty
    }

    function name() public {
        // block_0:
    }

    function owner() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0 & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function removeLimits() public {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0) {
          // block_1:
          slot_22 = 25000000000000000;
          slot_23 = 25000000000000000;
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
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

    function setMaxTxnAmount(uint256 arg0) public {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0) {
          // block_3:
          slot_22 = arg0;
        }
        else {
          // block_4:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function setMaxWalletSize(uint256 arg0) public {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0) {
          // block_3:
          slot_23 = arg0;
        }
        else {
          // block_4:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function setMinSwapTokensThreshold(uint256 arg0) public {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0) {
          // block_3:
          slot_24 = arg0;
        }
        else {
          // block_4:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function setTrading(bool arg0) public {
        // block_0:
        require(msg.data.length > 35);
        // block_1:
        require(false /* TODO: evm.eq.i.i */);
        // block_4:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0) {
          // block_5:
        }
        else {
          // block_6:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function setfees(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) public {
        // block_0:
        require(msg.data.length > 131);
        // block_1:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0) {
          // block_2:
          slot_8 = arg0;
          slot_10 = arg1;
          slot_9 = arg2;
          slot_11 = arg3;
        }
        else {
          // block_3:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function symbol() public {
        // block_0:
    }

    function toggleSwap(bool arg0) public {
        // block_0:
        require(msg.data.length > 35);
        // block_1:
        require(false /* TODO: evm.eq.i.i */);
        // block_4:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0) {
          // block_5:
        }
        else {
          // block_6:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function totalSupply() public returns (uint256 ret0) {
        // block_0:
        return 25000000000000000;
    }

    function transferFrom(address arg0, address arg1, uint256 arg2) public returns (uint256 ret0) {
        // block_0:
        if (msg.data.length > 99) {
          // goto block_1
        }
        // goto block_54
        // block_1:
        private__0x198d_0x198d(uint256(uint160(arg0)), 6936);
        private__0x198d_0x198d(uint256(uint160(arg1)), 6952);
        if (!(uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
          // goto block_2
        }
        // goto block_52
        // block_2:
        if (uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // goto block_51
        }
        // goto block_3
        // block_8:
        if (arg2 > slot_22) {
          // goto block_48
        }
        // goto block_9
        while (true) {
          // block_25:
          // block_26:
          if (!false /* TODO: evm.lt57.i.i.not */) {
            break;
          }
          continue;
        }
        // goto block_34
        // block_9:
        if (!(slot_16[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // block_47:
          revert(); // error_string, selector=0x8c379a0, string_length=35
        }
        // block_10:
        if (!(slot_16[uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_47
        }
        // goto block_11
        // block_11:
        // block_13:
        if (!false /* TODO: evm.lt81.i */) {
          // block_14:
        }
        // block_15:
        // block_18:
        if (false /* TODO: _0x10b9_0x0.i.in.reload */) {
          // block_19:
        }
        // block_20:
        // block_21:
        // block_22:
        // block_23:
        slot_21 = uint256(1) << uint256(168) | slot_21 & ~(uint256(255) << uint256(168));
        require(!false /* TODO: evm.iszero.i.i.not */);
        // block_24:
        // block_32:
        revert(); // returndata_forward
        // block_27:
        require(!false /* TODO: evm.iszero70.i.i.not */);
        // block_28:
        if (false /* TODO: evm.iszero76.i.i.not */) {
          // block_29:
          revert(); // returndata_forward
        }
        // block_34:
        slot_21 = slot_21 & ~(uint256(255) << uint256(168));
        if (!(address(this).balance == 0)) {
          // block_35:
        }
        // block_36:
        if (!(slot_5[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // block_53:
          return 1;
        }
        // block_37:
        if (!(slot_5[uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_53
        }
        // goto block_38
        // block_38:
        // block_39:
        if (!((slot_21 ^ uint256(uint160(arg1))) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
          // goto block_53
        }
        // goto block_40
        // block_40:
        if ((slot_21 ^ uint256(uint160(arg0))) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // block_41:
          if (!((slot_20 ^ uint256(uint160(arg1))) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
            // block_42:
            slot_12 = slot_8;
            slot_13 = slot_9;
          }
        }
        // block_43:
        if (!((slot_21 ^ uint256(uint160(arg1))) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
          // goto block_53
        }
        // block_44:
        if (!((slot_20 ^ uint256(uint160(arg0))) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
          // block_45:
          slot_12 = slot_10;
          slot_13 = slot_11;
        }
        // goto block_53
        // block_46:
        revert(); // error_string, selector=0x8c379a0, string_length=35
        // block_48:
        revert(); // error_string, selector=0x8c379a0, string_length=28
        // block_3:
        if (arg2 == 0) {
          // block_50:
          revert(); // error_string, selector=0x8c379a0, string_length=41
        }
        // block_4:
        // block_5:
        // block_6:
        // block_7:
        if ((slot_0 ^ uint256(uint160(arg0))) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // goto block_8
        }
        // goto block_49
        // block_49:
        revert(); // error_string, selector=0x8c379a0, string_length=63
        // block_51:
        revert(); // error_string, selector=0x8c379a0, string_length=35
        // block_52:
        revert(); // error_string, selector=0x8c379a0, string_length=37
        // block_54:
        revert(); // empty
    }

    function transferOwnership(address arg0) public {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        private__0x198d_0x198d(uint256(uint160(arg0)), 337005);
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0) {
          // block_3:
          if (uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
            // block_5:
            revert(); // error_string, selector=0x8c379a0, string_length=38
          }
          else {
            // block_4:
            emit Event_0x8BE0079C531659141344CD1FD0A4F28419497F9722A3DAAFE3B4186F6B6457E0(slot_0 & (uint256(1) << uint256(160)) + uint256(type(uint256).max), uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)); // TODO: recover event signature
            slot_0 = (uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0)) | slot_0 & uint256(0) - (uint256(1) << uint256(160));
          }
        }
        else {
          // block_6:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function transfer(address arg0, uint256 arg1) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 67);
        // block_2:
        private__0x198d_0x198d(uint256(uint160(arg0)), 6890);
        if (!(uint256(uint160(msg.sender)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
          // goto block_3
        }
        // goto block_53
        // block_3:
        if (uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // goto block_52
        }
        // goto block_4
        // block_9:
        if (arg1 > slot_22) {
          // goto block_49
        }
        // goto block_10
        while (true) {
          // block_26:
          // block_27:
          if (!false /* TODO: evm.lt57.i.i.i.not */) {
            break;
          }
          continue;
        }
        // goto block_35
        // block_10:
        if (!(slot_16[uint256(uint160(msg.sender)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // block_48:
          revert(); // error_string, selector=0x8c379a0, string_length=35
        }
        // block_11:
        if (!(slot_16[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_48
        }
        // goto block_12
        // block_12:
        // block_14:
        if (!false /* TODO: evm.lt81.i.i */) {
          // block_15:
        }
        // block_16:
        // block_19:
        if (false /* TODO: _0x10b9_0x0.i.i.in.reload */) {
          // block_20:
        }
        // block_21:
        // block_22:
        // block_23:
        // block_24:
        slot_21 = uint256(1) << uint256(168) | slot_21 & ~(uint256(255) << uint256(168));
        require(!false /* TODO: evm.iszero.i.i.i.not */);
        // block_25:
        // block_33:
        revert(); // returndata_forward
        // block_28:
        require(!false /* TODO: evm.iszero70.i.i.i.not */);
        // block_29:
        if (false /* TODO: evm.iszero76.i.i.i.not */) {
          // block_30:
          revert(); // returndata_forward
        }
        // block_35:
        slot_21 = slot_21 & ~(uint256(255) << uint256(168));
        if (!(address(this).balance == 0)) {
          // block_36:
        }
        // block_37:
        if (!(slot_5[uint256(uint160(msg.sender)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // block_54:
          return 1;
        }
        // block_38:
        if (!(slot_5[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_54
        }
        // goto block_39
        // block_39:
        // block_40:
        if (!((slot_21 ^ uint256(uint160(arg0))) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
          // goto block_54
        }
        // goto block_41
        // block_41:
        if ((slot_21 ^ uint256(uint160(msg.sender))) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // block_42:
          if (!((slot_20 ^ uint256(uint160(arg0))) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
            // block_43:
            slot_12 = slot_8;
            slot_13 = slot_9;
          }
        }
        // block_44:
        if (!((slot_21 ^ uint256(uint160(arg0))) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
          // goto block_54
        }
        // block_45:
        if (!((slot_20 ^ uint256(uint160(msg.sender))) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
          // block_46:
          slot_12 = slot_10;
          slot_13 = slot_11;
        }
        // goto block_54
        // block_49:
        revert(); // error_string, selector=0x8c379a0, string_length=28
        // block_4:
        if (arg1 == 0) {
          // block_51:
          revert(); // error_string, selector=0x8c379a0, string_length=41
        }
        // block_5:
        // block_6:
        // block_7:
        // block_8:
        if ((slot_0 ^ uint256(uint160(msg.sender))) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // goto block_9
        }
        // goto block_50
        // block_50:
        revert(); // error_string, selector=0x8c379a0, string_length=63
        // block_52:
        revert(); // error_string, selector=0x8c379a0, string_length=35
        // block_53:
        revert(); // error_string, selector=0x8c379a0, string_length=37
    }

    function unblockBot(address arg0) public {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        private__0x198d_0x198d(uint256(uint160(arg0)), 337005);
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0) {
          // block_3:
        }
        else {
          // block_4:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function uniswapV2Pair() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_21 & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function uniswapV2Router() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_20 & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function private__0x1242_0x1242(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal returns (uint256 ret0) {
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

    function private__0x16b9_0x16b9(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal returns (uint256 ret0) {
        // block_0:
        if (arg1 == 0) {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
        else {
          // block_1:
          return private__0x1dc2_0x1dc2(arg2, arg1, 336596);
        }
    }

    function private__0x1786_0x1786(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        temp_0 = private__0x1d05_0x1d05(arg1, arg0, 6035);
        if (temp_0 < arg1) {
          // block_2:
          revert(); // error_string, selector=0x8c379a0, string_length=27
        }
        else {
          // block_1:
          return temp_0;
        }
    }

    function private__0x18f8_0x18f8(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        if (arg1 == 0) {
          // block_2:
          return 0;
        }
        else {
          // block_1:
          if (private__0x1dc2_0x1dc2(arg1 * arg0, arg1, 6432) == arg0) {
            // block_3:
            return arg1 * arg0;
          }
          else {
            // block_4:
            revert(); // error_string, selector=0x8c379a0, string_length=33
          }
        }
    }

    function private__0x198d_0x198d(uint256 arg0, uint256 arg1) internal {
        // block_0:
        require(arg0 & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_1:
    }

    function private__0x1c89_0x1c89(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(96);
    }

    function private__0x1cea_0x1cea(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(1);
    }

    function private__0x1d05_0x1d05(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + arg1;
    }

    function private__0x1dc2_0x1dc2(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 / arg1;
    }

    function private__0x6b3_0x6b3(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        private__0xbe2_0xbe2(arg0, arg1, uint256(uint160(msg.sender)), 231568);
        return 1;
    }

    function private__0xbe2_0xbe2(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
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
}
