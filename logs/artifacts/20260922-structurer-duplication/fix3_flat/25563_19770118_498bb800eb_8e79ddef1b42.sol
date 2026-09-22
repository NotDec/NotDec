contract Decompiled {
    event Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0x8BE0079C531659141344CD1FD0A4F28419497F9722A3DAAFE3B4186F6B6457E0(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0x8C5BE1E5EBEC7D5BD14F71427D1E84F3DD0314C0F7B2291E5B200AC8C7C3B925(uint256 indexed arg0, uint256 indexed arg1);

    uint256 public slot_0;
    uint256 public slot_10;
    mapping(uint256 => uint256) public slot_11;
    mapping(uint256 => uint256) public slot_12;
    mapping(uint256 => uint256) public slot_13;
    mapping(uint256 => uint256) public slot_14;
    uint256 public slot_2;
    uint256 public slot_3;
    uint256 public slot_4;
    uint256 public slot_5;
    uint256 public slot_6;
    uint256 public slot_7;
    uint256 public slot_8;
    uint256 public slot_9;

    function public_0x1de3b450() public returns (uint256 ret0) {
        // block_0:
        return slot_6;
    }

    function public_0x41e1259d() public {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0) {
          // block_3:
          if (false /* TODO: evm.lt */) {
            // block_4:
            if (false /* TODO: evm.lt6 */) {
              // block_5:
            }
            else {
              // block_6:
              revert(); // error_string, selector=0x8c379a0, string_length=40
            }
          }
          else {
            // block_7:
            revert(); // error_string, selector=0x8c379a0, string_length=35
          }
        }
        else {
          // block_8:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function public_0x55811acc() public {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0) {
          // block_3:
          if (false /* TODO: evm.lt */) {
            // block_4:
            if (false /* TODO: evm.gt */) {
              // block_5:
            }
            else {
              // block_6:
              revert(); // error_string, selector=0x8c379a0, string_length=37
            }
          }
          else {
            // block_7:
            revert(); // error_string, selector=0x8c379a0, string_length=35
          }
        }
        else {
          // block_8:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function public_0xaca1f157() public {
        // block_0:
        require(msg.data.length > 35);
        // block_1:
        require(false /* TODO: evm.gt */);
        // block_2:
        require(false /* TODO: evm.slt10 */);
        // block_3:
        require(false /* TODO: evm.gt14 */);
        // block_4:
        require(!false /* TODO: evm.gt21.not */);
        // block_5:
        // block_10:
        revert(); // error_string, selector=0x8c379a0
        if (!(uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0)) {
          // goto block_10
        }
        while (false /* TODO: evm.lt.i.not */) {
          // block_7:
          if (!false /* TODO: evm.eq.i.i.i */) {
            break;
          }
          // block_9:
        }
        // block_8:
        revert(); // empty
    }

    function public_0xdba9d420() public {
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

    function public_0xe5cd8b6a() public {
        // block_0:
    }

    function public_0xeeeeeeee() public {
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
        return slot_12[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0))];
    }

    function approve(address arg0, uint256 arg1) public returns (bool ret0) {
        // block_0:
        require(msg.data.length > 67);
        // block_1:
        require(uint256(uint160(arg0)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_4:
        return private__0x581_0x581(arg1, uint256(uint160(arg0)), 223092) != 0;
    }

    function balanceOf(address arg0) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 35);
        // block_1:
        require(uint256(uint160(arg0)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_4:
        return slot_11[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0))];
    }

    function buyTax() public returns (uint256 ret0) {
        // block_0:
        return slot_4;
    }

    function changeMaxTx(uint256 arg0) public {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0) {
          // block_3:
          if (arg0 < 99) {
            // block_4:
            slot_9 = private__0x1905_0x1905(arg0, 10000000000000000000000, 2508);
          }
          else {
            // block_5:
            revert(); // error_string, selector=0x8c379a0, string_length=26
          }
        }
        else {
          // block_6:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function changeMaxWallet(uint256 arg0) public {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0) {
          // block_3:
          if (arg0 < 99) {
            // block_4:
            slot_10 = private__0x1905_0x1905(arg0, 10000000000000000000000, 1583);
          }
          else {
            // block_5:
            revert(); // error_string, selector=0x8c379a0, string_length=30
          }
        }
        else {
          // block_6:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function changeTax(uint256 arg0, uint256 arg1, uint256 arg2) public {
        // block_0:
        require(msg.data.length > 99);
        // block_1:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0) {
          // block_2:
          if (false /* TODO: _0xa16_0x0.in */) {
            // block_4:
            slot_4 = arg0;
            slot_5 = arg1;
            slot_6 = arg2;
          }
          // block_3:
          if (private__0x192f_0x192f(private__0x192f_0x192f(arg0, arg1, 2601), arg2, 2611) < 90) {
            // goto block_4
          }
          // goto block_5
          // block_5:
          revert(); // error_string, selector=0x8c379a0, string_length=26
        }
        else {
          // block_6:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function decimals() public returns (uint256 ret0) {
        // block_0:
        return 18;
    }

    function getContractBalance() public returns (uint256 ret0) {
        // block_0:
        return address(this).balance;
    }

    function maxTxAmount() public returns (uint256 ret0) {
        // block_0:
        return slot_9;
    }

    function maxWalletAmount() public returns (uint256 ret0) {
        // block_0:
        return slot_10;
    }

    function name() public {
        // block_0:
        while (false /* TODO: evm.lt.not */) {
          // block_3:
        }
        // block_2:
    }

    function openTrading() public {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0) {
          // block_1:
          slot_8 = block.number;
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function owner() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0 & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function removeLimits() public {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0) {
          // block_1:
          slot_9 = 10000000000000000000000000;
          slot_10 = 10000000000000000000000000;
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
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

    function sellTax() public returns (uint256 ret0) {
        // block_0:
        return slot_5;
    }

    function symbol() public {
        // block_0:
        while (false /* TODO: evm.lt.not */) {
          // block_3:
        }
        // block_2:
    }

    function totalSupply() public returns (uint256 ret0) {
        // block_0:
        return 10000000000000000000000000;
    }

    function transferFrom(address arg0, address arg1, uint256 arg2) public returns (uint256 ret0) {
        // block_0:
        if (msg.data.length > 99) {
          // goto block_1
        }
        // goto block_46
        // block_1:
        require(uint256(uint160(arg0)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_3:
        require(uint256(uint160(arg1)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_5:
        if (!(uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
          // goto block_6
        }
        // goto block_44
        // block_6:
        if (arg2 > 1000000000) {
          // goto block_7
        }
        // goto block_43
        // block_7:
        if (slot_13[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_8
        }
        // goto block_39
        // block_8:
        if (slot_13[uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_9
        }
        else {
          // goto block_39
        }
        // block_21:
        if (false /* TODO: evm.iszero135.i.not */) {
          // block_33:
          revert(); // returndata_forward
        }
        // block_22:
        if (!false /* TODO: evm.gt140.i.not */) {
          // block_23:
          require(!false /* TODO: evm.iszero158.i.not */);
          // block_24:
          if (false /* TODO: evm.iszero165.i.not */) {
            // block_31:
            revert(); // returndata_forward
          }
          else {
            // block_25:
            if (false /* TODO: evm.iszero200.i.not */) {
              // block_30:
              revert(); // returndata_forward
            }
            // block_26:
            if (false /* TODO: evm.slt.i */) {
              // goto block_27
            }
            // goto block_29
            // block_29:
            revert(); // empty
          }
        }
        // block_27:
        slot_7 = slot_7 & uint256((type(uint256).max - 65280));
        // goto block_28
        // block_9:
        if (slot_7 == 0) {
          // block_38:
          revert(); // error_string, selector=0x8c379a0, string_length=34
        }
        // block_10:
        if (arg2 > slot_9) {
          // goto block_38
        }
        // goto block_11
        // block_11:
        // block_13:
        if (slot_7 / uint256(256) & uint256(255) == 1) {
          // goto block_36
        }
        // goto block_14
        // block_14:
        // block_28:
        // block_34:
        revert(); // empty
        // block_43:
        require(false, "Min transfer amt");
        // block_44:
        revert(); // error_string, selector=0x8c379a0, string_length=37
        // block_39:
        if (false /* TODO: evm.iszero274.i */) {
          // block_36:
          // block_41:
          emit Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF(0 /* TODO: unresolved value: evm.sub252.i.pn.in.reload */ + uint256(type(uint256).max) & uint256(uint160(arg0)), 0 /* TODO: unresolved value: evm.sub249.i.pn.in.reload */ + uint256(type(uint256).max) & uint256(uint160(arg1))); // TODO: recover event signature
        }
        else {
          // block_42:
          emit Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF(uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max), uint256(uint160(address(this)))); // TODO: recover event signature
          emit Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF((uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0)), (uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg1))); // TODO: recover event signature
        }
        // block_45:
        private__0xcb8_0xcb8(private__0x191c_0x191c(slot_12[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)], arg2, 1656), uint256(uint160(msg.sender)), uint256(uint160(arg0)), 1661);
        return 1;
        // block_46:
        revert(); // empty
    }

    function transferOwnership(address arg0) public {
        // block_0:
        require(msg.data.length > 35);
        // block_1:
        require(uint256(uint160(arg0)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_4:
        private__0xc82_0xc82(uint256(uint160(arg0)), 223956);
    }

    function transfer(address arg0, uint256 arg1) public returns (uint256 ret0) {
        // block_0:
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
        // goto block_43
        // block_5:
        if (arg1 > 1000000000) {
          // goto block_6
        }
        // goto block_42
        // block_6:
        if (slot_13[uint256(uint160(msg.sender)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_7
        }
        // goto block_38
        // block_7:
        if (slot_13[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_8
        }
        else {
          // goto block_38
        }
        // block_15:
        if (!(slot_11[uint256(uint160(address(this)))] > slot_2)) {
          // goto block_27
        }
        // block_16:
        if (!(slot_7 / uint256(256) & uint256(255) == 0)) {
          // goto block_27
        }
        // goto block_17
        // block_17:
        if (slot_11[uint256(uint160(address(this)))] > slot_3) {
          // block_18:
        }
        // block_19:
        slot_7 = slot_7 & uint256((type(uint256).max - 65280)) | uint256(256);
        if (false /* TODO: evm.iszero130.i.i.not */) {
          // goto block_33
        }
        // goto block_20
        // block_20:
        if (false /* TODO: evm.iszero135.i.i.not */) {
          // block_32:
          revert(); // returndata_forward
        }
        // block_21:
        if (!false /* TODO: evm.gt140.i.i.not */) {
          // block_22:
          require(!false /* TODO: evm.iszero158.i.i.not */);
          // block_23:
          if (false /* TODO: evm.iszero165.i.i.not */) {
            // block_30:
            revert(); // returndata_forward
          }
          else {
            // block_24:
            if (false /* TODO: evm.iszero200.i.i.not */) {
              // block_29:
              revert(); // returndata_forward
            }
            // block_25:
            if (false /* TODO: evm.slt.i.i */) {
              // goto block_26
            }
            // goto block_28
            // block_28:
            revert(); // empty
          }
        }
        // block_26:
        slot_7 = slot_7 & uint256((type(uint256).max - 65280));
        // goto block_27
        // block_8:
        if (slot_7 == 0) {
          // block_37:
          revert(); // error_string, selector=0x8c379a0, string_length=34
        }
        // block_9:
        if (arg1 > slot_9) {
          // goto block_37
        }
        // goto block_10
        // block_10:
        // block_12:
        if (slot_7 / uint256(256) & uint256(255) == 1) {
          // goto block_35
        }
        // goto block_13
        // block_13:
        // block_27:
        // block_33:
        revert(); // empty
        // block_36:
        revert(); // error_string, selector=0x8c379a0, string_length=26
        // block_42:
        require(false, "Min transfer amt");
        // block_43:
        revert(); // error_string, selector=0x8c379a0, string_length=37
        // block_38:
        if (false /* TODO: evm.iszero274.i.i */) {
          // block_35:
          // block_40:
          emit Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF(0 /* TODO: unresolved value: evm.sub252.i.i.pn.in.reload */ + uint256(type(uint256).max) & uint256(uint160(msg.sender)), 0 /* TODO: unresolved value: evm.sub249.i.i.pn.in.reload */ + uint256(type(uint256).max) & uint256(uint160(arg0))); // TODO: recover event signature
        }
        else {
          // block_41:
          emit Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF(uint256(uint160(msg.sender)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max), uint256(uint160(address(this)))); // TODO: recover event signature
          emit Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF((uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(msg.sender)), (uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0))); // TODO: recover event signature
        }
        // block_44:
        return 1;
    }

    function uniswapV2Pair() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(1461501637330902918203684832716283019655932542975) & uint256(959103400640002380765068170292887717331457669537);
    }

    function private__0x18ba_0x18ba(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(96);
    }

    function private__0x1905_0x1905(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg1 * arg0;
    }

    function private__0x191c_0x191c(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 - arg1;
    }

    function private__0x192f_0x192f(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg1 + arg0;
    }

    function private__0x1971_0x1971(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 / arg1;
    }

    function private__0xcb8_0xcb8(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
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

    function private__0x581_0x581(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        private__0xcb8_0xcb8(arg0, arg1, uint256(uint160(msg.sender)), 223989);
        return 1;
    }

    function private__0xc82_0xc82(uint256 arg0, uint256 arg1) internal {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0) {
          // block_1:
          if (arg0 & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
            // block_3:
            revert(); // error_string, selector=0x8c379a0, string_length=38
          }
          else {
            // block_2:
            emit Event_0x8BE0079C531659141344CD1FD0A4F28419497F9722A3DAAFE3B4186F6B6457E0(slot_0 & (uint256(1) << uint256(160)) + uint256(type(uint256).max), arg0 & (uint256(1) << uint256(160)) + uint256(type(uint256).max)); // TODO: recover event signature
            slot_0 = (uint256(1) << uint256(160)) + uint256(type(uint256).max) & arg0 | slot_0 & uint256(0) - (uint256(1) << uint256(160));
          }
        }
        else {
          // block_4:
          revert(); // error_string, selector=0x8c379a0
        }
    }
}
