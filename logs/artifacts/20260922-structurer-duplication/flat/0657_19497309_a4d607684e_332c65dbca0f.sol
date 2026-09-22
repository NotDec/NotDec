contract Decompiled {
    event Event_0x9D8F7706EA1113D1A167B526ECA956215946DD36CC7DF39EB16180222D8B5DF7(uint256 indexed arg0);
    event Event_0x8C5BE1E5EBEC7D5BD14F71427D1E84F3DD0314C0F7B2291E5B200AC8C7C3B925(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0xFFA9187BF1F18BF477BD0EA1BCBB64E93B6A98132473929EDFCE215CD9B16FAB(uint256 indexed arg0, bool indexed arg1);
    event Event_0x6B4F1BE9103E6CBCD38CA4A922334F2C3109B260130A6676A987F94088FD6746();
    event Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0x55678E47D0A699D3AB99B0184C4FF14F2246BA80522DEB921AA0C8823578C44A();
    event Event_0x4EFA56652237561D0F1FD31311AEAAA41F3B754A461545ED3CF6CED5876D2982(uint256 indexed arg0);
    event Event_0x8BE0079C531659141344CD1FD0A4F28419497F9722A3DAAFE3B4186F6B6457E0(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0xA56FEB2D31B9A7424DB0BE063FD450863979C9E2382CF5110F869BD1AD361BB7();
    event Event_0xDEDA980967FCEAD7B61E78AC46A4DA14274AF29E894D4D61E8B81EC38AB3E438();

    mapping(uint256 => uint256) public slot_0;
    mapping(uint256 => uint256) public slot_1;
    uint256 public slot_10;
    uint256 public slot_11;
    uint256 public slot_12;
    uint256 public slot_13;
    uint256 public slot_14;
    mapping(uint256 => uint256) public slot_15;
    uint256[] public slot_16;
    uint256 public slot_17;
    uint256 public slot_18;
    mapping(uint256 => uint256) public slot_19;
    uint256 public slot_2;
    uint256 public slot_20;
    uint256 public slot_21;
    uint256 public slot_22;
    uint256 public slot_23;
    uint256 public slot_24;
    uint256 public slot_25;
    uint256 public slot_26;
    uint256 public slot_27;
    uint256 public slot_28;
    mapping(uint256 => uint256) public slot_29;
    uint256 public slot_3;
    mapping(uint256 => uint256) public slot_30;
    mapping(uint256 => uint256) public slot_31;
    uint256 public slot_4;
    uint256 public slot_5;
    uint256 public slot_6;
    uint256 public slot_7;
    uint256 public slot_8;
    uint256 public slot_9;

    function public_0x2fe82a92() public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        require(false /* TODO: evm.lt */);
        // block_3:
        return (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_16[0 /* TODO: unresolved value: evm.calldataload.i.load */] & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function public_0x3c14f4d5() public {
        // block_0:
    }

    function public_0x84791215() public {
        // block_0:
    }

    function public_0xb38d5ef4() public {
        // block_0:
        require(msg.data.length > 67);
        // block_2:
    }

    function public_0xb79c1886() public {
        // block_0:
        require(msg.data.length > 67);
        // block_2:
    }

    function public_0xc44a24dd() public returns (bool ret0) {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        return slot_15[0 /* TODO: unresolved value: evm.calldataload.i.load */] != 0;
    }

    function public_0xd86c9fec() public {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_3:
          if (false /* TODO: evm.branch.cond8.not */) {
            // block_5:
            revert(); // error_string, selector=0x8c379a0, string_length=29
          }
          else {
            // block_4:
          }
        }
        else {
          // block_6:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function public_0xe54e9849() public {
        // block_0:
    }

    function public_0xe94ae90f() public returns (bool ret0) {
        // block_0:
        return block.number < slot_14;
    }

    function public_0xeeeeeeee() public {
        // block_0:
    }

    function isExcludedMaxTransactionAmount(address arg0) public returns (bool ret0) {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        private__0x2e39_0x2e39(uint256(uint160(arg0)), 804805);
        return slot_30[uint256(uint160(arg0))] != 0;
    }

    function allowance(address arg0, address arg1) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 67);
        // block_2:
        private__0x2e39_0x2e39(uint256(uint160(arg0)), 12244);
        private__0x2e39_0x2e39(uint256(uint160(arg1)), 804995);
        return slot_1[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0))];
    }

    function approve(address arg0, uint256 arg1) public returns (bool ret0) {
        // block_0:
        require(msg.data.length > 67);
        // block_2:
        private__0x2e39_0x2e39(uint256(uint160(arg0)), 11887);
        return private__0xaa5_0xaa5(arg1, uint256(uint160(arg0)), 452695) != 0;
    }

    function automatedMarketMakerPairs(address arg0) public returns (bool ret0) {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        private__0x2e39_0x2e39(uint256(uint160(arg0)), 804805);
        return slot_31[uint256(uint160(arg0))] != 0;
    }

    function balanceOf(address arg0) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        private__0x2e39_0x2e39(uint256(uint160(arg0)), 804805);
        return private__0xd0b_0xd0b(uint256(uint160(arg0)), 453505);
    }

    function blockForPenaltyEnd() public returns (uint256 ret0) {
        // block_0:
        return slot_14;
    }

    function buyLiquidityFee() public returns (uint256 ret0) {
        // block_0:
        return slot_23;
    }

    function buyOperationsFee() public returns (uint256 ret0) {
        // block_0:
        return slot_22;
    }

    function buyTotalFees() public returns (uint256 ret0) {
        // block_0:
        return slot_21;
    }

    function decimals() public returns (uint256 ret0) {
        // block_0:
        return 18;
    }

    function decreaseAllowance(address arg0, uint256 arg1) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 67);
        // block_2:
        private__0x2e39_0x2e39(uint256(uint160(arg0)), 11887);
        if (slot_1[uint256(uint160(msg.sender))] < arg1) {
          // block_4:
          revert(); // error_string, selector=0x8c379a0, string_length=37
        }
        else {
          // block_3:
          private__0x1d01_0x1d01(slot_1[uint256(uint160(msg.sender))] - arg1, uint256(uint160(arg0)), uint256(uint160(msg.sender)), 5270);
          return 1;
        }
    }

    function dexRouter() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_9 & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function disableTransferDelay() public {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_1:
          slot_20 = slot_20 & uint256((type(uint256).max - 255));
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function enableTrading(uint256 arg0) public {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_3:
          if (slot_18 / uint256(256) & uint256(255) == 0) {
            // block_4:
            if (arg0 < 11) {
              // block_5:
              slot_18 = slot_18 & uint256((type(uint256).max - 16776960)) | uint256(65792);
              slot_13 = block.number;
              slot_14 = private__0x3056_0x3056(block.number, arg0, 4075);
              emit Event_0xA56FEB2D31B9A7424DB0BE063FD450863979C9E2382CF5110F869BD1AD361BB7(); // TODO: recover event signature
            }
            else {
              // block_6:
              revert(); // error_string, selector=0x8c379a0, string_length=39
            }
          }
          else {
            // block_7:
            revert(); // error_string, selector=0x8c379a0, string_length=23
          }
        }
        else {
          // block_8:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function excludeFromFees(address arg0, bool arg1) public {
        // block_0:
        require(msg.data.length > 67);
        // block_2:
        private__0x2e39_0x2e39(uint256(uint160(arg0)), 12158);
    }

    function excludeFromMaxTransaction(address arg0, bool arg1) public {
        // block_0:
        require(msg.data.length > 67);
        if (msg.data.length > 67) {
          // goto block_2
        }
        // goto block_1
        // block_2:
        private__0x2e39_0x2e39(uint256(uint160(arg0)), 12158);
        if (!(uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5)) {
          // block_7:
          revert(); // error_string, selector=0x8c379a0
        }
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // goto block_3
        }
        // goto block_7
        // block_3:
        if (!false /* TODO: evm.branch.cond4.not */) {
          // block_5:
        }
        if (false /* TODO: evm.branch.cond4.not */) {
          // goto block_4
        }
        // goto block_5
        // block_4:
        if (!((uint256(uint160(arg0)) ^ slot_10) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
          // goto block_5
        }
        // block_6:
        revert(); // error_string, selector=0x8c379a0, string_length=39
    }

    function increaseAllowance(address arg0, uint256 arg1) public returns (bool ret0) {
        // block_0:
        require(msg.data.length > 67);
        // block_2:
        private__0x2e39_0x2e39(uint256(uint160(arg0)), 11887);
        return private__0xb95_0xb95(arg1, uint256(uint160(arg0)), 453056) != 0;
    }

    function limitsInEffect() public returns (bool ret0) {
        // block_0:
        return slot_18 != 0;
    }

    function lpPair() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_10 & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function maxBuyAmount() public returns (uint256 ret0) {
        // block_0:
        return slot_6;
    }

    function maxSellAmount() public returns (uint256 ret0) {
        // block_0:
        return slot_7;
    }

    function maxWallet() public returns (uint256 ret0) {
        // block_0:
        return slot_8;
    }

    function name() public {
        // block_0:
    }

    function operationsAddress() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_12 & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function owner() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5 & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function removeLimits() public {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_1:
          slot_18 = slot_18 & uint256((type(uint256).max - 255));
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function renounceOwnership(bool arg0) public {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_3:
          if (false /* TODO: evm.branch.cond3.not */) {
            // block_5:
            revert(); // error_string, selector=0x8c379a0, string_length=24
          }
          else {
            // block_4:
            emit Event_0x8BE0079C531659141344CD1FD0A4F28419497F9722A3DAAFE3B4186F6B6457E0((uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5, 0); // TODO: recover event signature
            slot_5 = slot_5 & uint256(0) - (uint256(1) << uint256(160));
          }
        }
        else {
          // block_6:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function restoreLimits() public {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_1:
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function sellLiquidityFee() public returns (uint256 ret0) {
        // block_0:
        return slot_26;
    }

    function sellOperationsFee() public returns (uint256 ret0) {
        // block_0:
        return slot_25;
    }

    function sellTotalFees() public returns (uint256 ret0) {
        // block_0:
        return slot_24;
    }

    function setAutomatedMarketMakerPair(address arg0, bool arg1) public {
        // block_0:
        require(msg.data.length > 67);
        // block_2:
        private__0x2e39_0x2e39(uint256(uint160(arg0)), 12158);
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_3:
          if ((uint256(uint160(arg0)) ^ slot_10) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
            // block_5:
            revert(); // error_string, selector=0x8c379a0, string_length=57
          }
          else {
            // block_4:
            emit Event_0xFFA9187BF1F18BF477BD0EA1BCBB64E93B6A98132473929EDFCE215CD9B16FAB(uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max), 0 /* TODO: unresolved value: evm.calldataload2.i.load */ != 0); // TODO: recover event signature
          }
        }
        else {
          // block_6:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function setOperationsAddress(address arg0) public {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        private__0x2e39_0x2e39(uint256(uint160(arg0)), 804805);
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_3:
          if (uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
            // block_5:
            revert(); // error_string, selector=0x8c379a0, string_length=38
          }
          else {
            // block_4:
            slot_12 = uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) | slot_12 & uint256(0) - (uint256(1) << uint256(160));
            emit Event_0x4EFA56652237561D0F1FD31311AEAAA41F3B754A461545ED3CF6CED5876D2982(uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)); // TODO: recover event signature
          }
        }
        else {
          // block_6:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function snipersCaught() public returns (uint256 ret0) {
        // block_0:
        return slot_17;
    }

    function swapEnabled() public returns (bool ret0) {
        // block_0:
        return slot_18 / uint256(65536) & uint256(255) != 0;
    }

    function swapTokensAtAmount() public returns (uint256 ret0) {
        // block_0:
        return slot_11;
    }

    function symbol() public {
        // block_0:
    }

    function tokensForLiquidity() public returns (uint256 ret0) {
        // block_0:
        return slot_28;
    }

    function tokensForOperations() public returns (uint256 ret0) {
        // block_0:
        return slot_27;
    }

    function totalSupply() public returns (uint256 ret0) {
        // block_0:
        return slot_2;
    }

    function tradingActiveBlock() public returns (uint256 ret0) {
        // block_0:
        return slot_13;
    }

    function tradingActive() public returns (bool ret0) {
        // block_0:
        return slot_18 / uint256(256) & uint256(255) != 0;
    }

    function transferDelayEnabled() public returns (bool ret0) {
        // block_0:
        return slot_20 != 0;
    }

    function transferForeignToken(address arg0, address arg1) public returns (bool ret0) {
        // block_0:
        require(msg.data.length > 67);
        if (msg.data.length > 67) {
          // goto block_2
        }
        // goto block_1
        // block_2:
        private__0x2e39_0x2e39(uint256(uint160(arg0)), 12244);
        private__0x2e39_0x2e39(uint256(uint160(arg1)), 804995);
        if (!(uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5)) {
          // block_15:
          revert(); // error_string, selector=0x8c379a0
        }
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // goto block_3
        }
        // goto block_15
        // block_3:
        if (uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // block_14:
          revert(); // error_string, selector=0x8c379a0, string_length=26
        }
        if (!(uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
          // goto block_4
        }
        // goto block_14
        // block_4:
        if (!(uint256(uint160(address(this))) == uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max))) {
          // block_6:
          if (false /* TODO: evm.iszero29.not */) {
            // block_12:
            revert(); // returndata_forward
          }
          else {
            // block_7:
            require(false /* TODO: evm.slt */);
            // block_8:
            if (false /* TODO: evm.iszero57.not */) {
              // block_10:
              revert(); // returndata_forward
            }
            else {
              // block_9:
              emit Event_0xDEDA980967FCEAD7B61E78AC46A4DA14274AF29E894D4D61E8B81EC38AB3E438(); // TODO: recover event signature
              return 0 /* TODO: unresolved value: private__0x30b7_0x30b7 */ != 0;
            }
          }
        }
        if (uint256(uint160(address(this))) == uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)) {
          // goto block_5
        }
        // goto block_6
        // block_5:
        if (slot_18 / uint256(256) & uint256(255) == 0) {
          // goto block_6
        }
        // block_13:
        revert(); // error_string, selector=0x8c379a0, string_length=52
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
        // goto block_89
        // block_1:
        private__0x2e39_0x2e39(uint256(uint160(arg0)), 11969);
        private__0x2e39_0x2e39(uint256(uint160(arg1)), 11985);
        if (!(uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
          // goto block_2
        }
        // goto block_85
        // block_2:
        if (!(uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
          // goto block_3
        }
        // goto block_84
        // block_3:
        if (!(arg2 == 0)) {
          // goto block_4
        }
        // goto block_83
        // block_4:
        if (!(slot_18 / uint256(256) & uint256(255) == 0)) {
          // goto block_7
        }
        // block_5:
        if (!(slot_29[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_7
        }
        // block_6:
        if (!(slot_29[uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_7
        }
        // goto block_82
        // block_7:
        if (block.number < slot_14) {
          // goto block_12
        }
        // block_8:
        if (slot_18 / uint256(256) & uint256(255) == 0) {
          // goto block_12
        }
        // block_9:
        if (slot_15[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_12
        }
        // block_10:
        if ((slot_5 ^ uint256(uint160(arg1))) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // goto block_12
        }
        // block_11:
        if (uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 57005) {
          // goto block_12
        }
        // goto block_81
        // block_12:
        if (slot_18 == 0) {
          // goto block_35
        }
        // block_13:
        if ((slot_5 ^ uint256(uint160(arg0))) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // goto block_35
        }
        // block_14:
        if ((slot_5 ^ uint256(uint160(arg1))) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // goto block_35
        }
        // block_15:
        if (uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 57005) {
          // goto block_35
        }
        // block_16:
        if (!(slot_29[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_35
        }
        // block_17:
        if (!(slot_29[uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_35
        }
        // block_18:
        if (slot_20 == 0) {
          // goto block_24
        }
        // block_19:
        if ((slot_9 ^ uint256(uint160(arg1))) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // goto block_24
        }
        // block_20:
        if ((slot_10 ^ uint256(uint160(arg1))) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // goto block_24
        }
        // block_21:
        if (!(slot_19[uint256(uint160(tx.origin))] < private__0x31e2_0x31e2(block.number, 2, 8481))) {
          // block_80:
          revert(); // error_string, selector=0x8c379a0, string_length=53
        }
        if (slot_19[uint256(uint160(tx.origin))] < private__0x31e2_0x31e2(block.number, 2, 8481)) {
          // goto block_22
        }
        // goto block_80
        // block_22:
        if (!(slot_19[uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] < private__0x31e2_0x31e2(block.number, 2, 8516))) {
          // goto block_80
        }
        // block_23:
        // block_24:
        if (slot_31[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_26
        }
        // block_25:
        if (!(slot_30[uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_26
        }
        // goto block_33
        // block_26:
        if (!(slot_31[uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_27
        }
        // goto block_28
        // block_27:
        if (!(slot_30[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_28
        }
        // goto block_31
        // block_28:
        if (!(slot_30[uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_35
        }
        // block_29:
        if (!(private__0x3056_0x3056(arg2, private__0xd0b_0xd0b(uint256(uint160(arg1)), 9165), 9175) > slot_8)) {
          // goto block_35
        }
        // block_30:
        require(false, "Max Wallet Exceeded");
        // block_31:
        if (!(arg2 > slot_7)) {
          // goto block_35
        }
        // block_32:
        revert(); // error_string, selector=0x8c379a0, string_length=42
        // block_33:
        if (arg2 > slot_6) {
          // block_79:
          revert(); // error_string, selector=0x8c379a0, string_length=40
        }
        if (!(arg2 > slot_6)) {
          // goto block_34
        }
        // goto block_79
        // block_34:
        if (!(private__0x3056_0x3056(arg2, private__0xd0b_0xd0b(uint256(uint160(arg1)), 8867), 8877) > slot_8)) {
          // goto block_35
        }
        // goto block_78
        // block_35:
        if (private__0xd0b_0xd0b(uint256(uint160(address(this))), 9254) < slot_11) {
          // goto block_53
        }
        // block_36:
        if (slot_18 / uint256(65536) & uint256(255) == 0) {
          // goto block_53
        }
        // block_37:
        if (!(slot_10 / (uint256(1) << uint256(160)) & uint256(255) == 0)) {
          // goto block_53
        }
        // block_38:
        if (slot_31[uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_53
        }
        // block_39:
        slot_10 = uint256(1) << uint256(160) | slot_10 & ~(uint256(255) << uint256(160));
        temp_0 = private__0xd0b_0xd0b(uint256(uint160(address(this))), 10564);
        temp_1 = private__0x3056_0x3056(slot_28, slot_27, 10584);
        if (false /* TODO: spec.select */) {
          // goto block_52
        }
        // goto block_40
        // block_40:
        if (temp_0 > private__0x31f5_0x31f5(15, slot_11, 10620)) {
          // block_41:
        }
        // block_42:
        if (false /* TODO: evm.iszero.i.i.i.not */) {
          // goto block_47
        }
        // block_43:
        require(!false /* TODO: evm.iszero53.i.i.i.not */);
        if (!false /* TODO: evm.iszero53.i.i.i.not */) {
          // goto block_44
        }
        // goto block_46
        // block_44:
        if (false /* TODO: evm.iszero59.i.i.i.not */) {
          // block_45:
          revert(); // returndata_forward
        }
        if (false /* TODO: evm.iszero59.i.i.i.not */) {
          // goto block_45
        }
        else {
          // goto block_48
        }
        // block_47:
        revert(); // returndata_forward
        // block_48:
        temp_2 = private__0x31e2_0x31e2(address(this).balance, private__0x320c_0x320c(private__0x31f5_0x31f5(address(this).balance, slot_27, 10749), private__0x31e2_0x31e2(temp_1, private__0x320c_0x320c(slot_28, 2, 10726), 10736), 10759), 10771);
        slot_28 = 0;
        slot_27 = 0;
        if (false /* TODO: spec.select1 */) {
          // block_49:
        }
        // block_50:
        if (false /* TODO: evm.eq.i.i */) {
          // goto block_52
        }
        // block_51:
        // block_52:
        slot_10 = slot_10 & ~(uint256(255) << uint256(160));
        // block_53:
        if (!(slot_29[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // block_86:
          if (slot_1[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] < arg2) {
            // block_88:
            revert(); // error_string, selector=0x8c379a0, string_length=40
          }
          else {
            // block_87:
            private__0x1d01_0x1d01(slot_1[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] - arg2, uint256(uint160(msg.sender)), uint256(uint160(arg0)), 2912);
            return 1;
          }
        }
        if (slot_29[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_54
        }
        // goto block_86
        // block_54:
        if (!(slot_29[uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_86
        }
        // goto block_55
        // block_55:
        if (!(block.number < slot_14)) {
          // block_56:
          if (!(arg2 < private__0x31e2_0x31e2(slot_6, 900000000000000000, 9505))) {
            // block_57:
          }
        }
        // block_58:
        if (!false /* TODO: _0x253e_0x0.i.in.reload */) {
          // goto block_60
        }
        // block_59:
        // block_60:
        if (false /* TODO: _0x2562_0x0.i.reload */) {
          // goto block_64
        }
        // block_61:
        if (!(slot_31[uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_64
        }
        // block_62:
        if (!(slot_29[uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_64
        }
        // block_63:
        if (slot_21 == 0) {
          // goto block_64
        }
        // goto block_70
        // block_64:
        if (slot_31[uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_66
        }
        // block_65:
        if (slot_24 == 0) {
          // goto block_66
        }
        // goto block_69
        // block_66:
        if (!(slot_31[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // block_67:
          if (!(slot_21 == 0)) {
            // block_68:
            temp_3 = private__0x320c_0x320c(private__0x31f5_0x31f5(arg2, slot_21, 10252), 100, 10262);
            slot_28 = private__0x3056_0x3056(slot_28, private__0x320c_0x320c(private__0x31f5_0x31f5(temp_3, slot_23, 10281), slot_21, 10291), 10308);
            slot_27 = private__0x3056_0x3056(slot_27, private__0x320c_0x320c(private__0x31f5_0x31f5(temp_3, slot_22, 10329), slot_21, 10339), 10356);
          }
        }
        // goto block_75
        // block_69:
        temp_4 = private__0x320c_0x320c(private__0x31f5_0x31f5(arg2, slot_24, 10112), 100, 10122);
        slot_28 = private__0x3056_0x3056(slot_28, private__0x320c_0x320c(private__0x31f5_0x31f5(temp_4, slot_26, 10141), slot_24, 10151), 10168);
        slot_27 = private__0x3056_0x3056(slot_27, private__0x320c_0x320c(private__0x31f5_0x31f5(temp_4, slot_25, 804516), slot_24, 808327), 809337);
        // goto block_75
        // block_70:
        if (!(block.number < slot_14)) {
          // block_71:
          slot_6 = private__0x31e2_0x31e2(slot_6, 1, 9691);
        }
        // block_72:
        if (!(slot_15[uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_74
        }
        // block_73:
        slot_17 = private__0x3056_0x3056(slot_17, 1, 9786);
        emit Event_0x55678E47D0A699D3AB99B0184C4FF14F2246BA80522DEB921AA0C8823578C44A(); // TODO: recover event signature
        // block_74:
        temp_5 = private__0x320c_0x320c(private__0x31f5_0x31f5(99, arg2, 9934), 100, 9944);
        slot_28 = private__0x3056_0x3056(slot_28, private__0x320c_0x320c(private__0x31f5_0x31f5(temp_5, slot_23, 9963), slot_21, 9973), 9990);
        slot_27 = private__0x3056_0x3056(slot_27, private__0x320c_0x320c(private__0x31f5_0x31f5(temp_5, slot_22, 804385), slot_21, 808237), 809295);
        // goto block_75
        // block_75:
        if (!false /* TODO: evm.iszero490.i */) {
          // block_76:
        }
        // block_77:
        // goto block_86
        // block_78:
        require(false, "Max Wallet Exceeded");
        // block_81:
        revert(); // error_string, selector=0x8c379a0, string_length=70
        // block_82:
        require(false, "Trading is not active.");
        // block_83:
        revert(); // error_string, selector=0x8c379a0, string_length=29
        // block_84:
        revert(); // error_string, selector=0x8c379a0
        // block_85:
        revert(); // error_string, selector=0x8c379a0
        // block_89:
        revert(); // empty
    }

    function transferOwnership(address arg0) public {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        private__0x2e39_0x2e39(uint256(uint160(arg0)), 804805);
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_3:
          if (uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
            // block_5:
            revert(); // error_string, selector=0x8c379a0, string_length=38
          }
          else {
            // block_4:
            emit Event_0x8BE0079C531659141344CD1FD0A4F28419497F9722A3DAAFE3B4186F6B6457E0(slot_5 & (uint256(1) << uint256(160)) + uint256(type(uint256).max), uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)); // TODO: recover event signature
            slot_5 = (uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0)) | slot_5 & uint256(0) - (uint256(1) << uint256(160));
          }
        }
        else {
          // block_6:
          revert(); // error_string, selector=0x8c379a0
        }
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
        private__0x2e39_0x2e39(uint256(uint160(arg0)), 11887);
        if (!(uint256(uint160(msg.sender)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
          // goto block_3
        }
        // goto block_86
        // block_3:
        if (!(uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
          // goto block_4
        }
        // goto block_85
        // block_4:
        if (!(arg1 == 0)) {
          // goto block_5
        }
        // goto block_84
        // block_5:
        if (!(slot_18 / uint256(256) & uint256(255) == 0)) {
          // goto block_8
        }
        // block_6:
        if (!(slot_29[uint256(uint160(msg.sender)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_8
        }
        // block_7:
        if (!(slot_29[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_8
        }
        // goto block_83
        // block_8:
        if (block.number < slot_14) {
          // goto block_13
        }
        // block_9:
        if (slot_18 / uint256(256) & uint256(255) == 0) {
          // goto block_13
        }
        // block_10:
        if (slot_15[uint256(uint160(msg.sender)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_13
        }
        // block_11:
        if ((slot_5 ^ uint256(uint160(arg0))) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // goto block_13
        }
        // block_12:
        if (uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 57005) {
          // goto block_13
        }
        // goto block_82
        // block_13:
        if (!(slot_18 == 0)) {
          // goto block_14
        }
        // goto block_36
        // block_14:
        if ((slot_5 ^ uint256(uint160(msg.sender))) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // goto block_36
        }
        // block_15:
        if ((slot_5 ^ uint256(uint160(arg0))) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // goto block_36
        }
        // block_16:
        if (uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 57005) {
          // goto block_36
        }
        // block_17:
        if (!(slot_29[uint256(uint160(msg.sender)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_36
        }
        // block_18:
        if (!(slot_29[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_36
        }
        // block_19:
        if (slot_20 == 0) {
          // goto block_25
        }
        // block_20:
        if ((slot_9 ^ uint256(uint160(arg0))) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // goto block_25
        }
        // block_21:
        if ((slot_10 ^ uint256(uint160(arg0))) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // goto block_25
        }
        // block_22:
        if (!(slot_19[uint256(uint160(tx.origin))] < private__0x31e2_0x31e2(block.number, 2, 8481))) {
          // block_81:
          revert(); // error_string, selector=0x8c379a0, string_length=53
        }
        if (slot_19[uint256(uint160(tx.origin))] < private__0x31e2_0x31e2(block.number, 2, 8481)) {
          // goto block_23
        }
        // goto block_81
        // block_23:
        if (!(slot_19[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] < private__0x31e2_0x31e2(block.number, 2, 8516))) {
          // goto block_81
        }
        // block_24:
        // block_25:
        if (slot_31[uint256(uint160(msg.sender)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_27
        }
        // block_26:
        if (!(slot_30[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_27
        }
        // goto block_34
        // block_27:
        if (!(slot_31[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_28
        }
        // goto block_29
        // block_28:
        if (!(slot_30[uint256(uint160(msg.sender)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_29
        }
        // goto block_32
        // block_29:
        if (!(slot_30[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_36
        }
        // block_30:
        if (!(private__0x3056_0x3056(arg1, private__0xd0b_0xd0b(uint256(uint160(arg0)), 9165), 9175) > slot_8)) {
          // goto block_36
        }
        // block_31:
        require(false, "Max Wallet Exceeded");
        // block_32:
        if (!(arg1 > slot_7)) {
          // goto block_36
        }
        // block_33:
        revert(); // error_string, selector=0x8c379a0, string_length=42
        // block_34:
        if (arg1 > slot_6) {
          // block_80:
          revert(); // error_string, selector=0x8c379a0, string_length=40
        }
        if (!(arg1 > slot_6)) {
          // goto block_35
        }
        // goto block_80
        // block_35:
        if (!(private__0x3056_0x3056(arg1, private__0xd0b_0xd0b(uint256(uint160(arg0)), 8867), 8877) > slot_8)) {
          // goto block_36
        }
        // goto block_79
        // block_36:
        if (private__0xd0b_0xd0b(uint256(uint160(address(this))), 9254) < slot_11) {
          // goto block_54
        }
        // block_37:
        if (slot_18 / uint256(65536) & uint256(255) == 0) {
          // goto block_54
        }
        // block_38:
        if (!(slot_10 / (uint256(1) << uint256(160)) & uint256(255) == 0)) {
          // goto block_54
        }
        // block_39:
        if (slot_31[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_54
        }
        // block_40:
        slot_10 = uint256(1) << uint256(160) | slot_10 & ~(uint256(255) << uint256(160));
        temp_0 = private__0xd0b_0xd0b(uint256(uint160(address(this))), 10564);
        temp_1 = private__0x3056_0x3056(slot_28, slot_27, 10584);
        if (false /* TODO: spec.select */) {
          // goto block_53
        }
        // block_41:
        if (temp_0 > private__0x31f5_0x31f5(15, slot_11, 10620)) {
          // block_42:
        }
        // block_43:
        if (false /* TODO: evm.iszero.i.i.i.i.not */) {
          // goto block_48
        }
        // block_44:
        require(!false /* TODO: evm.iszero53.i.i.i.i.not */);
        if (!false /* TODO: evm.iszero53.i.i.i.i.not */) {
          // goto block_45
        }
        // goto block_47
        // block_45:
        if (false /* TODO: evm.iszero59.i.i.i.i.not */) {
          // block_46:
          revert(); // returndata_forward
        }
        if (false /* TODO: evm.iszero59.i.i.i.i.not */) {
          // goto block_46
        }
        else {
          // goto block_49
        }
        // block_48:
        revert(); // returndata_forward
        // block_49:
        temp_2 = private__0x31e2_0x31e2(address(this).balance, private__0x320c_0x320c(private__0x31f5_0x31f5(address(this).balance, slot_27, 10749), private__0x31e2_0x31e2(temp_1, private__0x320c_0x320c(slot_28, 2, 10726), 10736), 10759), 10771);
        slot_28 = 0;
        slot_27 = 0;
        if (false /* TODO: spec.select1 */) {
          // block_50:
        }
        // block_51:
        if (false /* TODO: evm.eq.i.i.i */) {
          // goto block_53
        }
        // block_52:
        // block_53:
        slot_10 = slot_10 & ~(uint256(255) << uint256(160));
        // block_54:
        if (!(slot_29[uint256(uint160(msg.sender)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // block_87:
          return 1;
        }
        if (slot_29[uint256(uint160(msg.sender)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_55
        }
        // goto block_87
        // block_55:
        if (!(slot_29[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_87
        }
        // block_56:
        if (!(block.number < slot_14)) {
          // block_57:
          if (!(arg1 < private__0x31e2_0x31e2(slot_6, 900000000000000000, 9505))) {
            // block_58:
          }
        }
        // block_59:
        if (!false /* TODO: _0x253e_0x0.i.i.in.reload */) {
          // goto block_61
        }
        // block_60:
        // block_61:
        if (false /* TODO: _0x2562_0x0.i.i.reload */) {
          // goto block_65
        }
        // block_62:
        if (!(slot_31[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_65
        }
        // block_63:
        if (!(slot_29[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_65
        }
        // block_64:
        if (slot_21 == 0) {
          // goto block_65
        }
        // goto block_71
        // block_65:
        if (slot_31[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_67
        }
        // block_66:
        if (slot_24 == 0) {
          // goto block_67
        }
        // goto block_70
        // block_67:
        if (!(slot_31[uint256(uint160(msg.sender)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // block_68:
          if (!(slot_21 == 0)) {
            // block_69:
            temp_3 = private__0x320c_0x320c(private__0x31f5_0x31f5(arg1, slot_21, 10252), 100, 10262);
            slot_28 = private__0x3056_0x3056(slot_28, private__0x320c_0x320c(private__0x31f5_0x31f5(temp_3, slot_23, 10281), slot_21, 10291), 10308);
            slot_27 = private__0x3056_0x3056(slot_27, private__0x320c_0x320c(private__0x31f5_0x31f5(temp_3, slot_22, 10329), slot_21, 10339), 10356);
          }
        }
        // goto block_76
        // block_70:
        temp_4 = private__0x320c_0x320c(private__0x31f5_0x31f5(arg1, slot_24, 10112), 100, 10122);
        slot_28 = private__0x3056_0x3056(slot_28, private__0x320c_0x320c(private__0x31f5_0x31f5(temp_4, slot_26, 10141), slot_24, 10151), 10168);
        slot_27 = private__0x3056_0x3056(slot_27, private__0x320c_0x320c(private__0x31f5_0x31f5(temp_4, slot_25, 804516), slot_24, 808327), 809337);
        // goto block_76
        // block_71:
        if (!(block.number < slot_14)) {
          // block_72:
          slot_6 = private__0x31e2_0x31e2(slot_6, 1, 9691);
        }
        // block_73:
        if (!(slot_15[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_75
        }
        // block_74:
        slot_17 = private__0x3056_0x3056(slot_17, 1, 9786);
        emit Event_0x55678E47D0A699D3AB99B0184C4FF14F2246BA80522DEB921AA0C8823578C44A(); // TODO: recover event signature
        // block_75:
        temp_5 = private__0x320c_0x320c(private__0x31f5_0x31f5(99, arg1, 9934), 100, 9944);
        slot_28 = private__0x3056_0x3056(slot_28, private__0x320c_0x320c(private__0x31f5_0x31f5(temp_5, slot_23, 9963), slot_21, 9973), 9990);
        slot_27 = private__0x3056_0x3056(slot_27, private__0x320c_0x320c(private__0x31f5_0x31f5(temp_5, slot_22, 804385), slot_21, 808237), 809295);
        // block_76:
        if (!false /* TODO: evm.iszero490.i.i */) {
          // block_77:
        }
        // block_78:
        // goto block_87
        // block_79:
        require(false, "Max Wallet Exceeded");
        // block_82:
        revert(); // error_string, selector=0x8c379a0, string_length=70
        // block_83:
        require(false, "Trading is not active.");
        // block_84:
        revert(); // error_string, selector=0x8c379a0, string_length=29
        // block_85:
        revert(); // error_string, selector=0x8c379a0
        // block_86:
        revert(); // error_string, selector=0x8c379a0
    }

    function private__0x14ad_0x14ad(uint256 arg0, uint256 arg1, uint256 arg2) internal {
        // block_0:
        uint256 temp_0;
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_1:
          slot_22 = arg1;
          slot_23 = arg0;
          temp_0 = private__0x3056_0x3056(arg1, arg0, 5355);
          slot_21 = temp_0;
          if (temp_0 < 16) {
            // block_2:
          }
          else {
            // block_3:
            revert(); // error_string, selector=0x8c379a0, string_length=29
          }
        }
        else {
          // block_4:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function private__0x1544_0x1544(uint256 arg0, uint256 arg1, uint256 arg2) internal {
        // block_0:
        uint256 temp_0;
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_1:
          slot_25 = arg1;
          slot_26 = arg0;
          temp_0 = private__0x3056_0x3056(arg1, arg0, 5506);
          slot_24 = temp_0;
          if (temp_0 < 21) {
            // block_2:
          }
          else {
            // block_3:
            revert(); // error_string, selector=0x8c379a0, string_length=29
          }
        }
        else {
          // block_4:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function private__0x15d7_0x15d7(uint256 arg0, uint256 arg1, uint256 arg2) internal {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_1:
          emit Event_0x9D8F7706EA1113D1A167B526ECA956215946DD36CC7DF39EB16180222D8B5DF7(arg1 & (uint256(1) << uint256(160)) + uint256(type(uint256).max)); // TODO: recover event signature
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function private__0x1d01_0x1d01(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
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

    function private__0x31f5_0x31f5(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg1 * arg0;
    }

    function private__0x320c_0x320c(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 / arg1;
    }

    function private__0x28ac_0x28ac(uint256 arg0, uint256 arg1, uint256 arg2) internal {
        // block_0:
        private__0x28d6_0x28d6(arg0, arg1, 804557);
        emit Event_0xFFA9187BF1F18BF477BD0EA1BCBB64E93B6A98132473929EDFCE215CD9B16FAB(arg1 & (uint256(1) << uint256(160)) + uint256(type(uint256).max), arg0 != 0); // TODO: recover event signature
    }

    function private__0x28d6_0x28d6(uint256 arg0, uint256 arg1, uint256 arg2) internal {
        // block_0:
        emit Event_0x6B4F1BE9103E6CBCD38CA4A922334F2C3109B260130A6676A987F94088FD6746(); // TODO: recover event signature
    }

    function private__0x2a9b_0x2a9b(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
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

    function private__0x2e39_0x2e39(uint256 arg0, uint256 arg1) internal {
        // block_0:
        require(arg0 & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_1:
    }

    function private__0x2f52_0x2f52(uint256 arg0, uint256 arg1) internal {
        // block_0:
        require(arg0 < 2);
        // block_1:
    }

    function private__0x3056_0x3056(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg1 + arg0;
    }

    function private__0x3069_0x3069(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(96);
    }

    function private__0x30f1_0x30f1(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3, uint256 arg4, uint256 arg5, uint256 arg6, uint256 arg7) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(192);
    }

    function private__0x315a_0x315a(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(128);
    }

    function private__0x319f_0x319f(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(128);
    }

    function private__0x31e2_0x31e2(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 - arg1;
    }

    function private__0xaa5_0xaa5(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        private__0x1d01_0x1d01(arg0, arg1, uint256(uint160(msg.sender)), 455194);
        return 1;
    }

    function private__0xb95_0xb95(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        private__0x1d01_0x1d01(private__0x3056_0x3056(slot_1[uint256(uint160(msg.sender))], arg0, 687226), arg1, uint256(uint160(msg.sender)), 571210);
        return 1;
    }

    function private__0xd0b_0xd0b(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return slot_0[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & arg0];
    }
}
