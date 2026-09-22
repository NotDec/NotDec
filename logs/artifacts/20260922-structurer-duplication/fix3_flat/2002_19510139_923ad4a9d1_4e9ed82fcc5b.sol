contract Decompiled {
    event Event_0x8C5BE1E5EBEC7D5BD14F71427D1E84F3DD0314C0F7B2291E5B200AC8C7C3B925(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0x8BE0079C531659141344CD1FD0A4F28419497F9722A3DAAFE3B4186F6B6457E0(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0xFFA9187BF1F18BF477BD0EA1BCBB64E93B6A98132473929EDFCE215CD9B16FAB(uint256 indexed arg0, bool indexed arg1);
    event Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0x17BBFB9A6069321B6DED73BD96327C9E6B7212A5CD51FF219CD61370ACAFB561();
    event Event_0x454C91AE84FCC766DDDA0DCB289F26B3D0176EFEACF4061FC219FA6CA8C3048D();
    event Event_0xFAF1B77ED79F6E898C44DD8AB36B330C7B2FD39BCAAB05ED6362480DF8703965(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0xA751787977EEB3902E30E1D19CA00C6AD274A1F622C31A206E32366700B05674(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0x9D8F7706EA1113D1A167B526ECA956215946DD36CC7DF39EB16180222D8B5DF7(uint256 indexed arg0);
    event Event_0x8462566617872A3FBAB94534675218431FF9E204063EE3F4F43D965626A39ABB();

    mapping(uint256 => uint256) public slot_0;
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
    uint256 public slot_2;
    mapping(uint256 => uint256) public slot_20;
    uint256 public slot_21;
    uint256 public slot_22;
    uint256 public slot_23;
    uint256 public slot_24;
    uint256 public slot_25;
    uint256 public slot_26;
    uint256 public slot_27;
    uint256 public slot_28;
    uint256 public slot_29;
    uint256 public slot_3;
    uint256 public slot_30;
    uint256 public slot_31;
    uint256 public slot_32;
    mapping(uint256 => uint256) public slot_33;
    mapping(uint256 => uint256) public slot_34;
    mapping(uint256 => uint256) public slot_35;
    uint256 public slot_4;
    uint256 public slot_5;
    uint256 public slot_6;
    uint256 public slot_7;
    uint256 public slot_8;
    uint256 public slot_9;

    function public_0x535e02d8() public {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_3:
          emit Event_0xFAF1B77ED79F6E898C44DD8AB36B330C7B2FD39BCAAB05ED6362480DF8703965(0 /* TODO: unresolved value: evm.calldataload.i.load */ & (uint256(1) << uint256(160)) + uint256(type(uint256).max), (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_9); // TODO: recover event signature
        }
        else {
          // block_4:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function public_0x7d831a8b() public {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_3:
          emit Event_0xA751787977EEB3902E30E1D19CA00C6AD274A1F622C31A206E32366700B05674(0 /* TODO: unresolved value: evm.calldataload.i.load */ & (uint256(1) << uint256(160)) + uint256(type(uint256).max), (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_8); // TODO: recover event signature
        }
        else {
          // block_4:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function public_0xeeeeeeee() public {
        // block_0:
    }

    function isExcludedMaxTransactionAmount(address arg0) public returns (bool ret0) {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        private__0x2cd4_0x2cd4(uint256(uint160(arg0)), 851310);
        return slot_34[uint256(uint160(arg0))] != 0;
    }

    function allowance(address arg0, address arg1) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 67);
        // block_2:
        private__0x2cd4_0x2cd4(uint256(uint160(arg0)), 11860);
        private__0x2cd4_0x2cd4(uint256(uint160(arg1)), 11876);
        return slot_1[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0))];
    }

    function approve(address arg0, uint256 arg1) public returns (bool ret0) {
        // block_0:
        require(msg.data.length > 67);
        // block_2:
        private__0x2cd4_0x2cd4(uint256(uint160(arg0)), 11524);
        return private__0xb9b_0xb9b(arg1, uint256(uint160(arg0)), 468345) != 0;
    }

    function automatedMarketMakerPairs(address arg0) public returns (bool ret0) {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        private__0x2cd4_0x2cd4(uint256(uint160(arg0)), 851310);
        return slot_35[uint256(uint160(arg0))] != 0;
    }

    function balanceOf(address arg0) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        private__0x2cd4_0x2cd4(uint256(uint160(arg0)), 851310);
        return slot_0[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0))];
    }

    function buyDevelopmentFee() public returns (uint256 ret0) {
        // block_0:
        return slot_25;
    }

    function buyLiquidityFee() public returns (uint256 ret0) {
        // block_0:
        return slot_24;
    }

    function buyMarketingFee() public returns (uint256 ret0) {
        // block_0:
        return slot_23;
    }

    function buyTotalFees() public returns (uint256 ret0) {
        // block_0:
        return slot_22;
    }

    function createPair() public {
        // block_0:
    }

    function deadAddress() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(65535) & uint256(57005);
    }

    function decimals() public returns (uint256 ret0) {
        // block_0:
        return 18;
    }

    function decreaseAllowance(address arg0, uint256 arg1) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 67);
        // block_2:
        private__0x2cd4_0x2cd4(uint256(uint160(arg0)), 11524);
        if (slot_1[uint256(uint160(msg.sender))] < arg1) {
          // block_4:
          revert(); // error_string, selector=0x8c379a0, string_length=37
        }
        else {
          // block_3:
          private__0x1b46_0x1b46(slot_1[uint256(uint160(msg.sender))] - arg1, uint256(uint160(arg0)), uint256(uint160(msg.sender)), 5336);
          return 1;
        }
    }

    function disableTransferDelay() public returns (uint256 ret0) {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_1:
          slot_21 = slot_21 & uint256((type(uint256).max - 255));
          return 1;
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function enableTrade() public {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_1:
          slot_17 = slot_17 & uint256((type(uint256).max - 16776960)) | uint256(65792);
          slot_13 = block.timestamp;
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function excludeFromFees(address arg0, bool arg1) public {
        // block_0:
        require(msg.data.length > 67);
        // block_1:
        private__0x2cd4_0x2cd4(uint256(uint160(arg0)), 11743);
        require(false /* TODO: evm.eq.i.i */);
        // block_4:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_5:
          emit Event_0x9D8F7706EA1113D1A167B526ECA956215946DD36CC7DF39EB16180222D8B5DF7(uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)); // TODO: recover event signature
        }
        else {
          // block_6:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function excludeFromMaxTransaction(address arg0, bool arg1) public {
        // block_0:
        require(msg.data.length > 67);
        // block_1:
        private__0x2cd4_0x2cd4(uint256(uint160(arg0)), 11743);
        require(false /* TODO: evm.eq.i.i */);
        // block_4:
    }

    function increaseAllowance(address arg0, uint256 arg1) public returns (bool ret0) {
        // block_0:
        require(msg.data.length > 67);
        // block_2:
        private__0x2cd4_0x2cd4(uint256(uint160(arg0)), 11524);
        return private__0xd36_0xd36(arg1, uint256(uint160(arg0)), 469106) != 0;
    }

    function isExcludedFromFees(address arg0) public returns (bool ret0) {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        private__0x2cd4_0x2cd4(uint256(uint160(arg0)), 851310);
        return slot_33[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0))] != 0;
    }

    function lastLpBurnTime() public returns (uint256 ret0) {
        // block_0:
        return slot_13;
    }

    function lastManualLpBurnTime() public returns (uint256 ret0) {
        // block_0:
        return slot_19;
    }

    function limitsInEffect() public returns (bool ret0) {
        // block_0:
        return slot_17 != 0;
    }

    function lpBurnEnabled() public returns (bool ret0) {
        // block_0:
        return slot_11 != 0;
    }

    function lpBurnFrequency() public returns (uint256 ret0) {
        // block_0:
        return slot_12;
    }

    function manualBurnFrequency() public returns (uint256 ret0) {
        // block_0:
        return slot_18;
    }

    function manualBurnLiquidityPairTokens(uint256 arg0) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // goto block_3
        }
        // goto block_16
        // block_3:
        if (block.timestamp > private__0x2f26_0x2f26(slot_19, slot_18, 6488)) {
          // goto block_4
        }
        // goto block_15
        // block_4:
        if (arg0 < 1001) {
          // goto block_5
        }
        // goto block_14
        // block_5:
        slot_19 = block.timestamp;
        if (false /* TODO: evm.iszero22.not */) {
          // goto block_13
        }
        // block_6:
        if (!false /* TODO: evm.iszero35 */) {
          // block_7:
        }
        // block_8:
        if (false /* TODO: evm.iszero53.not */) {
          // goto block_12
        }
        // block_9:
        if (false /* TODO: evm.iszero59.not */) {
          // block_11:
          revert(); // returndata_forward
        }
        else {
          // block_10:
          emit Event_0x8462566617872A3FBAB94534675218431FF9E204063EE3F4F43D965626A39ABB(); // TODO: recover event signature
          return 1;
        }
        // block_12:
        revert(); // empty
        // block_13:
        revert(); // returndata_forward
        // block_14:
        revert(); // error_string, selector=0x8c379a0, string_length=42
        // block_15:
        revert(); // error_string, selector=0x8c379a0, string_length=32
        // block_16:
        revert(); // error_string, selector=0x8c379a0
    }

    function manualsend() public {
        // block_0:
    }

    function manualswap() public {
        // block_0:
        if ((uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_9) {
          // goto block_2
        }
        // block_1:
        if ((uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_8) {
          // goto block_2
        }
        // goto block_11
        // block_2:
        if (!false /* TODO: evm.iszero.i.i.not */) {
          private__0x1b46_0x1b46(slot_0[uint256(uint160(address(this)))], slot_6 & (uint256(1) << uint256(160)) + uint256(type(uint256).max), uint256(uint160(address(this))), 9836);
          while (false /* TODO: evm.lt48.i.i.not */) {
            // block_9:
          }
          // block_5:
          require(!false /* TODO: evm.iszero60.i.i.not */);
          // block_6:
          if (false /* TODO: evm.iszero66.i.i.not */) {
            // block_7:
            revert(); // returndata_forward
          }
          else {
            // block_12:
          }
        }
        // block_10:
        revert(); // returndata_forward
        // block_11:
        revert(); // empty
    }

    function maxTransactionAmount() public returns (uint256 ret0) {
        // block_0:
        return slot_14;
    }

    function maxWallet() public returns (uint256 ret0) {
        // block_0:
        return slot_16;
    }

    function name() public {
        // block_0:
    }

    function owner() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5 & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function percentForLPBurn() public returns (uint256 ret0) {
        // block_0:
        return slot_10;
    }

    function removeLimits() public returns (uint256 ret0) {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_1:
          slot_17 = slot_17 & uint256((type(uint256).max - 255));
          return 1;
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function renounceOwnership() public {
        // block_0:
        private__0xe89_0xe89(469505);
    }

    function sellDevelopmentFee() public returns (uint256 ret0) {
        // block_0:
        return slot_29;
    }

    function sellLiquidityFee() public returns (uint256 ret0) {
        // block_0:
        return slot_28;
    }

    function sellMarketingFee() public returns (uint256 ret0) {
        // block_0:
        return slot_27;
    }

    function sellTotalFees() public returns (uint256 ret0) {
        // block_0:
        return slot_26;
    }

    function setAutoLPBurnSettings(uint256 arg0, uint256 arg1, bool arg2) public {
        // block_0:
        require(msg.data.length > 99);
        // block_1:
        require(false /* TODO: evm.eq.i.i */);
        // block_4:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_5:
          if (arg0 > 599) {
            // block_6:
            if (arg1 > 1000) {
              // block_8:
              revert(); // error_string, selector=0x8c379a0, string_length=48
            }
            else {
              // block_7:
              slot_12 = arg0;
              slot_10 = arg1;
            }
          }
          else {
            // block_9:
            revert(); // error_string, selector=0x8c379a0, string_length=51
          }
        }
        else {
          // block_10:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function setAutomatedMarketMakerPair(address arg0, bool arg1) public {
        // block_0:
        require(msg.data.length > 67);
        // block_1:
        private__0x2cd4_0x2cd4(uint256(uint160(arg0)), 11743);
        require(false /* TODO: evm.eq.i.i */);
        // block_4:
    }

    function swapEnabled() public returns (bool ret0) {
        // block_0:
        return slot_17 / uint256(65536) & uint256(255) != 0;
    }

    function swapTokensAtAmount() public returns (uint256 ret0) {
        // block_0:
        return slot_15;
    }

    function symbol() public {
        // block_0:
    }

    function tokensForDev() public returns (uint256 ret0) {
        // block_0:
        return slot_32;
    }

    function tokensForLiquidity() public returns (uint256 ret0) {
        // block_0:
        return slot_31;
    }

    function tokensForMarketing() public returns (uint256 ret0) {
        // block_0:
        return slot_30;
    }

    function totalSupply() public returns (uint256 ret0) {
        // block_0:
        return slot_2;
    }

    function tradingActive() public returns (bool ret0) {
        // block_0:
        return slot_17 / uint256(256) & uint256(255) != 0;
    }

    function transferDelayEnabled() public returns (bool ret0) {
        // block_0:
        return slot_21 != 0;
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
        // goto block_92
        // block_1:
        private__0x2cd4_0x2cd4(uint256(uint160(arg0)), 11617);
        private__0x2cd4_0x2cd4(uint256(uint160(arg1)), 11633);
        if (!(uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
          // goto block_2
        }
        // goto block_88
        // block_2:
        if (!(uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
          // goto block_3
        }
        // goto block_87
        // block_3:
        if (!(arg2 == 0)) {
          // goto block_4
        }
        // goto block_86
        // block_4:
        if (slot_17 == 0) {
          // goto block_30
        }
        // block_5:
        if ((slot_5 ^ uint256(uint160(arg0))) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // goto block_30
        }
        // block_6:
        if ((slot_5 ^ uint256(uint160(arg1))) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // goto block_30
        }
        // block_7:
        if (uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // goto block_30
        }
        // block_8:
        if (uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 57005) {
          // goto block_30
        }
        // block_9:
        if (!(slot_7 / (uint256(1) << uint256(160)) & uint256(255) == 0)) {
          // goto block_30
        }
        // block_10:
        if (slot_17 / uint256(256) & uint256(255) == 0) {
          // goto block_11
        }
        // goto block_13
        // block_11:
        if (slot_33[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_12
        }
        // goto block_13
        // block_12:
        if (slot_33[uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_85
        }
        // goto block_13
        // block_21:
        if (!(slot_35[uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_22
        }
        // goto block_23
        // block_22:
        if (!(slot_34[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_23
        }
        // goto block_26
        // block_23:
        if (!(slot_34[uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_30
        }
        // block_24:
        if (!(private__0x2f26_0x2f26(arg2, slot_0[uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)], 8434) > slot_16)) {
          // goto block_30
        }
        // block_25:
        require(false, "Max wallet exceeded");
        // block_26:
        if (!(arg2 > slot_14)) {
          // goto block_30
        }
        // block_27:
        revert(); // error_string, selector=0x8c379a0, string_length=54
        // block_30:
        if (slot_0[uint256(uint160(address(this)))] < slot_15) {
          // goto block_56
        }
        // block_31:
        if (slot_17 / uint256(65536) & uint256(255) == 0) {
          // goto block_56
        }
        // block_32:
        if (!(slot_7 / (uint256(1) << uint256(160)) & uint256(255) == 0)) {
          // goto block_56
        }
        // block_33:
        if (!(slot_35[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_56
        }
        // block_34:
        if (!(slot_33[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_56
        }
        // block_35:
        if (!(slot_33[uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_56
        }
        // block_36:
        slot_7 = uint256(1) << uint256(160) | slot_7 & ~(uint256(255) << uint256(160));
        temp_0 = private__0x2f26_0x2f26(private__0x2f26_0x2f26(slot_31, slot_30, 10346), slot_32, 10356);
        if (false /* TODO: spec.select */) {
          // goto block_55
        }
        // goto block_37
        // block_37:
        if (slot_0[uint256(uint160(address(this)))] > private__0x2ef0_0x2ef0(20, slot_15, 10395)) {
          // block_38:
          if (arg2 > private__0x2ef0_0x2ef0(20, slot_15, 10416)) {
            // block_39:
          }
        }
        // block_40:
        if (false /* TODO: evm.iszero.i.i.i.not */) {
          // goto block_48
        }
        while (true) {
          // block_41:
          // block_42:
          if (!false /* TODO: evm.lt48.i.i.i.not */) {
            break;
          }
          continue;
        }
        // block_43:
        if (!false /* TODO: evm.iszero60.i.i.i.not */) {
          // goto block_44
        }
        // goto block_46
        // block_44:
        if (false /* TODO: evm.iszero66.i.i.i.not */) {
          // goto block_45
        }
        // goto block_49
        // block_45:
        revert(); // returndata_forward
        // block_46:
        revert(); // empty
        // block_48:
        revert(); // returndata_forward
        // block_49:
        temp_1 = private__0x2bcf_0x2bcf(address(this).balance, address(this).balance, 10522);
        temp_2 = private__0x26e7_0x26e7(temp_0, private__0x26d5_0x26d5(slot_32, temp_1, 851117), 10578);
        temp_3 = private__0x2ff3_0x2ff3(private__0x2ff3_0x2ff3(temp_1, private__0x26e7_0x26e7(temp_0, private__0x26d5_0x26d5(slot_30, temp_1, 851080), 10550), 10592), temp_2, 10602);
        slot_31 = 0;
        slot_30 = 0;
        slot_32 = 0;
        if (!false /* TODO: evm.eq.i.i */) {
          // block_50:
        }
        // block_51:
        if (false /* TODO: spec.select1 */) {
          // goto block_52
        }
        // goto block_53
        // block_52:
        emit Event_0x17BBFB9A6069321B6DED73BD96327C9E6B7212A5CD51FF219CD61370ACAFB561(); // TODO: recover event signature
        // block_53:
        if (!false /* TODO: evm.eq76.i.i */) {
          // block_54:
        }
        // block_55:
        slot_7 = slot_7 & ~(uint256(255) << uint256(160));
        // block_56:
        if (!(slot_7 / (uint256(1) << uint256(160)) & uint256(255) == 0)) {
          // goto block_67
        }
        // block_57:
        if (slot_35[uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_67
        }
        // block_58:
        if (slot_11 == 0) {
          // goto block_67
        }
        // block_59:
        if (block.timestamp < private__0x2f26_0x2f26(slot_13, slot_12, 8808)) {
          // goto block_67
        }
        // block_60:
        if (!(slot_33[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_67
        }
        // block_61:
        slot_13 = block.timestamp;
        if (false /* TODO: evm.iszero339.i.not */) {
          // goto block_81
        }
        // goto block_62
        // block_62:
        if (!false /* TODO: evm.iszero353.i */) {
          // block_63:
        }
        // block_64:
        if (false /* TODO: evm.iszero372.i.not */) {
          // goto block_80
        }
        // goto block_65
        // block_65:
        if (false /* TODO: evm.iszero378.i.not */) {
          // block_79:
          revert(); // returndata_forward
        }
        // block_66:
        emit Event_0x454C91AE84FCC766DDDA0DCB289F26B3D0176EFEACF4061FC219FA6CA8C3048D(); // TODO: recover event signature
        // block_67:
        if (!(slot_33[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_78
        }
        // block_68:
        if (false /* TODO: cond */) {
          // goto block_78
        }
        // block_69:
        if (slot_35[uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_71
        }
        // block_70:
        if (slot_26 == 0) {
          // goto block_71
        }
        // goto block_74
        // block_71:
        if (!(slot_35[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // block_72:
          if (!(slot_22 == 0)) {
            // block_73:
            temp_4 = private__0x26e7_0x26e7(100, private__0x26d5_0x26d5(slot_22, arg2, 850887), 9246);
            slot_31 = private__0x2f26_0x2f26(slot_31, private__0x2f07_0x2f07(private__0x2ef0_0x2ef0(temp_4, slot_24, 9265), slot_22, 9275), 9291);
            slot_32 = private__0x2f26_0x2f26(slot_32, private__0x2f07_0x2f07(private__0x2ef0_0x2ef0(temp_4, slot_25, 9312), slot_22, 9322), 9338);
            slot_30 = private__0x2f26_0x2f26(slot_30, private__0x2f07_0x2f07(private__0x2ef0_0x2ef0(temp_4, slot_23, 9359), slot_22, 9369), 9385);
          }
        }
        // goto block_75
        // block_74:
        temp_5 = private__0x26e7_0x26e7(100, private__0x26d5_0x26d5(slot_26, arg2, 850850), 9025);
        slot_31 = private__0x2f26_0x2f26(slot_31, private__0x2f07_0x2f07(private__0x2ef0_0x2ef0(temp_5, slot_28, 9044), slot_26, 9054), 9070);
        slot_32 = private__0x2f26_0x2f26(slot_32, private__0x2f07_0x2f07(private__0x2ef0_0x2ef0(temp_5, slot_29, 9091), slot_26, 9101), 9117);
        slot_30 = private__0x2f26_0x2f26(slot_30, private__0x2f07_0x2f07(private__0x2ef0_0x2ef0(temp_5, slot_27, 9138), slot_26, 9148), 9164);
        // goto block_75
        // block_75:
        if (!false /* TODO: evm.iszero481.i */) {
          // block_76:
        }
        // block_77:
        // block_78:
        // goto block_89
        // block_80:
        revert(); // empty
        // block_81:
        revert(); // returndata_forward
        // block_13:
        if (!(slot_21 == 0)) {
          // block_14:
          if (!((slot_5 ^ uint256(uint160(arg1))) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
            // block_15:
            if (!((slot_6 ^ uint256(uint160(arg1))) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
              // block_16:
              if (!((slot_7 ^ uint256(uint160(arg1))) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
                // block_17:
                if (!(block.number > slot_20[uint256(uint160(tx.origin))])) {
                  // block_84:
                  revert(); // error_string, selector=0x8c379a0, string_length=73
                }
                // block_18:
              }
            }
          }
        }
        // block_19:
        if (slot_35[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_21
        }
        // block_20:
        if (!(slot_34[uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_21
        }
        // goto block_28
        // block_28:
        if (arg2 > slot_14) {
          // block_83:
          revert(); // error_string, selector=0x8c379a0, string_length=53
        }
        // block_29:
        if (!(private__0x2f26_0x2f26(arg2, slot_0[uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)], 8112) > slot_16)) {
          // goto block_30
        }
        // goto block_82
        // block_82:
        require(false, "Max wallet exceeded");
        // block_85:
        require(false, "Trading is not active.");
        // block_86:
        private__0x26f2_0x26f2(0, uint256(uint160(arg1)), uint256(uint160(arg0)), 7367);
        // block_89:
        if (slot_1[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] < arg2) {
          // block_91:
          revert(); // error_string, selector=0x8c379a0, string_length=40
        }
        else {
          // block_90:
          private__0x1b46_0x1b46(slot_1[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] - arg2, uint256(uint160(msg.sender)), uint256(uint160(arg0)), 3371);
          return 1;
        }
        // block_87:
        revert(); // error_string, selector=0x8c379a0
        // block_88:
        revert(); // error_string, selector=0x8c379a0
        // block_92:
        revert(); // empty
    }

    function transferOwnership(address arg0) public {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        private__0x2cd4_0x2cd4(uint256(uint160(arg0)), 851310);
        private__0x1884_0x1884(uint256(uint160(arg0)), 470871);
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
        private__0x2cd4_0x2cd4(uint256(uint160(arg0)), 11524);
        if (uint256(uint160(msg.sender)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // goto block_89
        }
        // goto block_3
        // block_22:
        if (!(slot_35[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_23
        }
        // goto block_24
        // block_23:
        if (!(slot_34[uint256(uint160(msg.sender)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_24
        }
        // goto block_27
        // block_24:
        if (!(slot_34[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_31
        }
        // block_25:
        if (!(private__0x2f26_0x2f26(arg1, slot_0[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)], 8434) > slot_16)) {
          // goto block_31
        }
        // block_26:
        require(false, "Max wallet exceeded");
        // block_27:
        if (!(arg1 > slot_14)) {
          // goto block_31
        }
        // block_28:
        revert(); // error_string, selector=0x8c379a0, string_length=54
        // block_31:
        if (slot_0[uint256(uint160(address(this)))] < slot_15) {
          // goto block_57
        }
        // block_32:
        if (slot_17 / uint256(65536) & uint256(255) == 0) {
          // goto block_57
        }
        // block_33:
        if (!(slot_7 / (uint256(1) << uint256(160)) & uint256(255) == 0)) {
          // goto block_57
        }
        // block_34:
        if (!(slot_35[uint256(uint160(msg.sender)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_57
        }
        // block_35:
        if (!(slot_33[uint256(uint160(msg.sender)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_57
        }
        // block_36:
        if (!(slot_33[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_57
        }
        // block_37:
        slot_7 = uint256(1) << uint256(160) | slot_7 & ~(uint256(255) << uint256(160));
        temp_0 = private__0x2f26_0x2f26(private__0x2f26_0x2f26(slot_31, slot_30, 10346), slot_32, 10356);
        if (false /* TODO: spec.select */) {
          // goto block_56
        }
        // block_38:
        if (slot_0[uint256(uint160(address(this)))] > private__0x2ef0_0x2ef0(20, slot_15, 10395)) {
          // block_39:
          if (arg1 > private__0x2ef0_0x2ef0(20, slot_15, 10416)) {
            // block_40:
          }
        }
        // block_41:
        if (false /* TODO: evm.iszero.i.i.i.i.not */) {
          // goto block_49
        }
        while (true) {
          // block_42:
          // block_43:
          if (!false /* TODO: evm.lt48.i.i.i.i.not */) {
            break;
          }
          continue;
        }
        // block_44:
        if (!false /* TODO: evm.iszero60.i.i.i.i.not */) {
          // goto block_45
        }
        // goto block_47
        // block_45:
        if (false /* TODO: evm.iszero66.i.i.i.i.not */) {
          // goto block_46
        }
        // goto block_50
        // block_46:
        revert(); // returndata_forward
        // block_47:
        revert(); // empty
        // block_49:
        revert(); // returndata_forward
        // block_50:
        temp_1 = private__0x2bcf_0x2bcf(address(this).balance, address(this).balance, 10522);
        temp_2 = private__0x26e7_0x26e7(temp_0, private__0x26d5_0x26d5(slot_32, temp_1, 851117), 10578);
        temp_3 = private__0x2ff3_0x2ff3(private__0x2ff3_0x2ff3(temp_1, private__0x26e7_0x26e7(temp_0, private__0x26d5_0x26d5(slot_30, temp_1, 851080), 10550), 10592), temp_2, 10602);
        slot_31 = 0;
        slot_30 = 0;
        slot_32 = 0;
        if (!false /* TODO: evm.eq.i.i.i */) {
          // block_51:
        }
        // block_52:
        if (false /* TODO: spec.select1 */) {
          // goto block_53
        }
        // goto block_54
        // block_53:
        emit Event_0x17BBFB9A6069321B6DED73BD96327C9E6B7212A5CD51FF219CD61370ACAFB561(); // TODO: recover event signature
        // block_54:
        if (!false /* TODO: evm.eq76.i.i.i */) {
          // block_55:
        }
        // block_56:
        slot_7 = slot_7 & ~(uint256(255) << uint256(160));
        // block_57:
        if (!(slot_7 / (uint256(1) << uint256(160)) & uint256(255) == 0)) {
          // goto block_68
        }
        // block_58:
        if (slot_35[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_68
        }
        // block_59:
        if (slot_11 == 0) {
          // goto block_68
        }
        // block_60:
        if (block.timestamp < private__0x2f26_0x2f26(slot_13, slot_12, 8808)) {
          // goto block_68
        }
        // block_61:
        if (!(slot_33[uint256(uint160(msg.sender)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_68
        }
        // block_62:
        slot_13 = block.timestamp;
        if (false /* TODO: evm.iszero339.i.i.not */) {
          // goto block_82
        }
        // block_63:
        if (!false /* TODO: evm.iszero353.i.i */) {
          // block_64:
        }
        // block_65:
        if (false /* TODO: evm.iszero372.i.i.not */) {
          // goto block_81
        }
        // block_66:
        if (false /* TODO: evm.iszero378.i.i.not */) {
          // block_80:
          revert(); // returndata_forward
        }
        // block_67:
        emit Event_0x454C91AE84FCC766DDDA0DCB289F26B3D0176EFEACF4061FC219FA6CA8C3048D(); // TODO: recover event signature
        // block_68:
        if (!(slot_33[uint256(uint160(msg.sender)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_79
        }
        // block_69:
        if (false /* TODO: cond */) {
          // goto block_79
        }
        // block_70:
        if (slot_35[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_72
        }
        // block_71:
        if (slot_26 == 0) {
          // goto block_72
        }
        // goto block_75
        // block_72:
        if (!(slot_35[uint256(uint160(msg.sender)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // block_73:
          if (!(slot_22 == 0)) {
            // block_74:
            temp_4 = private__0x26e7_0x26e7(100, private__0x26d5_0x26d5(slot_22, arg1, 850887), 9246);
            slot_31 = private__0x2f26_0x2f26(slot_31, private__0x2f07_0x2f07(private__0x2ef0_0x2ef0(temp_4, slot_24, 9265), slot_22, 9275), 9291);
            slot_32 = private__0x2f26_0x2f26(slot_32, private__0x2f07_0x2f07(private__0x2ef0_0x2ef0(temp_4, slot_25, 9312), slot_22, 9322), 9338);
            slot_30 = private__0x2f26_0x2f26(slot_30, private__0x2f07_0x2f07(private__0x2ef0_0x2ef0(temp_4, slot_23, 9359), slot_22, 9369), 9385);
          }
        }
        // goto block_76
        // block_75:
        temp_5 = private__0x26e7_0x26e7(100, private__0x26d5_0x26d5(slot_26, arg1, 850850), 9025);
        slot_31 = private__0x2f26_0x2f26(slot_31, private__0x2f07_0x2f07(private__0x2ef0_0x2ef0(temp_5, slot_28, 9044), slot_26, 9054), 9070);
        slot_32 = private__0x2f26_0x2f26(slot_32, private__0x2f07_0x2f07(private__0x2ef0_0x2ef0(temp_5, slot_29, 9091), slot_26, 9101), 9117);
        slot_30 = private__0x2f26_0x2f26(slot_30, private__0x2f07_0x2f07(private__0x2ef0_0x2ef0(temp_5, slot_27, 9138), slot_26, 9148), 9164);
        // block_76:
        if (!false /* TODO: evm.iszero481.i.i */) {
          // block_77:
        }
        // block_78:
        // block_79:
        // goto block_90
        // block_81:
        revert(); // empty
        // block_82:
        revert(); // returndata_forward
        // block_3:
        if (uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // block_88:
          revert(); // error_string, selector=0x8c379a0
        }
        // block_4:
        if (arg1 == 0) {
          // goto block_87
        }
        // goto block_5
        // block_5:
        // block_6:
        if ((slot_5 ^ uint256(uint160(msg.sender))) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // goto block_31
        }
        // goto block_7
        // block_7:
        if ((slot_5 ^ uint256(uint160(arg0))) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // goto block_31
        }
        // goto block_8
        // block_8:
        if (uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // goto block_31
        }
        // goto block_9
        // block_9:
        if (uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 57005) {
          // goto block_31
        }
        // goto block_10
        // block_10:
        if (!(slot_7 / (uint256(1) << uint256(160)) & uint256(255) == 0)) {
          // goto block_31
        }
        // goto block_11
        // block_11:
        // block_14:
        if (!(slot_21 == 0)) {
          // block_15:
          if (!((slot_5 ^ uint256(uint160(arg0))) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
            // block_16:
            if (!((slot_6 ^ uint256(uint160(arg0))) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
              // block_17:
              if (!((slot_7 ^ uint256(uint160(arg0))) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
                // block_18:
                if (!(block.number > slot_20[uint256(uint160(tx.origin))])) {
                  // block_85:
                  revert(); // error_string, selector=0x8c379a0, string_length=73
                }
                // block_19:
              }
            }
          }
        }
        // block_20:
        // block_21:
        if (!(slot_34[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_22
        }
        // goto block_29
        // block_29:
        if (arg1 > slot_14) {
          // block_84:
          revert(); // error_string, selector=0x8c379a0, string_length=53
        }
        // block_30:
        if (!(private__0x2f26_0x2f26(arg1, slot_0[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)], 8112) > slot_16)) {
          // goto block_31
        }
        // goto block_83
        // block_83:
        require(false, "Max wallet exceeded");
        // block_86:
        require(false, "Trading is not active.");
        // block_87:
        private__0x26f2_0x26f2(0, uint256(uint160(arg0)), uint256(uint160(msg.sender)), 7367);
        // block_90:
        return 1;
        // block_89:
        revert(); // error_string, selector=0x8c379a0
    }

    function uniswapV2Pair() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_7 & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function uniswapV2Router() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_6 & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function updateBuyFees(uint256 arg0, uint256 arg1, uint256 arg2) public {
        // block_0:
        require(msg.data.length > 99);
        // block_2:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_3:
          slot_23 = arg0;
          slot_24 = arg1;
          slot_25 = arg2;
          slot_22 = private__0x2f26_0x2f26(private__0x2f26_0x2f26(arg0, arg1, 4417), arg2, 4427);
        }
        else {
          // block_4:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function updateMaxTxnAmount(uint256 arg0) public {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_3:
          if (arg0 < private__0x2f07_0x2f07(private__0x2f07_0x2f07(private__0x2ef0_0x2ef0(1, slot_2, 3067), 1000, 3077), 1000000000000000000, 3087)) {
            // block_5:
            revert(); // error_string, selector=0x8c379a0, string_length=47
          }
          else {
            // block_4:
            slot_14 = private__0x2ef0_0x2ef0(1000000000000000000, arg0, 3208);
          }
        }
        else {
          // block_6:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function updateMaxWalletAmount(uint256 arg0) public {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_3:
          if (arg0 < private__0x2f07_0x2f07(private__0x2f07_0x2f07(private__0x2ef0_0x2ef0(5, slot_2, 5654), 1000, 5664), 1000000000000000000, 5674)) {
            // block_5:
            revert(); // error_string, selector=0x8c379a0, string_length=36
          }
          else {
            // block_4:
            slot_16 = private__0x2ef0_0x2ef0(1000000000000000000, arg0, 5783);
          }
        }
        else {
          // block_6:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function updateSellFees(uint256 arg0, uint256 arg1, uint256 arg2) public {
        // block_0:
        require(msg.data.length > 99);
        // block_2:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_3:
          slot_27 = arg0;
          slot_28 = arg1;
          slot_29 = arg2;
          slot_26 = private__0x2f26_0x2f26(private__0x2f26_0x2f26(arg0, arg1, 5562), arg2, 5572);
        }
        else {
          // block_4:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function updateSwapEnabled(bool arg0) public {
        // block_0:
        require(msg.data.length > 35);
        // block_1:
        require(false /* TODO: evm.eq.i.i */);
        // block_4:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_5:
        }
        else {
          // block_6:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function updateSwapTokensAtAmount(uint256 arg0) public returns (bool ret0) {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        return private__0x16f2_0x16f2(arg0, 470597) != 0;
    }

    function private__0x1884_0x1884(uint256 arg0, uint256 arg1) internal {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_1:
          if (arg0 & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
            // block_3:
            revert(); // error_string, selector=0x8c379a0, string_length=38
          }
          else {
            // block_2:
            private__0x24e1_0x24e1(arg0, 850659);
          }
        }
        else {
          // block_4:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function private__0x1023_0x1023(uint256 arg0, uint256 arg1, uint256 arg2) internal {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_1:
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function private__0x11a8_0x11a8(uint256 arg0, uint256 arg1, uint256 arg2) internal {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_1:
          if ((arg1 ^ slot_7) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
            // block_3:
            revert(); // error_string, selector=0x8c379a0, string_length=57
          }
          else {
            // block_2:
            private__0x2532_0x2532(arg0, arg1, 4704);
          }
        }
        else {
          // block_4:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function private__0x16f2_0x16f2(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_1:
          if (arg0 < private__0x2f07_0x2f07(private__0x2ef0_0x2ef0(1, slot_2, 5942), 100000, 5952)) {
            // block_5:
            revert(); // error_string, selector=0x8c379a0, string_length=53
          }
          else {
            // block_2:
            if (arg0 > private__0x2f07_0x2f07(private__0x2ef0_0x2ef0(5, slot_2, 6084), 1000, 6094)) {
              // block_4:
              revert(); // error_string, selector=0x8c379a0, string_length=52
            }
            else {
              // block_3:
              slot_15 = arg0;
              return 1;
            }
          }
        }
        else {
          // block_6:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function private__0xd36_0xd36(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        private__0x1b46_0x1b46(private__0x2f26_0x2f26(slot_1[uint256(uint160(msg.sender))], arg0, 3436), arg1, uint256(uint160(msg.sender)), 597465);
        return 1;
    }

    function private__0x1b46_0x1b46(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
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

    function private__0x24e1_0x24e1(uint256 arg0, uint256 arg1) internal {
        // block_0:
        slot_5 = (uint256(1) << uint256(160)) + uint256(type(uint256).max) & arg0 | slot_5 & uint256(0) - (uint256(1) << uint256(160));
        emit Event_0x8BE0079C531659141344CD1FD0A4F28419497F9722A3DAAFE3B4186F6B6457E0(slot_5 & (uint256(1) << uint256(160)) + uint256(type(uint256).max), (uint256(1) << uint256(160)) + uint256(type(uint256).max) & arg0); // TODO: recover event signature
    }

    function private__0x2532_0x2532(uint256 arg0, uint256 arg1, uint256 arg2) internal {
        // block_0:
        emit Event_0xFFA9187BF1F18BF477BD0EA1BCBB64E93B6A98132473929EDFCE215CD9B16FAB(arg1 & (uint256(1) << uint256(160)) + uint256(type(uint256).max), arg0 != 0); // TODO: recover event signature
    }

    function private__0x26d5_0x26d5(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return private__0x2ef0_0x2ef0(arg1, arg0, 850924);
    }

    function private__0x26e7_0x26e7(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return private__0x2f07_0x2f07(arg1, arg0, 850962);
    }

    function private__0x26f2_0x26f2(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
        // block_0:
        if (arg2 & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // block_6:
          revert(); // error_string, selector=0x8c379a0
        }
        else {
          // block_1:
          if (arg1 & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
            // block_5:
            revert(); // error_string, selector=0x8c379a0
          }
          else {
            // block_2:
            if (slot_0[arg2 & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] < arg0) {
              // block_4:
              revert(); // error_string, selector=0x8c379a0, string_length=38
            }
            else {
              // block_3:
              emit Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF((uint256(1) << uint256(160)) + uint256(type(uint256).max) & arg2, (uint256(1) << uint256(160)) + uint256(type(uint256).max) & arg1); // TODO: recover event signature
            }
          }
        }
    }

    function private__0x2bcf_0x2bcf(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return private__0x2ff3_0x2ff3(arg1, arg0, 851238);
    }

    function private__0x2cd4_0x2cd4(uint256 arg0, uint256 arg1) internal {
        // block_0:
        require(arg0 & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_1:
    }

    function private__0x2e6f_0x2e6f(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(96);
    }

    function private__0x2ef0_0x2ef0(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg1 * arg0;
    }

    function private__0x2f07_0x2f07(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 / arg1;
    }

    function private__0x2f26_0x2f26(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg1 + arg0;
    }

    function private__0x2f6b_0x2f6b(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(128);
    }

    function private__0x2fb0_0x2fb0(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(128);
    }

    function private__0x2ff3_0x2ff3(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 - arg1;
    }

    function private__0xb9b_0xb9b(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        private__0x1b46_0x1b46(arg0, arg1, uint256(uint160(msg.sender)), 471238);
        return 1;
    }

    function private__0xe89_0xe89(uint256 arg0) internal {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_1:
          private__0x24e1_0x24e1(0, 3772);
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
    }
}
