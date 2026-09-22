contract Decompiled {
    event Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0x8C5BE1E5EBEC7D5BD14F71427D1E84F3DD0314C0F7B2291E5B200AC8C7C3B925(uint256 indexed arg0, uint256 indexed arg1);

    uint256 public slot_0;
    uint256[] public slot_1;
    mapping(uint256 => uint256) public slot_10;
    mapping(uint256 => uint256) public slot_11;
    uint256 public slot_12;
    uint256[] public slot_2;
    uint256 public slot_3;
    uint256 public slot_4;
    uint256 public slot_5;
    uint256 public slot_6;
    mapping(uint256 => uint256) public slot_7;
    mapping(uint256 => uint256) public slot_8;
    mapping(uint256 => uint256) public slot_9;

    function public_0x0e14ebdc() public returns (uint256 ret0) {
        // block_0:
        return 200;
    }

    function public_0xcfa71593() public returns (uint256 ret0) {
        // block_0:
        return 200;
    }

    function public_0xd201b01e() public {
        // block_0:
        require(msg.data.length > 35);
        // block_1:
        require(false /* TODO: evm.eq.i.i */);
        // block_4:
    }

    function public_0xeeeeeeee() public {
        // block_0:
    }

    function addLiquidity() public {
        // block_0:
    }

    function allowance(address arg0, address arg1) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 67);
        // block_1:
        require(uint256(uint160(arg0)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_3:
        require(uint256(uint160(arg1)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_6:
        return slot_8[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0))];
    }

    function approve(address arg0, uint256 arg1) public returns (bool ret0) {
        // block_0:
        require(msg.data.length > 67);
        // block_1:
        require(uint256(uint160(arg0)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_4:
        return private__0x59f_0x59f(arg1, uint256(uint160(arg0)), 566940) != 0;
    }

    function automatedMarketMakerPairs(address arg0) public returns (bool ret0) {
        // block_0:
        require(msg.data.length > 35);
        // block_1:
        require(uint256(uint160(arg0)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_4:
        return slot_11[uint256(uint160(arg0))] != 0;
    }

    function balanceOf(address arg0) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 35);
        // block_1:
        require(uint256(uint160(arg0)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_4:
        return slot_7[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0))];
    }

    function buyTotalFees() public returns (uint256 ret0) {
        // block_0:
        return 50;
    }

    function decimals() public returns (uint256 ret0) {
        // block_0:
        return 18;
    }

    function launchBlock() public returns (uint256 ret0) {
        // block_0:
        return slot_5;
    }

    function limitsInEffect() public returns (bool ret0) {
        // block_0:
        return slot_4 / uint256(256) & uint256(255) != 0;
    }

    function manualSwap(uint256 arg0) public {
        // block_0:
        uint256 temp_0;
        require(msg.data.length > 35);
        if (msg.data.length > 35) {
          // goto block_1
        }
        // goto block_19
        // block_1:
        if (!((uint256(1) << uint256(160)) + uint256(type(uint256).max) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0 == uint256(uint160(msg.sender)))) {
          // block_17:
          revert(); // error_string, selector=0x8c379a0
        }
        if ((uint256(1) << uint256(160)) + uint256(type(uint256).max) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0 == uint256(uint160(msg.sender))) {
          // goto block_2
        }
        // goto block_17
        // block_2:
        require(arg0 != 0, "Invalid percent.");
        if (!(arg0 == 0)) {
          // goto block_3
        }
        // goto block_16
        // block_3:
        require(arg0 < 101, "Invalid percent.");
        if (arg0 < 101) {
          // goto block_4
        }
        // goto block_15
        // block_4:
        require(!false /* TODO: evm.iszero41.i.not */);
        if (!false /* TODO: evm.iszero41.i.not */) {
          // goto block_5
        }
        // goto block_14
        // block_5:
        if (false /* TODO: evm.iszero46.i.not */) {
          // block_13:
          revert(); // returndata_forward
        }
        if (!false /* TODO: evm.iszero46.i.not */) {
          // goto block_6
        }
        // goto block_13
        // block_6:
        if (address(this).balance == 0) {
          // goto block_18
        }
        // goto block_7
        // block_7:
        temp_0 = private__0x1f99_0x1f99(private__0x1f82_0x1f82(address(this).balance, 75, 2533), 100, 2543);
        if (!false /* TODO: evm.eq61.i */) {
          // block_8:
        }
        // block_9:
        if (false /* TODO: evm.eq76.i */) {
          // goto block_11
        }
        // block_10:
        // block_11:
        if (!false /* TODO: evm.eq92.i */) {
          // block_12:
        }
        // block_18:
        // TODO: recover remaining body
    }

    function maxTransactionAmount() public returns (uint256 ret0) {
        // block_0:
        return slot_1.length;
    }

    function maxWallet() public returns (uint256 ret0) {
        // block_0:
        return slot_2.length;
    }

    function name() public {
        // block_0:
    }

    function openTrading() public {
        // block_0:
        if ((uint256(1) << uint256(160)) + uint256(type(uint256).max) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0 == uint256(uint160(msg.sender))) {
          // block_1:
          if (slot_4 / uint256(65536) & uint256(255) == 0) {
            // block_2:
            slot_5 = block.number;
            slot_4 = slot_4 & uint256((type(uint256).max - 16711680)) | uint256(65536);
          }
          else {
            // block_3:
            revert(); // error_string, selector=0x8c379a0, string_length=24
          }
        }
        else {
          // block_4:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function owner() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0 & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function removeLimits() public {
        // block_0:
        if ((uint256(1) << uint256(160)) + uint256(type(uint256).max) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0 == uint256(uint160(msg.sender))) {
          // block_1:
          slot_4 = slot_4 & uint256((type(uint256).max - 65280));
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function renounceOwnership() public {
        // block_0:
        if ((uint256(1) << uint256(160)) + uint256(type(uint256).max) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0 == uint256(uint160(msg.sender))) {
          // block_1:
          slot_0 = slot_0 & uint256(0) - (uint256(1) << uint256(160));
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function sellTotalFees() public returns (uint256 ret0) {
        // block_0:
        return 50;
    }

    function setAutomatedMarketMakerPair(address arg0, bool arg1) public {
        // block_0:
        require(msg.data.length > 67);
        // block_1:
        require(uint256(uint160(arg0)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_4:
        if ((uint256(1) << uint256(160)) + uint256(type(uint256).max) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0 == uint256(uint160(msg.sender))) {
          // block_5:
          if ((uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0)) == (uint256(1) << uint256(160)) + uint256(1461501637330902918203684832716283019655932542975) & uint256(844096705618839578963235662334210453947556143065)) {
            // block_7:
            revert(); // error_string, selector=0x8c379a0, string_length=26
          }
          else {
            // block_6:
          }
        }
        else {
          // block_8:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function swapTokensAtAmount() public returns (uint256 ret0) {
        // block_0:
        return slot_3;
    }

    function symbol() public {
        // block_0:
    }

    function totalSupply() public returns (uint256 ret0) {
        // block_0:
        return 1000000000000000000000000;
    }

    function transferFrom(address arg0, address arg1, uint256 arg2) public returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        if (msg.data.length > 99) {
          // goto block_1
        }
        // goto block_100
        // block_1:
        require(uint256(uint160(arg0)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_3:
        require(uint256(uint160(arg1)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_5:
        if (slot_8[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == type(uint256).max) {
          // goto block_8
        }
        // block_6:
        if (slot_8[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] < arg2) {
          // block_99:
          revert(); // error_string, selector=0x8c379a0, string_length=40
        }
        if (!(slot_8[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] < arg2)) {
          // goto block_7
        }
        // goto block_99
        // block_7:
        private__0xe85_0xe85(slot_8[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] - arg2, uint256(uint160(msg.sender)), uint256(uint160(arg0)), 1619);
        // block_8:
        if (!(uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
          // goto block_9
        }
        // goto block_97
        // block_9:
        if (!(uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
          // goto block_10
        }
        // goto block_96
        // block_10:
        if (!(arg2 == 0)) {
          // goto block_11
        }
        // goto block_95
        // block_11:
        if (!(slot_4 / uint256(65536) & uint256(255) == 0)) {
          // goto block_15
        }
        // block_12:
        if ((slot_0 ^ uint256(uint160(arg0))) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // goto block_15
        }
        // block_13:
        if (uint256(uint160(address(this))) == uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)) {
          // goto block_15
        }
        // block_14:
        if ((slot_0 ^ uint256(uint160(arg1))) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // goto block_15
        }
        // goto block_94
        // block_15:
        if (!(slot_4 / uint256(256) & uint256(255) == 0)) {
          // goto block_16
        }
        // goto block_32
        // block_16:
        if (!((slot_0 ^ uint256(uint160(arg0))) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
          // goto block_17
        }
        // goto block_32
        // block_17:
        if (!((slot_0 ^ uint256(uint160(arg1))) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
          // goto block_18
        }
        // goto block_32
        // block_18:
        if (!(uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
          // goto block_19
        }
        // goto block_32
        // block_19:
        if (!(uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 57005)) {
          // goto block_20
        }
        // goto block_32
        // block_20:
        if (slot_4 == 0) {
          // goto block_21
        }
        // goto block_32
        // block_21:
        if (slot_11[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_23
        }
        // block_22:
        if (!(slot_10[uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_23
        }
        // goto block_30
        // block_23:
        if (slot_11[uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_25
        }
        // block_24:
        if (!(slot_10[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_25
        }
        // goto block_28
        // block_25:
        if (slot_10[uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_26
        }
        // goto block_32
        // block_26:
        if (private__0x207f_0x207f(arg2, slot_7[uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)], 5082) > slot_2.length) {
          // goto block_27
        }
        // goto block_32
        // block_27:
        require(false, "Max wallet exceeded");
        // block_28:
        if (arg2 > slot_1.length) {
          // goto block_29
        }
        // goto block_32
        // block_29:
        revert(); // error_string, selector=0x8c379a0, string_length=38
        // block_30:
        if (arg2 > slot_1.length) {
          // block_93:
          revert(); // error_string, selector=0x8c379a0, string_length=37
        }
        if (!(arg2 > slot_1.length)) {
          // goto block_31
        }
        // goto block_93
        // block_31:
        if (private__0x207f_0x207f(arg2, slot_7[uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)], 4776) > slot_2.length) {
          // goto block_92
        }
        // block_32:
        if (slot_9[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // block_33:
          if (!(slot_9[uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
            // goto block_34
          }
          // goto block_38
        }
        else {
          // block_34:
          if (!(uint256(uint160(address(this))) == uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max))) {
            // block_35:
            if (!(uint256(uint160(address(this))) == uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max))) {
              // block_36:
              if (!((slot_0 ^ uint256(uint160(arg0))) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
                // block_37:
                slot_12 = block.timestamp;
              }
            }
          }
        }
        // block_38:
        if (slot_9[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_40
        }
        // block_39:
        if (!(block.number > private__0x207f_0x207f(70, slot_5, 5336))) {
          // goto block_40
        }
        // goto block_91
        // block_40:
        if (!(slot_9[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_55
        }
        // block_41:
        if (!(slot_9[uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_55
        }
        // block_42:
        if (slot_11[uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_43
        }
        // goto block_46
        // block_43:
        if (slot_11[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_44
        }
        // goto block_47
        // block_44:
        if (false /* TODO: evm.iszero308.i */) {
          // goto block_46
        }
        // block_45:
        if (false /* TODO: evm.lt.i */) {
          // goto block_46
        }
        // goto block_55
        // block_46:
        // goto block_55
        // block_47:
        if (!(slot_6 > 10)) {
          // block_48:
          slot_6 = private__0x207f_0x207f(1, slot_6, 5670);
        }
        // block_49:
        if (!false /* TODO: evm.branch.cond328.i.not */) {
          // goto block_55
        }
        // block_50:
        if (slot_6 < 11) {
          // block_51:
        }
        else {
          // block_52:
        }
        // goto block_46
        // block_55:
        if (!(slot_7[uint256(uint160(address(this)))] < slot_3)) {
          // goto block_56
        }
        // goto block_74
        // block_56:
        if (slot_4 == 0) {
          // goto block_57
        }
        // goto block_74
        // block_57:
        if (slot_11[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_58
        }
        // goto block_74
        // block_58:
        if (slot_9[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_59
        }
        // goto block_74
        // block_59:
        if (!(slot_9[uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_74
        }
        // block_60:
        if (slot_7[uint256(uint160(address(this)))] > private__0x1f82_0x1f82(20, slot_3, 6978)) {
          // block_61:
        }
        // block_62:
        if (!false /* TODO: evm.iszero31.i.i.not */) {
          // goto block_63
        }
        // goto block_72
        // block_63:
        if (!false /* TODO: evm.iszero36.i.i.not */) {
          // goto block_64
        }
        // goto block_71
        // block_64:
        if (address(this).balance == 0) {
          // goto block_73
        }
        // block_65:
        temp_0 = private__0x1f99_0x1f99(private__0x1f82_0x1f82(address(this).balance, 75, 7294), 100, 7304);
        if (!false /* TODO: evm.eq.i.i */) {
          // block_66:
        }
        // block_67:
        if (!false /* TODO: evm.eq65.i.i */) {
          // goto block_68
        }
        // goto block_69
        // block_68:
        // block_69:
        if (!false /* TODO: evm.eq81.i.i */) {
          // block_70:
        }
        // goto block_73
        // block_71:
        revert(); // returndata_forward
        // block_72:
        revert(); // empty
        // block_73:
        slot_4 = slot_4 & uint256((type(uint256).max - 255));
        // block_74:
        if (slot_9[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // block_75:
        }
        // block_76:
        if (!(slot_7[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] < arg2)) {
          // goto block_77
        }
        // goto block_90
        // block_77:
        if (!false /* TODO: _0x179c_0x0.i.reload */) {
          // goto block_78
        }
        // goto block_89
        // block_78:
        if (block.number < private__0x207f_0x207f(10, slot_5, 5801)) {
          // goto block_83
        }
        // block_79:
        if (slot_11[uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // block_80:
          if (slot_11[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
            // goto block_87
          }
          // goto block_81
        }
        // block_81:
        // goto block_87
        // block_83:
        if (!(slot_11[uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_81
        }
        // block_84:
        if (!(slot_11[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_81
        }
        // block_87:
        if (!false /* TODO: evm.iszero498.i */) {
          // block_88:
          emit Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF(uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max), uint256(uint160(address(this)))); // TODO: recover event signature
        }
        // block_89:
        emit Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF(uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max), uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)); // TODO: recover event signature
        // goto block_98
        // block_90:
        revert(); // error_string, selector=0x8c379a0, string_length=38
        // block_91:
        emit Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF(uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max), uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)); // TODO: recover event signature
        // block_98:
        return 1;
        // block_92:
        require(false, "Max wallet exceeded");
        // block_94:
        require(false, "Trading not enabled");
        // block_95:
        revert(); // error_string, selector=0x8c379a0, string_length=41
        // block_96:
        revert(); // error_string, selector=0x8c379a0, string_length=35
        // block_97:
        revert(); // error_string, selector=0x8c379a0, string_length=37
        // block_100:
        revert(); // empty
    }

    function transfer(address arg0, uint256 arg1) public returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        if (msg.data.length > 67) {
          // goto block_1
        }
        // goto block_3
        // block_1:
        if (!(uint256(uint160(arg0)) & uint256(0) - (uint256(1) << uint256(160)) == 0)) {
          // goto block_2
        }
        // goto block_4
        // block_2:
        revert(); // empty
        // block_3:
        revert(); // empty
        // block_4:
        if (!(uint256(uint160(msg.sender)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
          // goto block_5
        }
        // goto block_93
        // block_5:
        if (!(uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
          // goto block_6
        }
        // goto block_92
        // block_6:
        if (!(arg1 == 0)) {
          // goto block_7
        }
        // goto block_91
        // block_7:
        if (!(slot_4 / uint256(65536) & uint256(255) == 0)) {
          // goto block_11
        }
        // block_8:
        if ((slot_0 ^ uint256(uint160(msg.sender))) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // goto block_11
        }
        // block_9:
        if (uint256(uint160(address(this))) == uint256(uint160(msg.sender)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)) {
          // goto block_11
        }
        // block_10:
        if ((slot_0 ^ uint256(uint160(arg0))) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // goto block_11
        }
        // goto block_90
        // block_11:
        if (!(slot_4 / uint256(256) & uint256(255) == 0)) {
          // goto block_12
        }
        // goto block_28
        // block_12:
        if (!((slot_0 ^ uint256(uint160(msg.sender))) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
          // goto block_13
        }
        // goto block_28
        // block_13:
        if (!((slot_0 ^ uint256(uint160(arg0))) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
          // goto block_14
        }
        // goto block_28
        // block_14:
        if (!(uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
          // goto block_15
        }
        // goto block_28
        // block_15:
        if (!(uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 57005)) {
          // goto block_16
        }
        // goto block_28
        // block_16:
        if (slot_4 == 0) {
          // goto block_17
        }
        // goto block_28
        // block_17:
        if (slot_11[uint256(uint160(msg.sender)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_19
        }
        // block_18:
        if (!(slot_10[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_19
        }
        // goto block_26
        // block_19:
        if (slot_11[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_21
        }
        // block_20:
        if (!(slot_10[uint256(uint160(msg.sender)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_21
        }
        // goto block_24
        // block_21:
        if (slot_10[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_22
        }
        // goto block_28
        // block_22:
        if (private__0x207f_0x207f(arg1, slot_7[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)], 5082) > slot_2.length) {
          // goto block_23
        }
        // goto block_28
        // block_23:
        require(false, "Max wallet exceeded");
        // block_24:
        if (arg1 > slot_1.length) {
          // goto block_25
        }
        // goto block_28
        // block_25:
        revert(); // error_string, selector=0x8c379a0, string_length=38
        // block_26:
        if (arg1 > slot_1.length) {
          // block_89:
          revert(); // error_string, selector=0x8c379a0, string_length=37
        }
        if (!(arg1 > slot_1.length)) {
          // goto block_27
        }
        // goto block_89
        // block_27:
        if (private__0x207f_0x207f(arg1, slot_7[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)], 4776) > slot_2.length) {
          // goto block_88
        }
        // goto block_28
        // block_28:
        if (slot_9[uint256(uint160(msg.sender)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // block_29:
          if (!(slot_9[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
            // goto block_30
          }
          // goto block_34
        }
        else {
          // block_30:
          if (!(uint256(uint160(address(this))) == uint256(uint160(msg.sender)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max))) {
            // block_31:
            if (!(uint256(uint160(address(this))) == uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max))) {
              // block_32:
              if (!((slot_0 ^ uint256(uint160(msg.sender))) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
                // block_33:
                slot_12 = block.timestamp;
              }
            }
          }
        }
        // block_34:
        if (slot_9[uint256(uint160(msg.sender)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_36
        }
        // block_35:
        if (!(block.number > private__0x207f_0x207f(70, slot_5, 5336))) {
          // goto block_36
        }
        // goto block_87
        // block_36:
        if (!(slot_9[uint256(uint160(msg.sender)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_51
        }
        // block_37:
        if (!(slot_9[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_51
        }
        // block_38:
        if (slot_11[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_39
        }
        // goto block_42
        // block_39:
        if (slot_11[uint256(uint160(msg.sender)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_40
        }
        // goto block_43
        // block_40:
        if (false /* TODO: evm.iszero308.i.i */) {
          // goto block_42
        }
        // block_41:
        if (false /* TODO: evm.lt.i.i */) {
          // goto block_42
        }
        // goto block_51
        // block_42:
        // goto block_51
        // block_43:
        if (!(slot_6 > 10)) {
          // block_44:
          slot_6 = private__0x207f_0x207f(1, slot_6, 5670);
        }
        // block_45:
        if (!false /* TODO: evm.branch.cond328.i.i.not */) {
          // goto block_51
        }
        // block_46:
        if (slot_6 < 11) {
          // block_47:
        }
        else {
          // block_48:
        }
        // goto block_42
        // block_51:
        if (!(slot_7[uint256(uint160(address(this)))] < slot_3)) {
          // goto block_52
        }
        // goto block_70
        // block_52:
        if (slot_4 == 0) {
          // goto block_53
        }
        // goto block_70
        // block_53:
        if (slot_11[uint256(uint160(msg.sender)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_54
        }
        // goto block_70
        // block_54:
        if (slot_9[uint256(uint160(msg.sender)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_55
        }
        // goto block_70
        // block_55:
        if (slot_9[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_56
        }
        else {
          // goto block_70
        }
        // block_56:
        if (slot_7[uint256(uint160(address(this)))] > private__0x1f82_0x1f82(20, slot_3, 6978)) {
          // block_57:
        }
        // block_58:
        if (!false /* TODO: evm.iszero31.i.i.i.not */) {
          // goto block_59
        }
        // goto block_68
        // block_59:
        if (!false /* TODO: evm.iszero36.i.i.i.not */) {
          // goto block_60
        }
        // goto block_67
        // block_60:
        if (address(this).balance == 0) {
          // goto block_69
        }
        // goto block_61
        // block_61:
        temp_0 = private__0x1f99_0x1f99(private__0x1f82_0x1f82(address(this).balance, 75, 7294), 100, 7304);
        if (!false /* TODO: evm.eq.i.i.i */) {
          // block_62:
        }
        // block_63:
        if (!false /* TODO: evm.eq65.i.i.i */) {
          // goto block_64
        }
        // goto block_65
        // block_64:
        // block_65:
        if (!false /* TODO: evm.eq81.i.i.i */) {
          // block_66:
        }
        // goto block_69
        // block_67:
        revert(); // returndata_forward
        // block_68:
        revert(); // empty
        // block_69:
        slot_4 = slot_4 & uint256((type(uint256).max - 255));
        // goto block_70
        // block_70:
        if (slot_9[uint256(uint160(msg.sender)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // block_71:
        }
        // block_72:
        if (!(slot_7[uint256(uint160(msg.sender)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] < arg1)) {
          // goto block_73
        }
        // goto block_86
        // block_73:
        if (!false /* TODO: _0x179c_0x0.i.i.reload */) {
          // goto block_74
        }
        // goto block_85
        // block_74:
        if (block.number < private__0x207f_0x207f(10, slot_5, 5801)) {
          // goto block_79
        }
        // block_75:
        if (slot_11[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // block_76:
          if (slot_11[uint256(uint160(msg.sender)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
            // goto block_83
          }
          // goto block_77
        }
        // block_77:
        // goto block_83
        // block_79:
        if (!(slot_11[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_77
        }
        // block_80:
        if (!(slot_11[uint256(uint160(msg.sender)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_77
        }
        // block_83:
        if (!false /* TODO: evm.iszero498.i.i */) {
          // block_84:
          emit Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF(uint256(uint160(msg.sender)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max), uint256(uint160(address(this)))); // TODO: recover event signature
        }
        // block_85:
        emit Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF(uint256(uint160(msg.sender)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max), uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)); // TODO: recover event signature
        // goto block_94
        // block_86:
        revert(); // error_string, selector=0x8c379a0, string_length=38
        // block_87:
        emit Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF(uint256(uint160(msg.sender)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max), uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)); // TODO: recover event signature
        // block_94:
        return 1;
        // block_88:
        require(false, "Max wallet exceeded");
        // block_90:
        require(false, "Trading not enabled");
        // block_91:
        revert(); // error_string, selector=0x8c379a0, string_length=41
        // block_92:
        revert(); // error_string, selector=0x8c379a0, string_length=35
        // block_93:
        revert(); // error_string, selector=0x8c379a0, string_length=37
    }

    function uniswapV2Pair() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(1461501637330902918203684832716283019655932542975) & uint256(844096705618839578963235662334210453947556143065);
    }

    function uniswapV2Router() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(730750818665451459101842416358141509827966271487) & uint256(697323163401596485410334513241460920685086001293);
    }

    function withdrawStuckToken(address arg0, address arg1) public {
        // block_0:
        require(msg.data.length > 67);
        // block_1:
        require(uint256(uint160(arg0)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_3:
        require(uint256(uint160(arg1)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_6:
    }

    function private__0x1f39_0x1f39(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(96);
    }

    function private__0x1eac_0x1eac(uint256 arg0, uint256 arg1) internal {
        // block_0:
        require(arg0 < 2);
        // block_1:
    }

    function private__0x1f82_0x1f82(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg1 * arg0;
    }

    function private__0x1f99_0x1f99(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 / arg1;
    }

    function private__0x207f_0x207f(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg1 + arg0;
    }

    function private__0x2092_0x2092(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 - arg1;
    }

    function private__0x59f_0x59f(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        private__0xe85_0xe85(arg0, arg1, uint256(uint160(msg.sender)), 567964);
        return 1;
    }

    function private__0xe85_0xe85(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
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
