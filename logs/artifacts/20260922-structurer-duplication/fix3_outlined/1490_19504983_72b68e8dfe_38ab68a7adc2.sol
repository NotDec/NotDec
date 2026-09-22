contract Decompiled {
    event Event_0x8C5BE1E5EBEC7D5BD14F71427D1E84F3DD0314C0F7B2291E5B200AC8C7C3B925(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0x454C91AE84FCC766DDDA0DCB289F26B3D0176EFEACF4061FC219FA6CA8C3048D();
    event Event_0x8BE0079C531659141344CD1FD0A4F28419497F9722A3DAAFE3B4186F6B6457E0(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0xFFA9187BF1F18BF477BD0EA1BCBB64E93B6A98132473929EDFCE215CD9B16FAB(uint256 indexed arg0, bool indexed arg1);
    event Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0x17BBFB9A6069321B6DED73BD96327C9E6B7212A5CD51FF219CD61370ACAFB561();
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
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_1:
          emit Event_0xFAF1B77ED79F6E898C44DD8AB36B330C7B2FD39BCAAB05ED6362480DF8703965(0 /* TODO: unresolved value: private__0x2d13_0x2d13 */ & (uint256(1) << uint256(160)) + uint256(type(uint256).max), (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_9); // TODO: recover event signature
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function public_0x7d831a8b() public {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_1:
          emit Event_0xA751787977EEB3902E30E1D19CA00C6AD274A1F622C31A206E32366700B05674(0 /* TODO: unresolved value: private__0x2d13_0x2d13 */ & (uint256(1) << uint256(160)) + uint256(type(uint256).max), (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_8); // TODO: recover event signature
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function public_0xeeeeeeee() public {
        // block_0:
    }

    function isExcludedMaxTransactionAmount(address arg0) public returns (bool ret0) {
        // block_0:
        return slot_34[uint256(uint160(arg0))] != 0;
    }

    function allowance(address arg0, address arg1) public returns (uint256 ret0) {
        // block_0:
        return slot_1[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0))];
    }

    function approve(address arg0, uint256 arg1) public returns (bool ret0) {
        // block_0:
        return private__0xb9b_0xb9b(arg1, uint256(uint160(arg0)), 468369) != 0;
    }

    function automatedMarketMakerPairs(address arg0) public returns (bool ret0) {
        // block_0:
        return slot_35[uint256(uint160(arg0))] != 0;
    }

    function balanceOf(address arg0) public returns (uint256 ret0) {
        // block_0:
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
        if (slot_1[uint256(uint160(msg.sender))] < arg1) {
          // block_2:
          revert(); // error_string, selector=0x8c379a0, string_length=37
        }
        else {
          // block_1:
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
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_1:
          emit Event_0x9D8F7706EA1113D1A167B526ECA956215946DD36CC7DF39EB16180222D8B5DF7(uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)); // TODO: recover event signature
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function excludeFromMaxTransaction(address arg0, bool arg1) public {
        // block_0:
    }

    function increaseAllowance(address arg0, uint256 arg1) public returns (bool ret0) {
        // block_0:
        return private__0xd36_0xd36(arg1, uint256(uint160(arg0)), 469130) != 0;
    }

    function isExcludedFromFees(address arg0) public returns (bool ret0) {
        // block_0:
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
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // goto block_1
        }
        // goto block_14
        // block_1:
        if (block.timestamp > private__0x2f27_0x2f27(slot_19, slot_18, 6488)) {
          // goto block_2
        }
        // goto block_13
        // block_2:
        if (arg0 < 1001) {
          // goto block_3
        }
        // goto block_12
        // block_3:
        slot_19 = block.timestamp;
        if (false /* TODO: evm.iszero22.not */) {
          // goto block_11
        }
        // goto block_4
        // block_4:
        if (!false /* TODO: evm.iszero35 */) {
          // block_5:
        }
        // block_6:
        if (false /* TODO: evm.iszero53.not */) {
          // goto block_10
        }
        // block_7:
        if (false /* TODO: evm.iszero59.not */) {
          // block_9:
          revert(); // returndata_forward
        }
        else {
          // block_8:
          emit Event_0x8462566617872A3FBAB94534675218431FF9E204063EE3F4F43D965626A39ABB(); // TODO: recover event signature
          return 1;
        }
        // block_10:
        revert(); // empty
        // block_11:
        revert(); // returndata_forward
        // block_12:
        revert(); // error_string, selector=0x8c379a0, string_length=42
        // block_13:
        revert(); // error_string, selector=0x8c379a0, string_length=32
        // block_14:
        revert(); // error_string, selector=0x8c379a0
    }

    function manualsend() public {
        // block_0:
    }

    function manualswap() public {
        // block_0:
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
        private__0xe89_0xe89(469529);
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
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_1:
          if (arg0 > 599) {
            // block_2:
            if (arg1 > 1000) {
              // block_4:
              revert(); // error_string, selector=0x8c379a0, string_length=48
            }
            else {
              // block_3:
              slot_12 = arg0;
              slot_10 = arg1;
            }
          }
          else {
            // block_5:
            revert(); // error_string, selector=0x8c379a0, string_length=51
          }
        }
        else {
          // block_6:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function setAutomatedMarketMakerPair(address arg0, bool arg1) public {
        // block_0:
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
        require(msg.data.length > 99);
        // block_1:
        private__0x2cd5_0x2cd5(uint256(uint160(arg0)), 11618);
        private__0x2cd5_0x2cd5(uint256(uint160(arg1)), 11634);
        if (slot_1[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] < arg2) {
          // block_3:
          revert(); // error_string, selector=0x8c379a0, string_length=40
        }
        else {
          // block_2:
          private__0x1b46_0x1b46(slot_1[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] - arg2, uint256(uint160(msg.sender)), uint256(uint160(arg0)), 3371);
          return 1;
        }
    }

    function transferOwnership(address arg0) public {
        // block_0:
        private__0x1884_0x1884(uint256(uint160(arg0)), 470895);
    }

    function transfer(address arg0, uint256 arg1) public returns (bool ret0) {
        // block_0:
        return private__0x14e2_0x14e2(arg1, uint256(uint160(arg0)), 470211) != 0;
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
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_1:
          slot_23 = arg0;
          slot_24 = arg1;
          slot_25 = arg2;
          slot_22 = private__0x2f27_0x2f27(private__0x2f27_0x2f27(arg0, arg1, 4417), arg2, 4427);
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function updateMaxTxnAmount(uint256 arg0) public {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_1:
          if (arg0 < private__0x2f08_0x2f08(private__0x2f08_0x2f08(private__0x2ef1_0x2ef1(1, slot_2, 3067), 1000, 3077), 1000000000000000000, 3087)) {
            // block_3:
            revert(); // error_string, selector=0x8c379a0, string_length=47
          }
          else {
            // block_2:
            slot_14 = private__0x2ef1_0x2ef1(1000000000000000000, arg0, 3208);
          }
        }
        else {
          // block_4:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function updateMaxWalletAmount(uint256 arg0) public {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_1:
          if (arg0 < private__0x2f08_0x2f08(private__0x2f08_0x2f08(private__0x2ef1_0x2ef1(5, slot_2, 5654), 1000, 5664), 1000000000000000000, 5674)) {
            // block_3:
            revert(); // error_string, selector=0x8c379a0, string_length=36
          }
          else {
            // block_2:
            slot_16 = private__0x2ef1_0x2ef1(1000000000000000000, arg0, 5783);
          }
        }
        else {
          // block_4:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function updateSellFees(uint256 arg0, uint256 arg1, uint256 arg2) public {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_1:
          slot_27 = arg0;
          slot_28 = arg1;
          slot_29 = arg2;
          slot_26 = private__0x2f27_0x2f27(private__0x2f27_0x2f27(arg0, arg1, 5562), arg2, 5572);
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function updateSwapEnabled(bool arg0) public {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_1:
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function updateSwapTokensAtAmount(uint256 arg0) public returns (bool ret0) {
        // block_0:
        return private__0x16f2_0x16f2(arg0, 470621) != 0;
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
            private__0x24e1_0x24e1(arg0, 850719);
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

    function private__0x14e2_0x14e2(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return 1;
    }

    function private__0x16f2_0x16f2(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_1:
          if (arg0 < private__0x2f08_0x2f08(private__0x2ef1_0x2ef1(1, slot_2, 5942), 100000, 5952)) {
            // block_5:
            revert(); // error_string, selector=0x8c379a0, string_length=53
          }
          else {
            // block_2:
            if (arg0 > private__0x2f08_0x2f08(private__0x2ef1_0x2ef1(5, slot_2, 6084), 1000, 6094)) {
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
        private__0x1b46_0x1b46(private__0x2f27_0x2f27(slot_1[uint256(uint160(msg.sender))], arg0, 3436), arg1, uint256(uint160(msg.sender)), 597501);
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
        return private__0x2ef1_0x2ef1(arg1, arg0, 850984);
    }

    function private__0x26e7_0x26e7(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return private__0x2f08_0x2f08(arg1, arg0, 851022);
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
        return private__0x2ff4_0x2ff4(arg1, arg0, 851298);
    }

    function private__0x2cd5_0x2cd5(uint256 arg0, uint256 arg1) internal {
        // block_0:
        require(arg0 & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_1:
    }

    function private__0x2e70_0x2e70(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(96);
    }

    function private__0x2ef1_0x2ef1(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg1 * arg0;
    }

    function private__0x2f08_0x2f08(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 / arg1;
    }

    function private__0x2f27_0x2f27(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg1 + arg0;
    }

    function private__0x2f6c_0x2f6c(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(128);
    }

    function private__0x2fb1_0x2fb1(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(128);
    }

    function private__0x2ff4_0x2ff4(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 - arg1;
    }

    function private__0xb9b_0xb9b(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        private__0x1b46_0x1b46(arg0, arg1, uint256(uint160(msg.sender)), 471262);
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
