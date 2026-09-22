contract Decompiled {
    event Event_0x9D8F7706EA1113D1A167B526ECA956215946DD36CC7DF39EB16180222D8B5DF7(uint256 indexed arg0);
    event Event_0x8C5BE1E5EBEC7D5BD14F71427D1E84F3DD0314C0F7B2291E5B200AC8C7C3B925(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0x55678E47D0A699D3AB99B0184C4FF14F2246BA80522DEB921AA0C8823578C44A();
    event Event_0xFFA9187BF1F18BF477BD0EA1BCBB64E93B6A98132473929EDFCE215CD9B16FAB(uint256 indexed arg0, bool indexed arg1);
    event Event_0x6B4F1BE9103E6CBCD38CA4A922334F2C3109B260130A6676A987F94088FD6746();
    event Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF(uint256 indexed arg0, uint256 indexed arg1);
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
        require(false /* TODO: evm.lt */);
        // block_1:
        return (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_16[0 /* TODO: unresolved value: abi_decode_word_from_calldata__0x2ee2 */] & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function public_0x3c14f4d5() public {
        // block_0:
    }

    function public_0x84791215() public {
        // block_0:
    }

    function public_0xb38d5ef4() public {
        // block_0:
    }

    function public_0xb79c1886() public {
        // block_0:
    }

    function public_0xc44a24dd() public returns (bool ret0) {
        // block_0:
        return slot_15[0 /* TODO: unresolved value: private__0x2e7d_0x2e7d */] != 0;
    }

    function public_0xd86c9fec() public {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_1:
          if (false /* TODO: evm.branch.cond8.not */) {
            // block_3:
            revert(); // error_string, selector=0x8c379a0, string_length=29
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
        return slot_30[uint256(uint160(arg0))] != 0;
    }

    function allowance(address arg0, address arg1) public returns (uint256 ret0) {
        // block_0:
        return slot_1[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0))];
    }

    function approve(address arg0, uint256 arg1) public returns (bool ret0) {
        // block_0:
        return private__0xaa5_0xaa5(arg1, uint256(uint160(arg0)), 452695) != 0;
    }

    function automatedMarketMakerPairs(address arg0) public returns (bool ret0) {
        // block_0:
        return slot_31[uint256(uint160(arg0))] != 0;
    }

    function balanceOf(address arg0) public returns (uint256 ret0) {
        // block_0:
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
        if (slot_1[uint256(uint160(msg.sender))] < arg1) {
          // block_2:
          revert(); // error_string, selector=0x8c379a0, string_length=37
        }
        else {
          // block_1:
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
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_1:
          if (slot_18 / uint256(256) & uint256(255) == 0) {
            // block_2:
            if (arg0 < 11) {
              // block_3:
              slot_18 = slot_18 & uint256((type(uint256).max - 16776960)) | uint256(65792);
              slot_13 = block.number;
              slot_14 = private__0x3056_0x3056(block.number, arg0, 4075);
              emit Event_0xA56FEB2D31B9A7424DB0BE063FD450863979C9E2382CF5110F869BD1AD361BB7(); // TODO: recover event signature
            }
            else {
              // block_4:
              revert(); // error_string, selector=0x8c379a0, string_length=39
            }
          }
          else {
            // block_5:
            revert(); // error_string, selector=0x8c379a0, string_length=23
          }
        }
        else {
          // block_6:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function excludeFromFees(address arg0, bool arg1) public {
        // block_0:
    }

    function excludeFromMaxTransaction(address arg0, bool arg1) public {
        // block_0:
        if (!(uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5)) {
          // block_5:
          revert(); // error_string, selector=0x8c379a0
        }
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // goto block_1
        }
        // goto block_5
        // block_1:
        if (!false /* TODO: evm.branch.cond4.not */) {
          // block_3:
        }
        if (false /* TODO: evm.branch.cond4.not */) {
          // goto block_2
        }
        // goto block_3
        // block_2:
        if (!((uint256(uint160(arg0)) ^ slot_10) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
          // goto block_3
        }
        // block_4:
        revert(); // error_string, selector=0x8c379a0, string_length=39
    }

    function increaseAllowance(address arg0, uint256 arg1) public returns (bool ret0) {
        // block_0:
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
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_1:
          if (false /* TODO: evm.branch.cond3.not */) {
            // block_3:
            revert(); // error_string, selector=0x8c379a0, string_length=24
          }
          else {
            // block_2:
            emit Event_0x8BE0079C531659141344CD1FD0A4F28419497F9722A3DAAFE3B4186F6B6457E0((uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5, 0); // TODO: recover event signature
            slot_5 = slot_5 & uint256(0) - (uint256(1) << uint256(160));
          }
        }
        else {
          // block_4:
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
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_1:
          if ((uint256(uint160(arg0)) ^ slot_10) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
            // block_3:
            revert(); // error_string, selector=0x8c379a0, string_length=57
          }
          else {
            // block_2:
            emit Event_0xFFA9187BF1F18BF477BD0EA1BCBB64E93B6A98132473929EDFCE215CD9B16FAB(uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max), 0 /* TODO: unresolved value: private.ret */ != 0); // TODO: recover event signature
          }
        }
        else {
          // block_4:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function setOperationsAddress(address arg0) public {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_1:
          if (uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
            // block_3:
            revert(); // error_string, selector=0x8c379a0, string_length=38
          }
          else {
            // block_2:
            slot_12 = uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) | slot_12 & uint256(0) - (uint256(1) << uint256(160));
            emit Event_0x4EFA56652237561D0F1FD31311AEAAA41F3B754A461545ED3CF6CED5876D2982(uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)); // TODO: recover event signature
          }
        }
        else {
          // block_4:
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
        if (!(uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5)) {
          // block_13:
          revert(); // error_string, selector=0x8c379a0
        }
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // goto block_1
        }
        // goto block_13
        // block_1:
        if (uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // block_12:
          revert(); // error_string, selector=0x8c379a0, string_length=26
        }
        if (!(uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
          // goto block_2
        }
        // goto block_12
        // block_2:
        if (!(uint256(uint160(address(this))) == uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max))) {
          // block_4:
          if (false /* TODO: evm.iszero29.not */) {
            // block_10:
            revert(); // returndata_forward
          }
          else {
            // block_5:
            require(false /* TODO: evm.slt */);
            // block_6:
            if (false /* TODO: evm.iszero57.not */) {
              // block_8:
              revert(); // returndata_forward
            }
            else {
              // block_7:
              emit Event_0xDEDA980967FCEAD7B61E78AC46A4DA14274AF29E894D4D61E8B81EC38AB3E438(); // TODO: recover event signature
              return 0 /* TODO: unresolved value: private__0x30b7_0x30b7 */ != 0;
            }
          }
        }
        if (uint256(uint160(address(this))) == uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)) {
          // goto block_3
        }
        // goto block_4
        // block_3:
        if (slot_18 / uint256(256) & uint256(255) == 0) {
          // goto block_4
        }
        // block_11:
        revert(); // error_string, selector=0x8c379a0, string_length=52
    }

    function transferFrom(address arg0, address arg1, uint256 arg2) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 99);
        // block_1:
        private__0x2e39_0x2e39(uint256(uint160(arg0)), 11969);
        private__0x2e39_0x2e39(uint256(uint160(arg1)), 11985);
        if (slot_1[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] < arg2) {
          // block_3:
          revert(); // error_string, selector=0x8c379a0, string_length=40
        }
        else {
          // block_2:
          private__0x1d01_0x1d01(slot_1[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] - arg2, uint256(uint160(msg.sender)), uint256(uint160(arg0)), 2912);
          return 1;
        }
    }

    function transferOwnership(address arg0) public {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_1:
          if (uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
            // block_3:
            revert(); // error_string, selector=0x8c379a0, string_length=38
          }
          else {
            // block_2:
            emit Event_0x8BE0079C531659141344CD1FD0A4F28419497F9722A3DAAFE3B4186F6B6457E0(slot_5 & (uint256(1) << uint256(160)) + uint256(type(uint256).max), uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)); // TODO: recover event signature
            slot_5 = (uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0)) | slot_5 & uint256(0) - (uint256(1) << uint256(160));
          }
        }
        else {
          // block_4:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function transfer(address arg0, uint256 arg1) public returns (bool ret0) {
        // block_0:
        return private__0x14a0_0x14a0(arg1, uint256(uint160(arg0)), 454064) != 0;
    }

    function private__0x14a0_0x14a0(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return 1;
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
