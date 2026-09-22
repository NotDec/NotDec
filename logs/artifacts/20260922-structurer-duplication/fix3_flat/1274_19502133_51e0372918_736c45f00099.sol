contract Decompiled {
    event Event_0x8C5BE1E5EBEC7D5BD14F71427D1E84F3DD0314C0F7B2291E5B200AC8C7C3B925(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0x8BE0079C531659141344CD1FD0A4F28419497F9722A3DAAFE3B4186F6B6457E0(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0x17BBFB9A6069321B6DED73BD96327C9E6B7212A5CD51FF219CD61370ACAFB561();
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
    uint256 public slot_23;
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
        require(msg.data.length > 67);
        // block_2:
        private__0x2649_0x2649(uint256(uint160(arg0)), 9063);
        private__0x2649_0x2649(uint256(uint160(arg1)), 9079);
        return slot_3[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0))];
    }

    function approve(address arg0, uint256 arg1) public returns (bool ret0) {
        // block_0:
        require(msg.data.length > 67);
        // block_2:
        private__0x2649_0x2649(uint256(uint160(arg0)), 9183);
        return private__0x88c_0x88c(arg1, uint256(uint160(arg0)), 272261) != 0;
    }

    function balanceOf(address arg0) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        private__0x2649_0x2649(uint256(uint160(arg0)), 576657);
        return 0 /* TODO: unresolved value: private__0xf19_0xf19 */;
    }

    function decimals() public returns (uint256 ret0) {
        // block_0:
        return slot_12;
    }

    function decreaseAllowance(address arg0, uint256 arg1) public returns (bool ret0) {
        // block_0:
        require(msg.data.length > 67);
        // block_2:
        private__0x2649_0x2649(uint256(uint160(arg0)), 9183);
        return private__0x10fc_0x10fc(arg1, uint256(uint160(arg0)), 273648) != 0;
    }

    function deliver(uint256 arg0) public {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        if (slot_5[uint256(uint160(msg.sender))] == 0) {
          // block_3:
          slot_9 = private__0x162b_0x162b(arg0, slot_9, 3297);
        }
        else {
          // block_4:
          revert(); // error_string, selector=0x8c379a0, string_length=44
        }
    }

    function enableAntiBot() public returns (bool ret0) {
        // block_0:
        return slot_23 / (uint256(1) << uint256(160)) & uint256(255) != 0;
    }

    function excludeFromFee(address arg0) public {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        private__0x2649_0x2649(uint256(uint160(arg0)), 576657);
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0) {
          // block_3:
        }
        else {
          // block_4:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function excludeFromReward(address arg0) public {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        private__0x2649_0x2649(uint256(uint160(arg0)), 576657);
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0) {
          // goto block_3
        }
        // goto block_8
        // block_3:
        if (!(slot_5[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_7
        }
        // block_4:
        if (!(slot_1[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // block_5:
        }
        // block_6:
        // block_7:
        revert(); // error_string, selector=0x8c379a0, string_length=27
        // block_8:
        revert(); // error_string, selector=0x8c379a0
    }

    function includeInReward(address arg0) public {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        private__0x2649_0x2649(uint256(uint160(arg0)), 576657);
    }

    function increaseAllowance(address arg0, uint256 arg1) public returns (bool ret0) {
        // block_0:
        require(msg.data.length > 67);
        // block_2:
        private__0x2649_0x2649(uint256(uint160(arg0)), 9183);
        return private__0xbc7_0xbc7(arg1, uint256(uint160(arg0)), 272828) != 0;
    }

    function isExcludedFromFee(address arg0) public returns (bool ret0) {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        private__0x2649_0x2649(uint256(uint160(arg0)), 576657);
        return slot_4[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0))] != 0;
    }

    function isExcludedFromReward(address arg0) public returns (bool ret0) {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        private__0x2649_0x2649(uint256(uint160(arg0)), 576657);
        return slot_5[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0))] != 0;
    }

    function name() public {
        // block_0:
    }

    function owner() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0 & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function pinkAntiBot() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_23 & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function reflectionFromToken(uint256 arg0, bool arg1) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 67);
        // block_1:
        require(false /* TODO: evm.eq.i.i */);
        // block_4:
        return 0 /* TODO: unresolved value: private__0xd37_0xd37 */;
    }

    function renounceOwnership() public {
        // block_0:
        private__0xf78_0xf78(273403);
    }

    function setCharityFeePercent(uint256 arg0) public {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        private__0x1158_0x1158(arg0, 273742);
    }

    function setEnableAntiBot(bool arg0) public {
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

    function setLiquidityFeePercent(uint256 arg0) public {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        private__0x10a8_0x10a8(arg0, 273571);
    }

    function setSwapBackSettings(uint256 arg0) public {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0) {
          // block_3:
          if (arg0 < private__0x1235_0x1235(10000, private__0xfec_0xfec(slot_7, 5, 424690), 4088)) {
            // block_5:
            revert(); // error_string, selector=0x8c379a0, string_length=56
          }
          else {
            // block_4:
            slot_22 = arg0;
            emit Event_0xF7EDD1A72D399EB95C56C07C5A26F00A9096735269C96C75CAA8FC4E15BCD5D2(); // TODO: recover event signature
          }
        }
        else {
          // block_6:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function setTaxFeePercent(uint256 arg0) public {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        private__0x75a_0x75a(arg0, 272144);
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
        require(msg.data.length > 35);
        // block_2:
        return 0 /* TODO: unresolved value: private__0x954_0x954 */;
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
        uint256 temp_0;
        uint256 temp_1;
        uint256 temp_2;
        if (msg.data.length > 99) {
          // goto block_1
        }
        // goto block_33
        // block_1:
        private__0x2649_0x2649(uint256(uint160(arg0)), 9121);
        private__0x2649_0x2649(uint256(uint160(arg1)), 9137);
        if (!(uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
          // goto block_2
        }
        // goto block_32
        // block_2:
        if (!(uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
          // goto block_3
        }
        // goto block_31
        // block_3:
        if (!(arg2 == 0)) {
          // goto block_4
        }
        // goto block_30
        // block_4:
        if (!(slot_23 / (uint256(1) << uint256(160)) & uint256(255) == 0)) {
          // goto block_5
        }
        // block_7:
        if (!false /* TODO: evm.lt.i.not */) {
          // block_8:
          if (slot_21 / (uint256(1) << uint256(160)) & uint256(255) == 0) {
            // block_9:
          }
        }
        // block_10:
        if (!false /* TODO: _0x1555_0x0.i.in.reload */) {
          // goto block_12
        }
        // block_11:
        // block_12:
        if (!false /* TODO: _0x156a_0x0.i.reload */) {
          // goto block_13
        }
        // goto block_25
        // block_13:
        slot_21 = uint256(1) << uint256(160) | slot_21 & ~(uint256(255) << uint256(160));
        temp_0 = private__0x1235_0x1235(2, slot_22, 5914);
        temp_1 = private__0x1692_0x1692(temp_0, slot_22, 5928);
        if (!false /* TODO: evm.iszero.i.i.not */) {
          // goto block_14
        }
        // goto block_23
        // block_14:
        if (!false /* TODO: evm.iszero17.i.i.not */) {
          private__0x1241_0x1241(temp_0, slot_19 & (uint256(1) << uint256(160)) + uint256(type(uint256).max), uint256(uint160(address(this))), 7368);
          while (false /* TODO: evm.lt53.i.i.not */) {
            // block_21:
          }
          // goto block_24
        }
        // block_22:
        revert(); // returndata_forward
        // block_23:
        revert(); // empty
        // block_17:
        require(!false /* TODO: evm.iszero66.i.i.not */);
        // block_18:
        if (false /* TODO: evm.iszero72.i.i.not */) {
          // block_19:
          revert(); // returndata_forward
        }
        // block_24:
        temp_2 = private__0x1692_0x1692(address(this).balance, address(this).balance, 5952);
        emit Event_0x17BBFB9A6069321B6DED73BD96327C9E6B7212A5CD51FF219CD61370ACAFB561(); // TODO: recover event signature
        slot_21 = slot_21 & ~(uint256(255) << uint256(160));
        // block_25:
        if (slot_4[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // block_26:
        }
        // block_27:
        return 1;
        // block_5:
        require(!false /* TODO: evm.iszero22.i.not */);
        // block_6:
        // block_28:
        revert(); // returndata_forward
        if (false /* TODO: evm.iszero27.i.not */) {
          // goto block_28
        }
        else {
          // goto block_7
        }
        // block_30:
        revert(); // error_string, selector=0x8c379a0, string_length=41
        // block_31:
        revert(); // error_string, selector=0x8c379a0, string_length=35
        // block_32:
        revert(); // error_string, selector=0x8c379a0, string_length=37
        // block_33:
        revert(); // empty
    }

    function transferOwnership(address arg0) public {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        private__0x2649_0x2649(uint256(uint160(arg0)), 576657);
        private__0x119d_0x119d(uint256(uint160(arg0)), 273865);
    }

    function transfer(address arg0, uint256 arg1) public returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        uint256 temp_1;
        uint256 temp_2;
        require(msg.data.length > 67);
        // block_2:
        private__0x2649_0x2649(uint256(uint160(arg0)), 9183);
        if (!(uint256(uint160(msg.sender)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
          // goto block_3
        }
        // goto block_33
        // block_3:
        if (!(uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
          // goto block_4
        }
        // goto block_32
        // block_4:
        if (!(arg1 == 0)) {
          // goto block_5
        }
        // goto block_31
        // block_5:
        if (!(slot_23 / (uint256(1) << uint256(160)) & uint256(255) == 0)) {
          // goto block_6
        }
        // block_8:
        if (!false /* TODO: evm.lt.i.i.not */) {
          // block_9:
          if (slot_21 / (uint256(1) << uint256(160)) & uint256(255) == 0) {
            // block_10:
          }
        }
        // block_11:
        if (!false /* TODO: _0x1555_0x0.i.i.in.reload */) {
          // goto block_13
        }
        // block_12:
        // block_13:
        if (!false /* TODO: _0x156a_0x0.i.i.reload */) {
          // goto block_14
        }
        // goto block_26
        // block_14:
        slot_21 = uint256(1) << uint256(160) | slot_21 & ~(uint256(255) << uint256(160));
        temp_0 = private__0x1235_0x1235(2, slot_22, 5914);
        temp_1 = private__0x1692_0x1692(temp_0, slot_22, 5928);
        if (!false /* TODO: evm.iszero.i.i.i.not */) {
          // goto block_15
        }
        // goto block_24
        // block_15:
        if (!false /* TODO: evm.iszero17.i.i.i.not */) {
          private__0x1241_0x1241(temp_0, slot_19 & (uint256(1) << uint256(160)) + uint256(type(uint256).max), uint256(uint160(address(this))), 7368);
          while (false /* TODO: evm.lt53.i.i.i.not */) {
            // block_22:
          }
          // goto block_25
        }
        // block_23:
        revert(); // returndata_forward
        // block_24:
        revert(); // empty
        // block_18:
        require(!false /* TODO: evm.iszero66.i.i.i.not */);
        // block_19:
        if (false /* TODO: evm.iszero72.i.i.i.not */) {
          // block_20:
          revert(); // returndata_forward
        }
        // block_25:
        temp_2 = private__0x1692_0x1692(address(this).balance, address(this).balance, 5952);
        emit Event_0x17BBFB9A6069321B6DED73BD96327C9E6B7212A5CD51FF219CD61370ACAFB561(); // TODO: recover event signature
        slot_21 = slot_21 & ~(uint256(255) << uint256(160));
        // block_26:
        if (slot_4[uint256(uint160(msg.sender)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // block_27:
        }
        // block_28:
        return 1;
        // block_6:
        require(!false /* TODO: evm.iszero22.i.i.not */);
        // block_7:
        // block_29:
        revert(); // returndata_forward
        if (false /* TODO: evm.iszero27.i.i.not */) {
          // goto block_29
        }
        else {
          // goto block_8
        }
        // block_31:
        revert(); // error_string, selector=0x8c379a0, string_length=41
        // block_32:
        revert(); // error_string, selector=0x8c379a0, string_length=35
        // block_33:
        revert(); // error_string, selector=0x8c379a0, string_length=37
    }

    function uniswapV2Pair() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_20 & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function uniswapV2Router() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_19 & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function private__0x10a8_0x10a8(uint256 arg0, uint256 arg1) internal {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0) {
          // block_1:
          slot_15 = arg0;
          require(private__0x256f_0x256f(private__0x256f_0x256f(slot_13, arg0, 424767), slot_17, 1969) < 2501, "Total fee is over 25%");
          // block_2:
        }
        else {
          // block_4:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function private__0x10fc_0x10fc(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return 1;
    }

    function private__0x1158_0x1158(uint256 arg0, uint256 arg1) internal {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0) {
          // block_1:
          slot_17 = arg0;
          require(private__0x256f_0x256f(private__0x256f_0x256f(slot_13, slot_15, 574973), arg0, 1969) < 2501, "Total fee is over 25%");
          // block_2:
        }
        else {
          // block_4:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function private__0x15dc_0x15dc(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal returns (uint256 ret0) {
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

    function private__0x162b_0x162b(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return private__0x256f_0x256f(arg1, arg0, 575205);
    }

    function private__0x119d_0x119d(uint256 arg0, uint256 arg1) internal {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0) {
          // block_1:
          if (arg0 & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
            // block_3:
            revert(); // error_string, selector=0x8c379a0, string_length=38
          }
          else {
            // block_2:
            private__0x169e_0x169e(arg0, 575054);
          }
        }
        else {
          // block_4:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function private__0x1235_0x1235(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg1 / arg0;
    }

    function private__0x1241_0x1241(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
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

    function private__0x1ff5_0x1ff5(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
        // block_0:
        emit Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF((uint256(1) << uint256(160)) + uint256(type(uint256).max) & arg2, (uint256(1) << uint256(160)) + uint256(type(uint256).max) & arg1); // TODO: recover event signature
    }

    function private__0x204f_0x204f(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
        // block_0:
        emit Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF((uint256(1) << uint256(160)) + uint256(type(uint256).max) & arg2, (uint256(1) << uint256(160)) + uint256(type(uint256).max) & arg1); // TODO: recover event signature
    }

    function private__0x1692_0x1692(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return private__0x25c6_0x25c6(arg1, arg0, 575243);
    }

    function private__0x169e_0x169e(uint256 arg0, uint256 arg1) internal {
        // block_0:
        slot_0 = (uint256(1) << uint256(160)) + uint256(type(uint256).max) & arg0 | slot_0 & uint256(0) - (uint256(1) << uint256(160));
        emit Event_0x8BE0079C531659141344CD1FD0A4F28419497F9722A3DAAFE3B4186F6B6457E0(slot_0 & (uint256(1) << uint256(160)) + uint256(type(uint256).max), (uint256(1) << uint256(160)) + uint256(type(uint256).max) & arg0); // TODO: recover event signature
    }

    function private__0x16ee_0x16ee(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg1 * arg0;
    }

    function private__0x1df0_0x1df0(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
        // block_0:
        emit Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF((uint256(1) << uint256(160)) + uint256(type(uint256).max) & arg2, (uint256(1) << uint256(160)) + uint256(type(uint256).max) & arg1); // TODO: recover event signature
    }

    function private__0x1f36_0x1f36(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
        // block_0:
        emit Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF((uint256(1) << uint256(160)) + uint256(type(uint256).max) & arg2, (uint256(1) << uint256(160)) + uint256(type(uint256).max) & arg1); // TODO: recover event signature
    }

    function private__0x21d1_0x21d1(uint256 arg0, uint256 arg1) internal {
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

    function private__0x22d9_0x22d9(uint256 arg0, uint256 arg1, uint256 arg2) internal {
        // block_0:
        slot_8 = private__0x1692_0x1692(arg1, slot_8, 8934);
        slot_9 = private__0x162b_0x162b(arg0, slot_9, 8950);
    }

    function private__0x24ca_0x24ca(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(96);
    }

    function private__0x256f_0x256f(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + arg1;
    }

    function private__0x25c6_0x25c6(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 - arg1;
    }

    function private__0x2618_0x2618(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(1);
    }

    function private__0x2649_0x2649(uint256 arg0, uint256 arg1) internal {
        // block_0:
        require(arg0 & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_1:
    }

    function private__0x75a_0x75a(uint256 arg0, uint256 arg1) internal {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0) {
          // block_1:
          slot_13 = arg0;
          require(private__0x256f_0x256f(private__0x256f_0x256f(arg0, slot_15, 273983), slot_17, 1969) < 2501, "Total fee is over 25%");
          // block_2:
        }
        else {
          // block_4:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function private__0x88c_0x88c(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        private__0x1241_0x1241(arg0, arg1, uint256(uint160(msg.sender)), 274215);
        return 1;
    }

    function private__0xbc7_0xbc7(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        private__0x1241_0x1241(private__0x162b_0x162b(arg0, slot_3[uint256(uint160(msg.sender))], 424383), arg1, uint256(uint160(msg.sender)), 349376);
        return 1;
    }

    function private__0xf78_0xf78(uint256 arg0) internal {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0) {
          // block_1:
          private__0x169e_0x169e(0, 4012);
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function private__0xfec_0xfec(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 * arg1;
    }
}
