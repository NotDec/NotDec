contract Decompiled {
    event Event_0x8BE0079C531659141344CD1FD0A4F28419497F9722A3DAAFE3B4186F6B6457E0(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0x8C5BE1E5EBEC7D5BD14F71427D1E84F3DD0314C0F7B2291E5B200AC8C7C3B925(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF(uint256 indexed arg0, uint256 indexed arg1);

    uint256 public slot_0;
    uint256 public slot_10;
    uint256 public slot_11;
    uint256 public slot_12;
    uint256 public slot_13;
    uint256 public slot_14;
    uint256 public slot_15;
    uint256 public slot_16;
    uint256 public slot_17;
    mapping(uint256 => uint256) public slot_2;
    mapping(uint256 => uint256) public slot_3;
    mapping(uint256 => uint256) public slot_4;
    mapping(uint256 => uint256) public slot_5;
    uint256 public slot_6;
    uint256 public slot_7;
    uint256 public slot_8;
    uint256 public slot_9;

    function public_0x7ab69269() public {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0) {
          // block_1:
          if (slot_17 == 0) {
            // block_2:
          }
          else {
            // block_3:
            revert(); // error_string, selector=0x8c379a0, string_length=26
          }
        }
        else {
          // block_4:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function public_0xeeeeeeee() public {
        // block_0:
    }

    function public_0xef3fc2dd() public {
        // block_0:
        require(msg.data.length > 131);
        // block_1:
        require(false /* TODO: evm.eq.i.i */);
        // block_3:
        require(false /* TODO: evm.eq.i3.i */);
        // block_5:
        require(false /* TODO: evm.eq.i9.i */);
        // block_7:
        require(false /* TODO: evm.eq.i15.i */);
        // block_10:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0) {
          // block_11:
        }
        else {
          // block_12:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function allowance(address arg0, address arg1) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 67);
        // block_2:
        private__0x1541_0x1541(uint256(uint160(arg0)), 5623);
        private__0x1541_0x1541(uint256(uint160(arg1)), 5639);
        return slot_3[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0))];
    }

    function approve(address arg0, uint256 arg1) public returns (bool ret0) {
        // block_0:
        require(msg.data.length > 67);
        // block_2:
        private__0x1541_0x1541(uint256(uint160(arg0)), 5492);
        return private__0x4ba_0x4ba(arg1, uint256(uint160(arg0)), 161880) != 0;
    }

    function balanceOf(address arg0) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        private__0x1541_0x1541(uint256(uint160(arg0)), 349557);
        return slot_2[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0))];
    }

    function buyFee() public returns (uint256 ret0, uint256 ret1) {
        // block_0:
        return (slot_11, slot_11 / uint256(256) & uint256(255));
    }

    function decimals() public returns (uint256 ret0) {
        // block_0:
        return slot_10;
    }

    function decreaseAllowance(address arg0, uint256 arg1) public returns (bool ret0) {
        // block_0:
        require(msg.data.length > 67);
        // block_2:
        private__0x1541_0x1541(uint256(uint160(arg0)), 5492);
        return private__0x680_0x680(arg1, uint256(uint160(arg0)), 162702) != 0;
    }

    function increaseAllowance(address arg0, uint256 arg1) public returns (bool ret0) {
        // block_0:
        require(msg.data.length > 67);
        // block_2:
        private__0x1541_0x1541(uint256(uint160(arg0)), 5492);
        return private__0x537_0x537(arg1, uint256(uint160(arg0)), 162286) != 0;
    }

    function name() public {
        // block_0:
    }

    function owner() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0 & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
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

    function sellFee() public returns (uint256 ret0, uint256 ret1) {
        // block_0:
        return (slot_12, slot_12 / uint256(256) & uint256(255));
    }

    function swapAndLiquifyEnabled() public returns (bool ret0) {
        // block_0:
        return slot_13 / uint256(16777216) & uint256(255) != 0;
    }

    function symbol() public {
        // block_0:
    }

    function totalSupply() public returns (uint256 ret0) {
        // block_0:
        return slot_6;
    }

    function transferFrom(address arg0, address arg1, uint256 arg2) public returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        uint256 temp_1;
        uint256 temp_2;
        uint256 temp_3;
        uint256 temp_4;
        uint256 temp_5;
        if (msg.data.length > 99) {
          // goto block_1
        }
        // goto block_45
        // block_1:
        private__0x1541_0x1541(uint256(uint160(arg0)), 5535);
        private__0x1541_0x1541(uint256(uint160(arg1)), 5551);
        if (!(uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
          // goto block_2
        }
        // goto block_43
        // block_2:
        if (!(uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
          // goto block_3
        }
        // goto block_42
        // block_3:
        if (!(arg2 == 0)) {
          // goto block_4
        }
        // goto block_41
        // block_4:
        if (!((slot_0 ^ uint256(uint160(arg0))) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
          // goto block_5
        }
        // goto block_7
        // block_5:
        if (!((slot_0 ^ uint256(uint160(arg1))) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
          // goto block_6
        }
        // goto block_7
        // block_6:
        if (slot_17 == 0) {
          // goto block_40
        }
        // goto block_7
        // block_7:
        if (!(slot_2[uint256(uint160(address(this)))] < slot_14)) {
          // block_8:
        }
        // block_9:
        if (!false /* TODO: evm.lt34.i.not */) {
          // goto block_10
        }
        // goto block_12
        // block_10:
        if (slot_13 / uint256(65536) & uint256(255) == 0) {
          // block_11:
        }
        // goto block_12
        // block_12:
        if (false /* TODO: _0xa84_0x0.i.in.reload */) {
          // block_13:
        }
        // block_14:
        if (!false /* TODO: _0xa99_0x0.i.reload */) {
          // goto block_15
        }
        // goto block_29
        // block_15:
        slot_13 = slot_13 & uint256((type(uint256).max - 16711680)) | uint256(65536);
        temp_0 = private__0x1738_0x1738(2, private__0x171f_0x171f(private__0x171f_0x171f(private__0x171f_0x171f(slot_11, slot_12, 3395), slot_11 / uint256(256) & uint256(255), 3405), slot_12 / uint256(256) & uint256(255), 3415), 3426);
        temp_1 = private__0x1772_0x1772(private__0x175b_0x175b(slot_15, private__0x171f_0x171f(slot_11, slot_12, 3457), 3470), temp_0, 3480);
        temp_2 = private__0x170c_0x170c(slot_15, temp_1, 3493);
        if (!false /* TODO: evm.iszero.i.i.not */) {
          // goto block_16
        }
        // goto block_23
        // block_16:
        private__0x741_0x741(temp_2, 697323163401596485410334513241460920685086001293, uint256(uint160(address(this))), 4333);
        while (false /* TODO: evm.lt50.i.i.not */) {
          // block_22:
        }
        // block_18:
        if (!false /* TODO: evm.iszero62.i.i.not */) {
          // goto block_19
        }
        // goto block_21
        // block_19:
        if (false /* TODO: evm.iszero68.i.i.not */) {
          // goto block_20
        }
        // goto block_24
        // block_20:
        revert(); // returndata_forward
        // block_21:
        revert(); // empty
        // block_23:
        revert(); // returndata_forward
        // block_24:
        temp_3 = private__0x1772_0x1772(private__0x170c_0x170c(address(this).balance, address(this).balance, 3516), private__0x170c_0x170c(temp_0, private__0x171f_0x171f(slot_11, slot_12, 3543), 3556), 3566);
        temp_4 = private__0x175b_0x175b(temp_3, private__0x171f_0x171f(slot_11, slot_12, 3593), 3606);
        if (!(temp_4 == 0)) {
          // block_25:
        }
        // block_26:
        temp_5 = private__0x175b_0x175b(private__0x175b_0x175b(2, temp_3, 3671), private__0x171f_0x171f(slot_11 / uint256(256) & uint256(255), slot_12 / uint256(256) & uint256(255), 3657), 3681);
        if (temp_5 == 0) {
          // goto block_28
        }
        // block_27:
        if (!false /* TODO: evm.iszero128.i.not */) {
          // goto block_28
        }
        // goto block_39
        // block_28:
        slot_13 = slot_13 & uint256((type(uint256).max - 16711680));
        // goto block_29
        // block_29:
        if (slot_4[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // block_30:
        }
        // block_31:
        if (!false /* TODO: _0xaec_0x0.i.reload */) {
          // goto block_44
        }
        // block_32:
        if (!(slot_5[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_44
        }
        // block_33:
        if (!(slot_5[uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_44
        }
        // goto block_34
        // block_34:
        if (arg2 > slot_14) {
          // block_38:
          revert(); // error_string, selector=0x8c379a0, string_length=40
        }
        // block_35:
        if ((uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg1)) == (uint256(1) << uint256(160)) + uint256(730750818665451459101842416358141509827966271487) & uint256(619965194449022640830676202122629412960427687921)) {
          // goto block_44
        }
        // goto block_36
        // block_36:
        // block_37:
        revert(); // error_string, selector=0x8c379a0, string_length=34
        // block_39:
        revert(); // returndata_forward
        // block_40:
        revert(); // error_string, selector=0x8c379a0, string_length=24
        // block_41:
        revert(); // error_string, selector=0x8c379a0, string_length=41
        // block_42:
        revert(); // error_string, selector=0x8c379a0, string_length=35
        // block_43:
        revert(); // error_string, selector=0x8c379a0, string_length=37
        // block_44:
        return 1;
        // block_45:
        revert(); // empty
    }

    function transfer(address arg0, uint256 arg1) public returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        uint256 temp_1;
        uint256 temp_2;
        uint256 temp_3;
        uint256 temp_4;
        uint256 temp_5;
        require(msg.data.length > 67);
        // block_2:
        private__0x1541_0x1541(uint256(uint160(arg0)), 5492);
        if (!(uint256(uint160(msg.sender)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
          // goto block_3
        }
        // goto block_44
        // block_3:
        if (!(uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
          // goto block_4
        }
        // goto block_43
        // block_4:
        if (!(arg1 == 0)) {
          // goto block_5
        }
        // goto block_42
        // block_5:
        if (!((slot_0 ^ uint256(uint160(msg.sender))) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
          // goto block_6
        }
        // goto block_8
        // block_6:
        if (!((slot_0 ^ uint256(uint160(arg0))) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
          // goto block_7
        }
        // goto block_8
        // block_7:
        if (slot_17 == 0) {
          // goto block_41
        }
        // block_8:
        if (!(slot_2[uint256(uint160(address(this)))] < slot_14)) {
          // block_9:
        }
        // block_10:
        if (!false /* TODO: evm.lt34.i.i.not */) {
          // goto block_11
        }
        // goto block_13
        // block_11:
        if (slot_13 / uint256(65536) & uint256(255) == 0) {
          // block_12:
        }
        // block_13:
        if (false /* TODO: _0xa84_0x0.i.i.in.reload */) {
          // block_14:
        }
        // block_15:
        if (!false /* TODO: _0xa99_0x0.i.i.reload */) {
          // goto block_16
        }
        // goto block_30
        // block_16:
        slot_13 = slot_13 & uint256((type(uint256).max - 16711680)) | uint256(65536);
        temp_0 = private__0x1738_0x1738(2, private__0x171f_0x171f(private__0x171f_0x171f(private__0x171f_0x171f(slot_11, slot_12, 3395), slot_11 / uint256(256) & uint256(255), 3405), slot_12 / uint256(256) & uint256(255), 3415), 3426);
        temp_1 = private__0x1772_0x1772(private__0x175b_0x175b(slot_15, private__0x171f_0x171f(slot_11, slot_12, 3457), 3470), temp_0, 3480);
        temp_2 = private__0x170c_0x170c(slot_15, temp_1, 3493);
        if (!false /* TODO: evm.iszero.i.i.i.not */) {
          // goto block_17
        }
        // goto block_24
        // block_17:
        private__0x741_0x741(temp_2, 697323163401596485410334513241460920685086001293, uint256(uint160(address(this))), 4333);
        while (false /* TODO: evm.lt50.i.i.i.not */) {
          // block_23:
        }
        // block_19:
        if (!false /* TODO: evm.iszero62.i.i.i.not */) {
          // goto block_20
        }
        // goto block_22
        // block_20:
        if (false /* TODO: evm.iszero68.i.i.i.not */) {
          // goto block_21
        }
        // goto block_25
        // block_21:
        revert(); // returndata_forward
        // block_22:
        revert(); // empty
        // block_24:
        revert(); // returndata_forward
        // block_25:
        temp_3 = private__0x1772_0x1772(private__0x170c_0x170c(address(this).balance, address(this).balance, 3516), private__0x170c_0x170c(temp_0, private__0x171f_0x171f(slot_11, slot_12, 3543), 3556), 3566);
        temp_4 = private__0x175b_0x175b(temp_3, private__0x171f_0x171f(slot_11, slot_12, 3593), 3606);
        if (!(temp_4 == 0)) {
          // block_26:
        }
        // block_27:
        temp_5 = private__0x175b_0x175b(private__0x175b_0x175b(2, temp_3, 3671), private__0x171f_0x171f(slot_11 / uint256(256) & uint256(255), slot_12 / uint256(256) & uint256(255), 3657), 3681);
        if (temp_5 == 0) {
          // goto block_29
        }
        // block_28:
        if (!false /* TODO: evm.iszero128.i.i.not */) {
          // goto block_29
        }
        // goto block_40
        // block_29:
        slot_13 = slot_13 & uint256((type(uint256).max - 16711680));
        // block_30:
        if (slot_4[uint256(uint160(msg.sender)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // block_31:
        }
        // block_32:
        if (!false /* TODO: _0xaec_0x0.i.i.reload */) {
          // goto block_45
        }
        // block_33:
        if (!(slot_5[uint256(uint160(msg.sender)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_45
        }
        // block_34:
        if (!(slot_5[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_45
        }
        // block_35:
        if (arg1 > slot_14) {
          // block_39:
          revert(); // error_string, selector=0x8c379a0, string_length=40
        }
        // block_36:
        if ((uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0)) == (uint256(1) << uint256(160)) + uint256(730750818665451459101842416358141509827966271487) & uint256(619965194449022640830676202122629412960427687921)) {
          // goto block_45
        }
        // goto block_37
        // block_37:
        // block_38:
        revert(); // error_string, selector=0x8c379a0, string_length=34
        // block_40:
        revert(); // returndata_forward
        // block_41:
        revert(); // error_string, selector=0x8c379a0, string_length=24
        // block_42:
        revert(); // error_string, selector=0x8c379a0, string_length=41
        // block_43:
        revert(); // error_string, selector=0x8c379a0, string_length=35
        // block_44:
        revert(); // error_string, selector=0x8c379a0, string_length=37
        // block_45:
        return 1;
    }

    function uniswapV2Pair() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(730750818665451459101842416358141509827966271487) & uint256(619965194449022640830676202122629412960427687921);
    }

    function uniswapV2Router() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(730750818665451459101842416358141509827966271487) & uint256(697323163401596485410334513241460920685086001293);
    }

    function private__0x13a9_0x13a9(uint256 arg0, uint256 arg1) internal {
        // block_0:
    }

    function private__0x175b_0x175b(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg1 * arg0;
    }

    function private__0x1541_0x1541(uint256 arg0, uint256 arg1) internal {
        // block_0:
        require(arg0 & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_1:
    }

    function private__0x16b0_0x16b0(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(96);
    }

    function private__0x16f9_0x16f9(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg1 + arg0;
    }

    function private__0x170c_0x170c(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 - arg1;
    }

    function private__0x171f_0x171f(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return (arg1 & uint256(255)) + (arg0 & uint256(255));
    }

    function private__0x1738_0x1738(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return (arg1 & uint256(255)) * (arg0 & uint256(255));
    }

    function private__0x1772_0x1772(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 / arg1;
    }

    function private__0x4ba_0x4ba(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        private__0x741_0x741(arg0, arg1, uint256(uint160(msg.sender)), 163031);
        return 1;
    }

    function private__0x537_0x537(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        private__0x741_0x741(private__0xca5_0xca5(arg0, slot_3[uint256(uint160(msg.sender))], 255891), arg1, uint256(uint160(msg.sender)), 209479);
        return 1;
    }

    function private__0x680_0x680(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return 1;
    }

    function private__0x741_0x741(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
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

    function private__0xc6d_0xc6d(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal returns (uint256 ret0) {
        // block_0:
        if (arg1 > arg2) {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
        else {
          // block_1:
          return private__0x170c_0x170c(arg2, arg1, 349061);
        }
    }

    function private__0xca5_0xca5(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        temp_0 = private__0x16f9_0x16f9(arg1, arg0, 3249);
        if (temp_0 < arg1) {
          // block_2:
          revert(); // error_string, selector=0x8c379a0, string_length=27
        }
        else {
          // block_1:
          return temp_0;
        }
    }
}
