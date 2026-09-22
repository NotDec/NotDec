contract Decompiled {
    event Event_0x8BE0079C531659141344CD1FD0A4F28419497F9722A3DAAFE3B4186F6B6457E0(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0xD851AEB8E2074B285CC12DA5E2FBF79E642E38F62EF8E59590790C157491EE05();
    event Event_0x51CD7CC33235A1C89F708FECEC535BF7CCA0F94ED05216751BEFB052CA83E679();
    event Event_0xE9070D302280CD857033F56893647494C1410643FE239DAABEE29E9292199B3D();
    event Event_0xBD5AA8E04DBF8CD0C0A2CF0D7F15CAB9D94D85AF3F5D347DC8359B9194610F02();
    event Event_0x4CBA14FD4026630E64B03F8C6A0130CA310C15A5376CF7F6735C66880BB7BCEB();
    event Event_0x8C5BE1E5EBEC7D5BD14F71427D1E84F3DD0314C0F7B2291E5B200AC8C7C3B925(uint256 indexed arg0, uint256 indexed arg1);

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
    mapping(uint256 => uint256) public slot_18;
    uint256 public slot_19;
    mapping(uint256 => uint256) public slot_2;
    uint256 public slot_3;
    uint256 public slot_4;
    uint256 public slot_5;
    uint256 public slot_6;
    uint256 public slot_7;
    uint256 public slot_8;
    uint256 public slot_9;

    function TAX() public returns (uint256 ret0) {
        // block_0:
        return slot_9;
    }

    function public_0x16697fc5() public {
        // block_0:
        require(msg.data.length > 67);
        // block_1:
        require(false /* TODO: evm.eq.i */);
        // block_4:
        private__0x1090_0x1090(1659);
    }

    function public_0x32fc4c01() public {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
    }

    function public_0x73bc5a36() public returns (uint256 ret0) {
        // block_0:
        return slot_17;
    }

    function public_0x864b3167() public {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
    }

    function public_0xeeeeeeee() public {
        // block_0:
    }

    function allowance(address arg0, address arg1) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 67);
        // block_2:
        private__0x1904_0x1904(uint256(uint160(arg0)), 6700);
        private__0x1904_0x1904(uint256(uint160(arg1)), 340367);
        return slot_2[uint256(uint160(arg0)) & uint256(1461501637330902918203684832716283019655932542975)];
    }

    function approve(address arg0, uint256 arg1) public returns (bool ret0) {
        // block_0:
        require(msg.data.length > 67);
        // block_2:
        private__0x1904_0x1904(uint256(uint160(arg0)), 6468);
        return private__0x659_0x659(arg1, uint256(uint160(arg0)), 189612) != 0;
    }

    function balanceOf(address arg0) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        private__0x1904_0x1904(uint256(uint160(arg0)), 340329);
        return slot_1[uint256(uint160(arg0)) & uint256(1461501637330902918203684832716283019655932542975)];
    }

    function decimals() public returns (uint256 ret0) {
        // block_0:
        return 18;
    }

    function decreaseAllowance(address arg0, uint256 arg1) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 67);
        // block_2:
        private__0x1904_0x1904(uint256(uint160(arg0)), 6468);
        if (slot_2[uint256(uint160(msg.sender))] < arg1) {
          // block_4:
          revert(); // error_string, selector=0x8c379a0, string_length=37
        }
        else {
          // block_3:
          private__0xedd_0xedd(slot_2[uint256(uint160(msg.sender))] - arg1, uint256(uint160(arg0)), uint256(uint160(msg.sender)), 290375);
          return 1;
        }
    }

    function disableLimits() public {
        // block_0:
        private__0x1090_0x1090(3578);
        if (slot_16 / uint256(374144419156711147060143317175368453031918731001856) & uint256(255) == 0) {
          // block_2:
          revert(); // error_string, selector=0x8c379a0, string_length=22
        }
        else {
          // block_1:
          slot_16 = slot_16 & uint256((type(uint256).max - 95406826884961342500336545879718955523139276405473280));
          slot_15 = slot_7;
          slot_14 = slot_7;
          emit Event_0xE9070D302280CD857033F56893647494C1410643FE239DAABEE29E9292199B3D(); // TODO: recover event signature
        }
    }

    function getIsExcludedFromFees(address arg0) public returns (bool ret0) {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        private__0x1904_0x1904(uint256(uint160(arg0)), 340329);
        return slot_18[uint256(uint160(arg0)) & uint256(1461501637330902918203684832716283019655932542975)] != 0;
    }

    function increaseAllowance(address arg0, uint256 arg1) public returns (bool ret0) {
        // block_0:
        require(msg.data.length > 67);
        // block_2:
        private__0x1904_0x1904(uint256(uint160(arg0)), 6468);
        return private__0x77e_0x77e(arg1, uint256(uint160(arg0)), 189892) != 0;
    }

    function limit() public returns (bool ret0) {
        // block_0:
        return slot_16 / uint256(374144419156711147060143317175368453031918731001856) & uint256(255) != 0;
    }

    function name() public {
        // block_0:
    }

    function openTrading() public {
        // block_0:
        private__0x1090_0x1090(3301);
        emit Event_0x51CD7CC33235A1C89F708FECEC535BF7CCA0F94ED05216751BEFB052CA83E679(); // TODO: recover event signature
    }

    function owner() public returns (uint256 ret0) {
        // block_0:
        return slot_0;
    }

    function pair() public returns (uint256 ret0) {
        // block_0:
        return slot_8;
    }

    function renounceOwnership() public {
        // block_0:
        private__0xb9e_0xb9e(190203);
    }

    function setMaxAmount(uint256 arg0) public {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        private__0x1090_0x1090(2002);
        if (arg0 > private__0x1ad1_0x1ad1(slot_7, 1000, 2018)) {
          // block_3:
          slot_14 = arg0;
          slot_15 = arg0;
        }
        else {
          // block_4:
          revert(); // error_string, selector=0x8c379a0, string_length=28
        }
    }

    function swapBack() public {
        // block_0:
        if (!false /* TODO: evm.iszero.i.not */) {
          // goto block_1
        }
        // goto block_13
        // block_1:
        private__0xedd_0xedd(slot_17, slot_6, uint256(uint160(address(this))), 2469);
        while (false /* TODO: evm.lt45.i.not */) {
          // block_12:
        }
        // block_3:
        require(!false /* TODO: evm.iszero55.i.not */);
        // block_4:
        if (false /* TODO: evm.iszero61.i.not */) {
          // block_10:
          revert(); // returndata_forward
        }
        else {
          // block_5:
          if (address(this).balance == 0) {
            // block_14:
            emit Event_0xD851AEB8E2074B285CC12DA5E2FBF79E642E38F62EF8E59590790C157491EE05(); // TODO: recover event signature
            slot_19 = slot_19 & uint256((type(uint256).max - 255));
          }
          // block_6:
          if (!false /* TODO: evm.eq.i */) {
            // block_7:
          }
          // block_8:
          if (!false /* TODO: evm.branch.cond85.i.not */) {
            // goto block_14
          }
          // goto block_9
          // block_9:
          revert(); // error_string, selector=0x8c379a0, string_length=39
        }
        // block_13:
        revert(); // returndata_forward
    }

    function symbol() public {
        // block_0:
    }

    function totalSupply() public returns (uint256 ret0) {
        // block_0:
        return slot_3;
    }

    function transferFrom(address arg0, address arg1, uint256 arg2) public returns (uint256 ret0) {
        // block_0:
        if (!(msg.data.length > 99)) {
          // goto block_46
        }
        // goto block_1
        // block_1:
        private__0x1904_0x1904(uint256(uint160(arg0)), 6576);
        private__0x1904_0x1904(uint256(uint160(arg1)), 6592);
        private__0x1111_0x1111(arg2, uint256(uint160(msg.sender)), uint256(uint160(arg0)), 1759);
        if (!(slot_18[uint256(uint160(arg0)) & uint256(1461501637330902918203684832716283019655932542975)] == 0)) {
          // block_44:
          private__0x1621_0x1621(arg2, uint256(uint160(arg1)), uint256(uint160(arg0)), 4778);
          // block_45:
          return 1;
        }
        // block_2:
        if (!(slot_18[uint256(uint160(arg1)) & uint256(1461501637330902918203684832716283019655932542975)] == 0)) {
          // goto block_44
        }
        // goto block_3
        // block_3:
        // block_4:
        if ((slot_8 ^ uint256(uint160(arg0))) & uint256(1461501637330902918203684832716283019655932542975) == 0) {
          // goto block_5
        }
        // goto block_44
        // block_5:
        if (!(slot_19 == 0)) {
          // goto block_44
        }
        // block_6:
        if (!(slot_16 == 0)) {
          // goto block_7
        }
        // goto block_43
        // block_7:
        if (!(slot_16 / uint256(374144419156711147060143317175368453031918731001856) & uint256(255) == 0)) {
          // goto block_8
        }
        // goto block_13
        // block_8:
        if (!((slot_8 ^ uint256(uint160(arg0))) & uint256(1461501637330902918203684832716283019655932542975) == 0)) {
          // block_9:
          if (!((slot_8 ^ uint256(uint160(arg1))) & uint256(1461501637330902918203684832716283019655932542975) == 0)) {
            // goto block_11
          }
          // goto block_10
        }
        // goto block_41
        // block_13:
        if (!((uint256(uint160(arg1)) ^ slot_8) & uint256(1461501637330902918203684832716283019655932542975) == 0)) {
          // goto block_33
        }
        // goto block_14
        // block_14:
        slot_13 = private__0x1abe_0x1abe(slot_13, 1, 5300);
        if (slot_13 > private__0x1ad1_0x1ad1(slot_11, 2, 5323)) {
          // block_15:
        }
        else {
          // block_16:
        }
        // block_17:
        if (slot_17 > slot_1[uint256(uint160(address(this)))]) {
          // goto block_33
        }
        // block_18:
        if (!false /* TODO: evm.iszero.i.i.not */) {
          // goto block_19
        }
        // goto block_31
        // block_19:
        private__0xedd_0xedd(slot_17, slot_6, uint256(uint160(address(this))), 2469);
        while (false /* TODO: evm.lt45.i.i.not */) {
          // block_30:
        }
        // block_21:
        if (!false /* TODO: evm.iszero55.i.i.not */) {
          // goto block_22
        }
        // goto block_29
        // block_22:
        if (!false /* TODO: evm.iszero61.i.i.not */) {
          // goto block_23
        }
        // goto block_28
        // block_23:
        if (address(this).balance == 0) {
          // goto block_32
        }
        // goto block_24
        // block_24:
        if (!false /* TODO: evm.eq.i.i */) {
          // block_25:
        }
        // block_26:
        if (!false /* TODO: evm.branch.cond85.i.i.not */) {
          // goto block_32
        }
        // block_27:
        revert(); // error_string, selector=0x8c379a0, string_length=39
        // block_28:
        revert(); // returndata_forward
        // block_29:
        revert(); // empty
        // block_31:
        revert(); // returndata_forward
        // block_32:
        emit Event_0xD851AEB8E2074B285CC12DA5E2FBF79E642E38F62EF8E59590790C157491EE05(); // TODO: recover event signature
        slot_19 = slot_19 & uint256((type(uint256).max - 255));
        // block_33:
        if ((uint256(uint160(arg0)) ^ slot_8) & uint256(1461501637330902918203684832716283019655932542975) == 0) {
          // block_34:
          slot_12 = private__0x1abe_0x1abe(slot_12, 1, 5452);
          if (slot_12 > slot_11) {
            // block_35:
          }
          else {
            // block_36:
          }
          // block_37:
        }
        // goto block_38
        // block_38:
        if (!false /* TODO: evm.iszero110.i */) {
          // block_39:
        }
        // block_40:
        // goto block_45
        // block_10:
        if (arg2 > slot_14) {
          // block_42:
          revert(); // custom_error_candidate, selector=0x801bc44b, args=0
        }
        // block_11:
        if ((slot_8 ^ uint256(uint160(arg1))) & uint256(1461501637330902918203684832716283019655932542975) == 0) {
          // goto block_13
        }
        // goto block_12
        // block_12:
        // block_41:
        revert(); // custom_error_candidate, selector=0xa9a44dff, args=0
        // block_43:
        revert(); // error_string, selector=0x8c379a0, string_length=19
        // block_46:
        revert(); // empty
    }

    function transferOwnership(address arg0) public {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        private__0x1904_0x1904(uint256(uint160(arg0)), 340329);
        private__0xd3b_0xd3b(uint256(uint160(arg0)), 190760);
    }

    function transfer(address arg0, uint256 arg1) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 67);
        // block_2:
        private__0x1904_0x1904(uint256(uint160(arg0)), 6468);
        if (!(slot_18[uint256(uint160(msg.sender)) & uint256(1461501637330902918203684832716283019655932542975)] == 0)) {
          // block_45:
          private__0x1621_0x1621(arg1, uint256(uint160(arg0)), uint256(uint160(msg.sender)), 4778);
          // block_46:
          return 1;
        }
        // block_3:
        if (!(slot_18[uint256(uint160(arg0)) & uint256(1461501637330902918203684832716283019655932542975)] == 0)) {
          // goto block_45
        }
        // goto block_4
        // block_4:
        // block_5:
        if ((slot_8 ^ uint256(uint160(msg.sender))) & uint256(1461501637330902918203684832716283019655932542975) == 0) {
          // goto block_6
        }
        // goto block_45
        // block_6:
        if (!(slot_19 == 0)) {
          // goto block_45
        }
        // block_7:
        if (!(slot_16 == 0)) {
          // goto block_8
        }
        // goto block_44
        // block_8:
        if (!(slot_16 / uint256(374144419156711147060143317175368453031918731001856) & uint256(255) == 0)) {
          // goto block_9
        }
        // goto block_14
        // block_9:
        if (!((slot_8 ^ uint256(uint160(msg.sender))) & uint256(1461501637330902918203684832716283019655932542975) == 0)) {
          // block_10:
          if (!((slot_8 ^ uint256(uint160(arg0))) & uint256(1461501637330902918203684832716283019655932542975) == 0)) {
            // goto block_12
          }
          // goto block_11
        }
        // goto block_42
        // block_14:
        if (!((uint256(uint160(arg0)) ^ slot_8) & uint256(1461501637330902918203684832716283019655932542975) == 0)) {
          // goto block_34
        }
        // block_15:
        slot_13 = private__0x1abe_0x1abe(slot_13, 1, 5300);
        if (slot_13 > private__0x1ad1_0x1ad1(slot_11, 2, 5323)) {
          // block_16:
        }
        else {
          // block_17:
        }
        // block_18:
        if (slot_17 > slot_1[uint256(uint160(address(this)))]) {
          // goto block_34
        }
        // block_19:
        if (!false /* TODO: evm.iszero.i.i.i.not */) {
          // goto block_20
        }
        // goto block_32
        // block_20:
        private__0xedd_0xedd(slot_17, slot_6, uint256(uint160(address(this))), 2469);
        while (false /* TODO: evm.lt45.i.i.i.not */) {
          // block_31:
        }
        // block_22:
        if (!false /* TODO: evm.iszero55.i.i.i.not */) {
          // goto block_23
        }
        // goto block_30
        // block_23:
        if (!false /* TODO: evm.iszero61.i.i.i.not */) {
          // goto block_24
        }
        // goto block_29
        // block_24:
        if (address(this).balance == 0) {
          // goto block_33
        }
        // block_25:
        if (!false /* TODO: evm.eq.i.i.i */) {
          // block_26:
        }
        // block_27:
        if (!false /* TODO: evm.branch.cond85.i.i.i.not */) {
          // goto block_33
        }
        // block_28:
        revert(); // error_string, selector=0x8c379a0, string_length=39
        // block_29:
        revert(); // returndata_forward
        // block_30:
        revert(); // empty
        // block_32:
        revert(); // returndata_forward
        // block_33:
        emit Event_0xD851AEB8E2074B285CC12DA5E2FBF79E642E38F62EF8E59590790C157491EE05(); // TODO: recover event signature
        slot_19 = slot_19 & uint256((type(uint256).max - 255));
        // block_34:
        if ((uint256(uint160(msg.sender)) ^ slot_8) & uint256(1461501637330902918203684832716283019655932542975) == 0) {
          // block_35:
          slot_12 = private__0x1abe_0x1abe(slot_12, 1, 5452);
          if (slot_12 > slot_11) {
            // block_36:
          }
          else {
            // block_37:
          }
          // block_38:
        }
        // block_39:
        if (!false /* TODO: evm.iszero110.i.i */) {
          // block_40:
        }
        // block_41:
        // goto block_46
        // block_11:
        if (arg1 > slot_14) {
          // block_43:
          revert(); // custom_error_candidate, selector=0x801bc44b, args=0
        }
        // block_12:
        if ((slot_8 ^ uint256(uint160(arg0))) & uint256(1461501637330902918203684832716283019655932542975) == 0) {
          // goto block_14
        }
        // goto block_13
        // block_13:
        // block_42:
        revert(); // custom_error_candidate, selector=0xa9a44dff, args=0
        // block_44:
        revert(); // error_string, selector=0x8c379a0, string_length=19
    }

    function treasuryWallet() public returns (uint256 ret0) {
        // block_0:
        return slot_16 / uint256(256) & uint256(1461501637330902918203684832716283019655932542975);
    }

    function private__0x1090_0x1090(uint256 arg0) internal {
        // block_0:
        if (uint256(uint160(msg.sender)) == slot_0) {
          // block_1:
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0, string_length=32
        }
    }

    function private__0x1111_0x1111(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
        // block_0:
        if (!(slot_2[arg2 & uint256(1461501637330902918203684832716283019655932542975)] == type(uint256).max)) {
          // block_2:
          if (slot_2[arg2 & uint256(1461501637330902918203684832716283019655932542975)] < arg0) {
            // block_4:
            revert(); // error_string, selector=0x8c379a0, string_length=29
          }
          // block_3:
          private__0xedd_0xedd(slot_2[arg2 & uint256(1461501637330902918203684832716283019655932542975)] - arg0, arg1, arg2, 340142);
        }
        // block_1:
    }

    function private__0x1bfc_0x1bfc(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 - arg1;
    }

    function private__0x15ac_0x15ac(uint256 arg0, uint256 arg1) internal {
        // block_0:
        emit Event_0x8BE0079C531659141344CD1FD0A4F28419497F9722A3DAAFE3B4186F6B6457E0(slot_0 & uint256(1461501637330902918203684832716283019655932542975), arg0 & uint256(1461501637330902918203684832716283019655932542975)); // TODO: recover event signature
    }

    function private__0x1621_0x1621(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
        // block_0:
        if (arg2 & uint256(1461501637330902918203684832716283019655932542975) == 0) {
          // block_6:
          revert(); // error_string, selector=0x8c379a0, string_length=37
        }
        else {
          // block_1:
          if (arg1 & uint256(1461501637330902918203684832716283019655932542975) == 0) {
            // block_5:
            revert(); // error_string, selector=0x8c379a0, string_length=35
          }
          else {
            // block_2:
            if (slot_1[arg2 & uint256(1461501637330902918203684832716283019655932542975)] < arg0) {
              // block_4:
              revert(); // error_string, selector=0x8c379a0, string_length=38
            }
            else {
              // block_3:
              emit Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF(arg2 & uint256(1461501637330902918203684832716283019655932542975), arg1 & uint256(1461501637330902918203684832716283019655932542975)); // TODO: recover event signature
            }
          }
        }
    }

    function private__0x1904_0x1904(uint256 arg0, uint256 arg1) internal {
        // block_0:
        require(arg0 < 1461501637330902918203684832716283019655932542976);
        // block_1:
    }

    function private__0x1abe_0x1abe(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg1 + arg0;
    }

    function private__0x1ad1_0x1ad1(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 / arg1;
    }

    function private__0x1be5_0x1be5(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg1 * arg0;
    }

    function private__0x659_0x659(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        private__0xedd_0xedd(arg0, arg1, uint256(uint160(msg.sender)), 190983);
        return 1;
    }

    function private__0x6f5_0x6f5(uint256 arg0, uint256 arg1) internal {
        // block_0:
        private__0x1090_0x1090(1789);
        emit Event_0xBD5AA8E04DBF8CD0C0A2CF0D7F15CAB9D94D85AF3F5D347DC8359B9194610F02(); // TODO: recover event signature
    }

    function private__0x77e_0x77e(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        private__0xedd_0xedd(private__0x1abe_0x1abe(slot_2[uint256(uint160(msg.sender))], arg0, 1989), arg1, uint256(uint160(msg.sender)), 240646);
        return 1;
    }

    function private__0xb9e_0xb9e(uint256 arg0) internal {
        // block_0:
        private__0x1090_0x1090(2982);
        private__0x15ac_0x15ac(0, 290267);
    }

    function private__0xbb2_0xbb2(uint256 arg0, uint256 arg1) internal {
        // block_0:
        private__0x1090_0x1090(3002);
        slot_17 = arg0;
        emit Event_0x4CBA14FD4026630E64B03F8C6A0130CA310C15A5376CF7F6735C66880BB7BCEB(); // TODO: recover event signature
    }

    function private__0xd3b_0xd3b(uint256 arg0, uint256 arg1) internal {
        // block_0:
        private__0x1090_0x1090(3395);
        if (arg0 & uint256(1461501637330902918203684832716283019655932542975) == 0) {
          // block_2:
          revert(); // error_string, selector=0x8c379a0, string_length=38
        }
        else {
          // block_1:
          private__0x15ac_0x15ac(arg0, 340038);
        }
    }

    function private__0xedd_0xedd(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
        // block_0:
        if (arg2 & uint256(1461501637330902918203684832716283019655932542975) == 0) {
          // block_4:
          revert(); // error_string, selector=0x8c379a0, string_length=36
        }
        else {
          // block_1:
          if (arg1 & uint256(1461501637330902918203684832716283019655932542975) == 0) {
            // block_3:
            revert(); // error_string, selector=0x8c379a0, string_length=34
          }
          else {
            // block_2:
            emit Event_0x8C5BE1E5EBEC7D5BD14F71427D1E84F3DD0314C0F7B2291E5B200AC8C7C3B925(arg2 & uint256(1461501637330902918203684832716283019655932542975), arg1 & uint256(1461501637330902918203684832716283019655932542975)); // TODO: recover event signature
          }
        }
    }
}
