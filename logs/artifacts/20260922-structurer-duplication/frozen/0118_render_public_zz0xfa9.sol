contract Decompiled {
    event Event_0x8C5BE1E5EBEC7D5BD14F71427D1E84F3DD0314C0F7B2291E5B200AC8C7C3B925(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF(uint256 indexed arg0, uint256 indexed arg1);

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
    }

    function public_0xeeeeeeee() public {
        // block_0:
    }

    function addLiquidity() public {
        // block_0:
    }

    function allowance(address arg0, address arg1) public returns (uint256 ret0) {
        // block_0:
        return slot_8[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0))];
    }

    function approve(address arg0, uint256 arg1) public returns (bool ret0) {
        // block_0:
        return private__0x59f_0x59f(arg1, uint256(uint160(arg0)), 566940) != 0;
    }

    function automatedMarketMakerPairs(address arg0) public returns (bool ret0) {
        // block_0:
        return slot_11[uint256(uint160(arg0))] != 0;
    }

    function balanceOf(address arg0) public returns (uint256 ret0) {
        // block_0:
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
        require(msg.data.length > 35);
        // block_1:
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
        if ((uint256(1) << uint256(160)) + uint256(type(uint256).max) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0 == uint256(uint160(msg.sender))) {
          // block_1:
          if ((uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0)) == (uint256(1) << uint256(160)) + uint256(1461501637330902918203684832716283019655932542975) & uint256(844096705618839578963235662334210453947556143065)) {
            // block_3:
            revert(); // error_string, selector=0x8c379a0, string_length=26
          }
          else {
            // block_2:
          }
        }
        else {
          // block_4:
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
        require(msg.data.length > 99);
        if (msg.data.length > 99) {
          // goto block_1
        }
        // goto block_6
        // block_1:
        if (slot_8[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == type(uint256).max) {
          // block_4:
          return 1;
        }
        if (!(slot_8[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == type(uint256).max)) {
          // goto block_2
        }
        // goto block_4
        // block_2:
        if (slot_8[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] < arg2) {
          // block_5:
          revert(); // error_string, selector=0x8c379a0, string_length=40
        }
        if (!(slot_8[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] < arg2)) {
          // goto block_3
        }
        // goto block_5
        // block_3:
        private__0xe85_0xe85(slot_8[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] - arg2, uint256(uint160(msg.sender)), uint256(uint160(arg0)), 1619);
        // goto block_4
        // TODO: recover remaining body
    }

    function transfer(address arg0, uint256 arg1) public returns (bool ret0) {
        // block_0:
        return private__0x7da_0x7da(arg1, uint256(uint160(arg0)), 567480) != 0;
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
    }

    function zz0xfa9(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) public {
        // block_0:
        if (!(arg2 & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
          // goto block_1
        }
        // goto block_77
        // block_1:
        if (!(arg1 & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
          // goto block_2
        }
        // goto block_76
        // block_2:
        if (!(arg0 == 0)) {
          // goto block_3
        }
        // goto block_75
        // block_3:
        if (!(slot_4 / uint256(65536) & uint256(255) == 0)) {
          // goto block_7
        }
        // block_4:
        if ((slot_0 ^ arg2) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // goto block_7
        }
        // block_5:
        if (uint256(uint160(address(this))) == arg2 & (uint256(1) << uint256(160)) + uint256(type(uint256).max)) {
          // goto block_7
        }
        // block_6:
        if ((slot_0 ^ arg1) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // goto block_7
        }
        // goto block_74
        // block_7:
        if (!(slot_4 / uint256(256) & uint256(255) == 0)) {
          // goto block_8
        }
        // goto block_24
        // block_8:
        if (!((slot_0 ^ arg2) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
          // goto block_9
        }
        // goto block_24
        // block_9:
        if (!((slot_0 ^ arg1) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
          // goto block_10
        }
        // goto block_24
        // block_10:
        if (!(arg1 & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
          // goto block_11
        }
        // goto block_24
        // block_11:
        if (!(arg1 & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 57005)) {
          // goto block_12
        }
        // goto block_24
        // block_12:
        if (slot_4 == 0) {
          // goto block_13
        }
        // goto block_24
        // block_13:
        if (slot_11[arg2 & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_15
        }
        // block_14:
        if (!(slot_10[arg1 & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_15
        }
        // goto block_22
        // block_15:
        if (slot_11[arg1 & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_17
        }
        // block_16:
        if (!(slot_10[arg2 & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_17
        }
        // goto block_20
        // block_17:
        if (slot_10[arg1 & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_18
        }
        // goto block_24
        // block_18:
        if (private__0x207f_0x207f(arg0, slot_7[arg1 & (uint256(1) << uint256(160)) + uint256(type(uint256).max)], 5082) > slot_2.length) {
          // goto block_19
        }
        // goto block_24
        // block_19:
        require(false, "Max wallet exceeded");
        // block_20:
        if (arg0 > slot_1.length) {
          // goto block_21
        }
        // goto block_24
        // block_21:
        revert(); // error_string, selector=0x8c379a0, string_length=38
        // block_22:
        if (arg0 > slot_1.length) {
          // block_73:
          revert(); // error_string, selector=0x8c379a0, string_length=37
        }
        if (!(arg0 > slot_1.length)) {
          // goto block_23
        }
        // goto block_73
        // block_23:
        if (private__0x207f_0x207f(arg0, slot_7[arg1 & (uint256(1) << uint256(160)) + uint256(type(uint256).max)], 4776) > slot_2.length) {
          // goto block_72
        }
        // goto block_24
        // block_24:
        if (slot_9[arg2 & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // block_25:
          if (!(slot_9[arg1 & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
            // goto block_26
          }
          // goto block_30
        }
        else {
          // block_26:
          if (!(uint256(uint160(address(this))) == arg2 & (uint256(1) << uint256(160)) + uint256(type(uint256).max))) {
            // block_27:
            if (!(uint256(uint160(address(this))) == arg1 & (uint256(1) << uint256(160)) + uint256(type(uint256).max))) {
              // block_28:
              if (!((slot_0 ^ arg2) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
                // block_29:
                slot_12 = block.timestamp;
              }
            }
          }
        }
        // block_30:
        if (slot_9[arg2 & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_32
        }
        // block_31:
        if (!(block.number > private__0x207f_0x207f(70, slot_5, 5336))) {
          // goto block_32
        }
        // goto block_71
        // block_32:
        if (slot_9[arg2 & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_33
        }
        // goto block_47
        // block_33:
        if (slot_9[arg1 & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_34
        }
        // goto block_47
        // block_34:
        if (slot_11[arg1 & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_35
        }
        // goto block_38
        // block_35:
        if (slot_11[arg2 & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_36
        }
        // goto block_39
        // block_36:
        if (false /* TODO: evm.iszero308 */) {
          // goto block_38
        }
        // block_37:
        if (false /* TODO: evm.lt */) {
          // goto block_38
        }
        // goto block_47
        // block_38:
        // goto block_47
        // block_39:
        if (!(slot_6 > 10)) {
          // block_40:
          slot_6 = private__0x207f_0x207f(1, slot_6, 5670);
        }
        // block_41:
        if (false /* TODO: evm.branch.cond328.not */) {
          // goto block_42
        }
        // goto block_47
        // block_42:
        if (slot_6 < 11) {
          // block_43:
        }
        else {
          // block_44:
        }
        // goto block_38
        // block_47:
        if (!(slot_7[uint256(uint160(address(this)))] < slot_3)) {
          // block_48:
          if (slot_4 == 0) {
            // block_49:
            if (slot_11[arg2 & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
              // block_50:
              if (slot_9[arg2 & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
                // block_51:
                if (slot_9[arg1 & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
                  // block_52:
                  slot_4 = slot_4 & uint256((type(uint256).max - 255));
                }
              }
            }
          }
        }
        // block_53:
        if (slot_9[arg2 & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_54
        }
        // goto block_55
        // block_54:
        // block_55:
        if (slot_7[arg2 & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] < arg0) {
          // block_70:
          revert(); // error_string, selector=0x8c379a0, string_length=38
        }
        if (!(slot_7[arg2 & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] < arg0)) {
          // goto block_56
        }
        // goto block_70
        // block_56:
        if (false /* TODO: _0x179c_0x0.reload */) {
          // goto block_68
        }
        // block_57:
        if (block.number < private__0x207f_0x207f(10, slot_5, 5801)) {
          // goto block_62
        }
        // block_58:
        if (slot_11[arg1 & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // block_59:
          if (slot_11[arg2 & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
            // goto block_66
          }
          // goto block_60
        }
        // block_60:
        // goto block_66
        // block_62:
        if (!(slot_11[arg1 & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_60
        }
        // block_63:
        if (!(slot_11[arg2 & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_60
        }
        // block_66:
        if (!false /* TODO: evm.iszero498 */) {
          // block_67:
          emit Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF(arg2 & (uint256(1) << uint256(160)) + uint256(type(uint256).max), uint256(uint160(address(this)))); // TODO: recover event signature
        }
        // block_68:
        emit Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF(arg2 & (uint256(1) << uint256(160)) + uint256(type(uint256).max), arg1 & (uint256(1) << uint256(160)) + uint256(type(uint256).max)); // TODO: recover event signature
        // block_78:
        // block_71:
        emit Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF(arg2 & (uint256(1) << uint256(160)) + uint256(type(uint256).max), arg1 & (uint256(1) << uint256(160)) + uint256(type(uint256).max)); // TODO: recover event signature
        // block_69:
        // block_72:
        require(false, "Max wallet exceeded");
        // block_74:
        require(false, "Trading not enabled");
        // block_75:
        revert(); // error_string, selector=0x8c379a0, string_length=41
        // block_76:
        revert(); // error_string, selector=0x8c379a0, string_length=35
        // block_77:
        revert(); // error_string, selector=0x8c379a0, string_length=37
    }

    function private__0x207f_0x207f(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg1 + arg0;
    }

    function private__0x2092_0x2092(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 - arg1;
    }

    function private__0x1eac_0x1eac(uint256 arg0, uint256 arg1) internal {
        // block_0:
        require(arg0 < 2);
        // block_1:
    }

    function private__0x1f39_0x1f39(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(96);
    }

    function private__0x1f82_0x1f82(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg1 * arg0;
    }

    function private__0x1f99_0x1f99(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 / arg1;
    }

    function private__0x59f_0x59f(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        private__0xe85_0xe85(arg0, arg1, uint256(uint160(msg.sender)), 567964);
        return 1;
    }

    function private__0x7da_0x7da(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
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
