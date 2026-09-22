contract Decompiled {
    event Event_0x8BE0079C531659141344CD1FD0A4F28419497F9722A3DAAFE3B4186F6B6457E0(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0x90B8024C4923D3873FF5B9FCB43D0360D4B9217FA41225D07BA379993552E743(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0x9D8F7706EA1113D1A167B526ECA956215946DD36CC7DF39EB16180222D8B5DF7(uint256 indexed arg0);
    event Event_0x8C5BE1E5EBEC7D5BD14F71427D1E84F3DD0314C0F7B2291E5B200AC8C7C3B925(uint256 indexed arg0, uint256 indexed arg1);

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
    mapping(uint256 => uint256) public slot_18;
    mapping(uint256 => uint256) public slot_19;
    uint256 public slot_2;
    uint256 public slot_3;
    uint256 public slot_4;
    uint256 public slot_5;
    uint256 public slot_6;
    uint256 public slot_7;
    uint256 public slot_8;
    uint256 public slot_9;

    function USDC() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_6 & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function public_0x3e063cf7() public {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_3:
          if (false /* TODO: evm.lt.not */) {
            // block_5:
            revert(); // error_string, selector=0x8c379a0, string_length=36
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

    function public_0xbe2172bd() public {
        // block_0:
        require(msg.data.length > 67);
        // block_2:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_3:
        }
        else {
          // block_4:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function public_0xeeeeeeee() public {
        // block_0:
    }

    function public_0xf726de1b() public {
        // block_0:
        require(msg.data.length > 67);
        // block_2:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_3:
        }
        else {
          // block_4:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function isExcludedMaxTransactionAmount(address arg0) public returns (bool ret0) {
        // block_0:
        require(msg.data.length > 35);
        // block_1:
        require(uint256(uint160(arg0)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_4:
        return slot_19[uint256(uint160(arg0))] != 0;
    }

    function allowance(address arg0, address arg1) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 67);
        // block_1:
        require(uint256(uint160(arg0)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_3:
        require(uint256(uint160(arg1)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_6:
        return slot_1[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0))];
    }

    function approve(address arg0, uint256 arg1) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 67);
        // block_1:
        require(uint256(uint160(arg0)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_4:
        private__0xfc1_0xfc1(arg1, uint256(uint160(arg0)), uint256(uint160(msg.sender)), 287763);
        return 1;
    }

    function balanceOf(address arg0) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 35);
        // block_1:
        require(uint256(uint160(arg0)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_4:
        return slot_0[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0))];
    }

    function buyDevFee() public returns (uint256 ret0) {
        // block_0:
        return slot_13;
    }

    function buyLiquidityFee() public returns (uint256 ret0) {
        // block_0:
        return slot_14;
    }

    function buyTotalFees() public returns (uint256 ret0) {
        // block_0:
        return slot_12;
    }

    function deadAddress() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(65535) & uint256(57005);
    }

    function decimals() public returns (uint256 ret0) {
        // block_0:
        return 18;
    }

    function devWallet() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_7 & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function excludeFromFees(address arg0, bool arg1) public {
        // block_0:
        require(msg.data.length > 67);
        // block_1:
        require(uint256(uint160(arg0)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_3:
        require(false /* TODO: evm.eq.i3.i */);
        // block_6:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_7:
          emit Event_0x9D8F7706EA1113D1A167B526ECA956215946DD36CC7DF39EB16180222D8B5DF7(uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)); // TODO: recover event signature
        }
        else {
          // block_8:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function excludeFromMaxTransaction(address arg0, bool arg1) public {
        // block_0:
        require(msg.data.length > 67);
        // block_1:
        require(uint256(uint160(arg0)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_3:
        require(false /* TODO: evm.eq.i3.i */);
        // block_6:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_7:
        }
        else {
          // block_8:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function isExcludedFromFees(address arg0) public returns (bool ret0) {
        // block_0:
        require(msg.data.length > 35);
        // block_1:
        require(uint256(uint160(arg0)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_4:
        return slot_18[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0))] != 0;
    }

    function limitsInEffect() public returns (bool ret0) {
        // block_0:
        return slot_11 != 0;
    }

    function maxTransactionAmount() public returns (uint256 ret0) {
        // block_0:
        return slot_8;
    }

    function maxTxnAmount(uint256 arg0) public {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_3:
          if (arg0 < private__0x1cca_0x1cca(private__0x1cca_0x1cca(private__0x1cab_0x1cab(1, slot_2, 2754), 1000, 2764), 1000000000000000000, 2774)) {
            // block_5:
            revert(); // error_string, selector=0x8c379a0, string_length=47
          }
          else {
            // block_4:
            slot_8 = private__0x1cab_0x1cab(1000000000000000000, arg0, 2895);
          }
        }
        else {
          // block_6:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function maxWallet() public returns (uint256 ret0) {
        // block_0:
        return slot_10;
    }

    function name() public {
        // block_0:
    }

    function openTrade() public {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_1:
          slot_11 = slot_11 & uint256((type(uint256).max - 16776960)) | uint256(65792);
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function owner() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5 & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function removeLimits() public returns (uint256 ret0) {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_1:
          slot_11 = slot_11 & uint256((type(uint256).max - 255));
          return 1;
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function renounceOwnership() public {
        // block_0:
        private__0xb55_0xb55(286540);
    }

    function sellDevFee() public returns (uint256 ret0) {
        // block_0:
        return slot_16;
    }

    function sellLiquidityFee() public returns (uint256 ret0) {
        // block_0:
        return slot_17;
    }

    function sellTotalFees() public returns (uint256 ret0) {
        // block_0:
        return slot_15;
    }

    function swapEnabled() public returns (bool ret0) {
        // block_0:
        return slot_11 / uint256(65536) & uint256(255) != 0;
    }

    function swapTokensAtAmount() public returns (uint256 ret0) {
        // block_0:
        return slot_9;
    }

    function symbol() public {
        // block_0:
    }

    function totalSupply() public returns (uint256 ret0) {
        // block_0:
        return slot_2;
    }

    function tradingActive() public returns (bool ret0) {
        // block_0:
        return slot_11 / uint256(256) & uint256(255) != 0;
    }

    function transferFrom(address arg0, address arg1, uint256 arg2) public returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        uint256 temp_1;
        if (msg.data.length > 99) {
          // goto block_1
        }
        // goto block_64
        // block_1:
        require(uint256(uint160(arg0)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_3:
        require(uint256(uint160(arg1)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_5:
        if (!(uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
          // goto block_6
        }
        // goto block_60
        // block_6:
        if (!(uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
          // goto block_7
        }
        // goto block_59
        // block_7:
        if (!(arg2 == 0)) {
          // goto block_8
        }
        // goto block_58
        // block_8:
        if (slot_11 == 0) {
          // goto block_24
        }
        // block_9:
        if ((slot_5 ^ uint256(uint160(arg0))) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // goto block_24
        }
        // block_10:
        if ((slot_5 ^ uint256(uint160(arg1))) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // goto block_24
        }
        // block_11:
        if (uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // goto block_24
        }
        // block_12:
        if (uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 57005) {
          // goto block_24
        }
        // block_13:
        if (!(slot_6 / (uint256(1) << uint256(160)) & uint256(255) == 0)) {
          // goto block_24
        }
        // block_14:
        if (!(slot_11 / uint256(256) & uint256(255) == 0)) {
          // goto block_17
        }
        // block_15:
        if (!(slot_18[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_17
        }
        // block_16:
        if (!(slot_18[uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_17
        }
        // goto block_57
        // block_17:
        if ((uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0)) == (uint256(1) << uint256(160)) + uint256(1461501637330902918203684832716283019655932542975) & uint256(828598564998723978240204936839059866082836049505)) {
          // goto block_18
        }
        // goto block_19
        // block_18:
        if (!(slot_19[uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_19
        }
        // goto block_22
        // block_19:
        if (!(slot_19[uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_24
        }
        // block_20:
        if (!(private__0x1cec_0x1cec(arg2, slot_0[uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)], 5083) > slot_10)) {
          // goto block_24
        }
        // block_21:
        require(false, "Max wallet exceeded");
        // block_24:
        if (slot_0[uint256(uint160(address(this)))] < slot_9) {
          // goto block_41
        }
        // block_25:
        if (slot_11 / uint256(65536) & uint256(255) == 0) {
          // goto block_41
        }
        // block_26:
        if (!(slot_6 / (uint256(1) << uint256(160)) & uint256(255) == 0)) {
          // goto block_41
        }
        // block_27:
        if (!((uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg1)) == (uint256(1) << uint256(160)) + uint256(1461501637330902918203684832716283019655932542975) & uint256(828598564998723978240204936839059866082836049505))) {
          // goto block_41
        }
        // block_28:
        if (!(slot_18[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_41
        }
        // block_29:
        if (!(slot_18[uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_41
        }
        // block_30:
        slot_6 = uint256(1) << uint256(160) | slot_6 & ~(uint256(255) << uint256(160));
        if (slot_0[uint256(uint160(address(this)))] == 0) {
          // goto block_40
        }
        // block_31:
        if (slot_0[uint256(uint160(address(this)))] > private__0x1cab_0x1cab(20, slot_9, 6386)) {
          // goto block_32
        }
        // goto block_33
        // block_32:
        while (true) {
          // block_33:
          // block_34:
          if (!false /* TODO: evm.lt33.i.i.i.not */) {
            break;
          }
          continue;
        }
        // goto block_40
        // block_35:
        require(!false /* TODO: evm.iszero44.i.i.i.not */);
        // block_36:
        if (false /* TODO: evm.iszero49.i.i.i.not */) {
          // block_37:
          revert(); // returndata_forward
        }
        // block_40:
        slot_6 = slot_6 & ~(uint256(255) << uint256(160));
        // block_41:
        if (!(slot_18[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_54
        }
        // block_42:
        if (false /* TODO: cond */) {
          // goto block_54
        }
        // block_43:
        if (!((uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg1)) == (uint256(1) << uint256(160)) + uint256(1461501637330902918203684832716283019655932542975) & uint256(828598564998723978240204936839059866082836049505))) {
          // goto block_45
        }
        // block_44:
        if (slot_15 == 0) {
          // goto block_45
        }
        // goto block_48
        // block_45:
        if ((uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0)) == (uint256(1) << uint256(160)) + uint256(1461501637330902918203684832716283019655932542975) & uint256(828598564998723978240204936839059866082836049505)) {
          // block_46:
          if (!(slot_12 == 0)) {
            // block_47:
            temp_0 = private__0x1926_0x1926(100, private__0x1913_0x1913(slot_12, arg2, 288589), 5763);
            private__0x1cca_0x1cca(private__0x1cab_0x1cab(temp_0, slot_13, 5811), slot_12, 5821);
          }
        }
        // goto block_49
        // block_48:
        temp_1 = private__0x1926_0x1926(100, private__0x1913_0x1913(slot_15, arg2, 288552), 5600);
        private__0x1cca_0x1cca(private__0x1cab_0x1cab(temp_1, slot_16, 5648), slot_15, 5658);
        // block_49:
        if (!false /* TODO: evm.iszero282.i */) {
          // block_50:
        }
        // block_51:
        if (!false /* TODO: evm.iszero286.i */) {
          // goto block_52
        }
        // goto block_53
        // block_52:
        // block_53:
        // block_54:
        // goto block_61
        // block_22:
        if (arg2 > slot_8) {
          // block_56:
          revert(); // error_string, selector=0x8c379a0, string_length=53
        }
        // block_23:
        if (!(private__0x1cec_0x1cec(arg2, slot_0[uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)], 4940) > slot_10)) {
          // goto block_24
        }
        // goto block_55
        // block_55:
        require(false, "Max wallet exceeded");
        // block_57:
        require(false, "Trading is not active.");
        // block_58:
        private__0x1777_0x1777(0, uint256(uint160(arg1)), uint256(uint160(arg0)), 4418);
        // block_61:
        if (slot_1[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] < arg2) {
          // block_63:
          revert(); // error_string, selector=0x8c379a0, string_length=40
        }
        else {
          // block_62:
          private__0xfc1_0xfc1(slot_1[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] - arg2, uint256(uint160(msg.sender)), uint256(uint160(arg0)), 2460);
          return 1;
        }
        // block_59:
        revert(); // error_string, selector=0x8c379a0
        // block_60:
        revert(); // error_string, selector=0x8c379a0
        // block_64:
        revert(); // empty
    }

    function transferOwnership(address arg0) public {
        // block_0:
        require(msg.data.length > 35);
        // block_1:
        require(uint256(uint160(arg0)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_4:
        private__0xea4_0xea4(uint256(uint160(arg0)), 287417);
    }

    function transfer(address arg0, uint256 arg1) public returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        uint256 temp_1;
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
        // goto block_59
        // block_5:
        if (!(uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
          // goto block_6
        }
        // goto block_58
        // block_6:
        if (!(arg1 == 0)) {
          // goto block_7
        }
        // goto block_57
        // block_7:
        if (slot_11 == 0) {
          // goto block_23
        }
        // block_8:
        if ((slot_5 ^ uint256(uint160(msg.sender))) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // goto block_23
        }
        // block_9:
        if ((slot_5 ^ uint256(uint160(arg0))) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // goto block_23
        }
        // block_10:
        if (uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // goto block_23
        }
        // block_11:
        if (uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 57005) {
          // goto block_23
        }
        // block_12:
        if (!(slot_6 / (uint256(1) << uint256(160)) & uint256(255) == 0)) {
          // goto block_23
        }
        // block_13:
        if (!(slot_11 / uint256(256) & uint256(255) == 0)) {
          // goto block_16
        }
        // block_14:
        if (!(slot_18[uint256(uint160(msg.sender)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_16
        }
        // block_15:
        if (!(slot_18[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_16
        }
        // goto block_56
        // block_16:
        if ((uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(1461501637330902918203684832716283019655932542975) & uint256(828598564998723978240204936839059866082836049505)) {
          // goto block_17
        }
        // goto block_18
        // block_17:
        if (!(slot_19[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_18
        }
        // goto block_21
        // block_18:
        if (!(slot_19[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_23
        }
        // block_19:
        if (!(private__0x1cec_0x1cec(arg1, slot_0[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)], 5083) > slot_10)) {
          // goto block_23
        }
        // block_20:
        require(false, "Max wallet exceeded");
        // block_23:
        if (slot_0[uint256(uint160(address(this)))] < slot_9) {
          // goto block_40
        }
        // block_24:
        if (slot_11 / uint256(65536) & uint256(255) == 0) {
          // goto block_40
        }
        // block_25:
        if (!(slot_6 / (uint256(1) << uint256(160)) & uint256(255) == 0)) {
          // goto block_40
        }
        // block_26:
        if (!((uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0)) == (uint256(1) << uint256(160)) + uint256(1461501637330902918203684832716283019655932542975) & uint256(828598564998723978240204936839059866082836049505))) {
          // goto block_40
        }
        // block_27:
        if (!(slot_18[uint256(uint160(msg.sender)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_40
        }
        // block_28:
        if (!(slot_18[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_40
        }
        // block_29:
        slot_6 = uint256(1) << uint256(160) | slot_6 & ~(uint256(255) << uint256(160));
        if (slot_0[uint256(uint160(address(this)))] == 0) {
          // goto block_39
        }
        // block_30:
        if (slot_0[uint256(uint160(address(this)))] > private__0x1cab_0x1cab(20, slot_9, 6386)) {
          // goto block_31
        }
        // goto block_32
        // block_31:
        while (true) {
          // block_32:
          // block_33:
          if (!false /* TODO: evm.lt33.i.i.i.not */) {
            break;
          }
          continue;
        }
        // goto block_53
        // block_44:
        if ((uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(1461501637330902918203684832716283019655932542975) & uint256(828598564998723978240204936839059866082836049505)) {
          // block_45:
          if (!(slot_12 == 0)) {
            // block_46:
            temp_0 = private__0x1926_0x1926(100, private__0x1913_0x1913(slot_12, arg1, 288589), 5763);
            private__0x1cca_0x1cca(private__0x1cab_0x1cab(temp_0, slot_13, 5811), slot_12, 5821);
          }
        }
        // goto block_48
        // block_47:
        temp_1 = private__0x1926_0x1926(100, private__0x1913_0x1913(slot_15, arg1, 288552), 5600);
        private__0x1cca_0x1cca(private__0x1cab_0x1cab(temp_1, slot_16, 5648), slot_15, 5658);
        // goto block_48
        // block_48:
        if (!false /* TODO: evm.iszero282.i */) {
          // block_49:
        }
        // block_50:
        if (!false /* TODO: evm.iszero286.i */) {
          // goto block_51
        }
        // goto block_52
        // block_51:
        // goto block_52
        // block_34:
        require(!false /* TODO: evm.iszero44.i.i.i.not */);
        // block_35:
        if (false /* TODO: evm.iszero49.i.i.i.not */) {
          // block_36:
          revert(); // returndata_forward
        }
        // block_39:
        slot_6 = slot_6 & ~(uint256(255) << uint256(160));
        // block_40:
        // block_53:
        // goto block_60
        // block_21:
        if (arg1 > slot_8) {
          // block_55:
          revert(); // error_string, selector=0x8c379a0, string_length=53
        }
        // block_22:
        if (!(private__0x1cec_0x1cec(arg1, slot_0[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)], 4940) > slot_10)) {
          // goto block_23
        }
        // goto block_54
        // block_54:
        require(false, "Max wallet exceeded");
        // block_56:
        require(false, "Trading is not active.");
        // block_57:
        private__0x1777_0x1777(0, uint256(uint160(arg0)), uint256(uint160(msg.sender)), 4418);
        // goto block_60
        // block_58:
        revert(); // error_string, selector=0x8c379a0
        // block_59:
        revert(); // error_string, selector=0x8c379a0
        // block_52:
        // block_61:
        // block_60:
        return 1;
    }

    function uniswapV2Pair() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(1461501637330902918203684832716283019655932542975) & uint256(828598564998723978240204936839059866082836049505);
    }

    function uniswapV2Router() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(730750818665451459101842416358141509827966271487) & uint256(697323163401596485410334513241460920685086001293);
    }

    function updateDevWallet(address arg0) public {
        // block_0:
        require(msg.data.length > 35);
        // block_1:
        require(uint256(uint160(arg0)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_4:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_5:
          emit Event_0x90B8024C4923D3873FF5B9FCB43D0360D4B9217FA41225D07BA379993552E743(uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max), (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_7); // TODO: recover event signature
          slot_7 = (uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0)) | slot_7 & uint256(0) - (uint256(1) << uint256(160));
        }
        else {
          // block_6:
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
        return private__0xd4d_0xd4d(arg0, 287190) != 0;
    }

    function private__0x1cca_0x1cca(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 / arg1;
    }

    function private__0x1cec_0x1cec(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + arg1;
    }

    function private__0x1725_0x1725(uint256 arg0, uint256 arg1) internal {
        // block_0:
        slot_5 = (uint256(1) << uint256(160)) + uint256(type(uint256).max) & arg0 | slot_5 & uint256(0) - (uint256(1) << uint256(160));
        emit Event_0x8BE0079C531659141344CD1FD0A4F28419497F9722A3DAAFE3B4186F6B6457E0(slot_5 & (uint256(1) << uint256(160)) + uint256(type(uint256).max), (uint256(1) << uint256(160)) + uint256(type(uint256).max) & arg0); // TODO: recover event signature
    }

    function private__0x1777_0x1777(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
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

    function private__0x1913_0x1913(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return private__0x1cab_0x1cab(arg1, arg0, 288740);
    }

    function private__0x1926_0x1926(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return private__0x1cca_0x1cca(arg1, arg0, 288778);
    }

    function private__0x1c60_0x1c60(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(96);
    }

    function private__0x1cab_0x1cab(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 * arg1;
    }

    function private__0xd4d_0xd4d(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_1:
          if (arg0 < private__0x1cca_0x1cca(private__0x1cab_0x1cab(1, slot_2, 3474), 100000, 3484)) {
            // block_5:
            revert(); // error_string, selector=0x8c379a0, string_length=53
          }
          else {
            // block_2:
            if (arg0 > private__0x1cca_0x1cca(private__0x1cab_0x1cab(5, slot_2, 3616), 1000, 3626)) {
              // block_4:
              revert(); // error_string, selector=0x8c379a0, string_length=52
            }
            else {
              // block_3:
              slot_9 = arg0;
              return 1;
            }
          }
        }
        else {
          // block_6:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function private__0x1d04_0x1d04(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(128);
    }

    function private__0x1d49_0x1d49(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(128);
    }

    function private__0xb55_0xb55(uint256 arg0) internal {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_1:
          private__0x1725_0x1725(0, 2953);
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function private__0xea4_0xea4(uint256 arg0, uint256 arg1) internal {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_1:
          if (arg0 & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
            // block_3:
            revert(); // error_string, selector=0x8c379a0, string_length=38
          }
          else {
            // block_2:
            private__0x1725_0x1725(arg0, 288358);
          }
        }
        else {
          // block_4:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function private__0xfc1_0xfc1(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
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
