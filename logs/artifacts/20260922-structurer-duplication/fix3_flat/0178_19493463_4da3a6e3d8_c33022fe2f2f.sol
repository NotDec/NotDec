contract Decompiled {
    event Event_0x4DBA622D284ED0014EE4B9A6A68386BE1A4C08A4913AE272DE89199CC686163();
    event Event_0x8C5BE1E5EBEC7D5BD14F71427D1E84F3DD0314C0F7B2291E5B200AC8C7C3B925(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF(uint256 indexed arg0, uint256 indexed arg1);

    uint256 public slot_0;
    uint256 public slot_1;
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
    mapping(uint256 => uint256) public slot_2;
    uint256 public slot_20;
    uint256 public slot_21;
    uint256 public slot_22;
    uint256 public slot_23;
    uint256 public slot_24;
    uint256 public slot_25;
    uint256 public slot_26;
    mapping(uint256 => uint256) public slot_3;
    mapping(uint256 => uint256) public slot_4;
    mapping(uint256 => uint256) public slot_5;
    uint256 public slot_6;
    uint256 public slot_7;
    uint256 public slot_8;
    uint256 public slot_9;

    function public_0x1da9d96d() public {
        // block_0:
        require(msg.data.length > 227);
        // block_1:
    }

    function public_0x9255b525() public {
        // block_0:
        require(msg.data.length > 99);
        // block_2:
    }

    function public_0xa5b23c67() public returns (uint256 ret0) {
        // block_0:
        return slot_26;
    }

    function public_0xcf7a0017() public returns (uint256 ret0) {
        // block_0:
        return slot_24;
    }

    function public_0xe18eb907() public {
        // block_0:
        require(msg.data.length > 99);
        // block_2:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0) {
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

    function allowance(address arg0, address arg1) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 67);
        // block_2:
        private__0x1893_0x1893(uint256(uint160(arg0)), 6725);
        private__0x1893_0x1893(uint256(uint160(arg1)), 336477);
        return slot_3[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0))];
    }

    function approve(address arg0, uint256 arg1) public returns (bool ret0) {
        // block_0:
        require(msg.data.length > 67);
        // block_2:
        private__0x1893_0x1893(uint256(uint160(arg0)), 6342);
        return private__0x4e8_0x4e8(arg1, uint256(uint160(arg0)), 245108) != 0;
    }

    function balanceOf(address arg0) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        private__0x1893_0x1893(uint256(uint160(arg0)), 336363);
        return slot_2[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0))];
    }

    function decimals() public returns (uint256 ret0) {
        // block_0:
        return 9;
    }

    function enableTrading() public {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0) {
          // block_1:
          slot_7 = uint256(1) << uint256(160) | slot_7 & ~(uint256(255) << uint256(160));
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function getOwner() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0 & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function isFeeExempt(address arg0) public returns (bool ret0) {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        private__0x1893_0x1893(uint256(uint160(arg0)), 336363);
        return slot_4[uint256(uint160(arg0))] != 0;
    }

    function isOwner(address arg0) public returns (bool ret0) {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        private__0x1893_0x1893(uint256(uint160(arg0)), 336363);
        return (slot_0 ^ uint256(uint160(arg0))) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0;
    }

    function manualSwap() public {
        // block_0:
        uint256 temp_0;
        uint256 temp_1;
        uint256 temp_2;
        uint256 temp_3;
        uint256 temp_4;
        uint256 temp_5;
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0) {
          // goto block_1
        }
        // goto block_18
        // block_1:
        temp_0 = private__0x1393_0x1393(2, private__0x139e_0x139e(slot_15, private__0x139e_0x139e(slot_14, private__0x139e_0x139e(1, slot_13, 337027), 335840), 335803), 4700);
        temp_1 = private__0xcb1_0xcb1(temp_0, private__0x1393_0x1393(slot_13, slot_11, 335877), 4728);
        temp_2 = private__0xca6_0xca6(temp_1, slot_11, 4741);
        if (!false /* TODO: evm.iszero.i.i.i.not */) {
          private__0xb83_0xb83(temp_2, slot_6 & (uint256(1) << uint256(160)) + uint256(type(uint256).max), uint256(uint160(address(this))), 5774);
          while (false /* TODO: evm.lt48.i.i.i.not */) {
            // block_8:
          }
          // block_4:
          require(!false /* TODO: evm.iszero60.i.i.i.not */);
          // block_5:
          if (false /* TODO: evm.iszero66.i.i.i.not */) {
            // block_6:
            revert(); // returndata_forward
          }
          // block_10:
          temp_3 = private__0xcb1_0xcb1(private__0xca6_0xca6(slot_13, temp_0, 4791), private__0xca6_0xca6(address(this).balance, address(this).balance, 4764), 4798);
          temp_4 = private__0x1393_0x1393(slot_13, temp_3, 4822);
          if (!(temp_4 == 0)) {
            // block_11:
          }
          // block_12:
          temp_5 = private__0x1393_0x1393(slot_14, private__0x1393_0x1393(2, temp_3, 335914), 4860);
          // block_13:
          // block_17:
          revert(); // returndata_forward
          if (temp_5 == 0) {
            // goto block_14
          }
          else {
            // goto block_13
          }
        }
        // block_9:
        revert(); // returndata_forward
        // block_14:
        if (address(this).balance == 0) {
          // block_19:
          slot_9 = slot_9 & uint256((type(uint256).max - 255));
        }
        // block_15:
        if (!false /* TODO: evm.iszero46.i.i.not */) {
          // goto block_19
        }
        // goto block_16
        // block_16:
        revert(); // returndata_forward
        // block_18:
        revert(); // error_string, selector=0x8c379a0
    }

    function maxSellAmount() public returns (uint256 ret0) {
        // block_0:
        return slot_25;
    }

    function name() public {
        // block_0:
    }

    function pair() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_7 & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function rescueERC20(address arg0, uint256 arg1) public {
        // block_0:
        require(msg.data.length > 67);
        // block_2:
        private__0x1893_0x1893(uint256(uint160(arg0)), 6342);
    }

    function setInternalAddresses(address arg0, address arg1, address arg2) public {
        // block_0:
        require(msg.data.length > 99);
        // block_2:
        private__0x1893_0x1893(uint256(uint160(arg0)), 6613);
        private__0x1893_0x1893(uint256(uint160(arg1)), 6629);
        private__0x1893_0x1893(uint256(uint160(arg2)), 6645);
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0) {
          // block_3:
          slot_22 = (uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0)) | slot_22 & uint256(0) - (uint256(1) << uint256(160));
          slot_23 = (uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg1)) | slot_23 & uint256(0) - (uint256(1) << uint256(160));
          slot_21 = (uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg2)) | slot_21 & uint256(0) - (uint256(1) << uint256(160));
        }
        else {
          // block_4:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function setisExempt(address arg0, bool arg1) public {
        // block_0:
        require(msg.data.length > 67);
        // block_2:
        private__0x1893_0x1893(uint256(uint160(arg0)), 6557);
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0) {
          // block_3:
        }
        else {
          // block_4:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function symbol() public {
        // block_0:
    }

    function totalSupply() public returns (uint256 ret0) {
        // block_0:
        return 0 /* TODO: unresolved value: private__0x4fe_0x4fe */;
    }

    function transferFrom(address arg0, address arg1, uint256 arg2) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 99);
        // block_1:
        private__0x1893_0x1893(uint256(uint160(arg0)), 6456);
        private__0x1893_0x1893(uint256(uint160(arg1)), 6472);
        if (arg2 > slot_3[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)]) {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
        else {
          // block_3:
          private__0xb83_0xb83(slot_3[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] - arg2, uint256(uint160(msg.sender)), uint256(uint160(arg0)), 1747);
          return 1;
        }
    }

    function transferOwnership(address arg0) public {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        private__0x1893_0x1893(uint256(uint160(arg0)), 336363);
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0) {
          // block_3:
          slot_0 = uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) | slot_0 & uint256(0) - (uint256(1) << uint256(160));
          emit Event_0x4DBA622D284ED0014EE4B9A6A68386BE1A4C08A4913AE272DE89199CC686163(); // TODO: recover event signature
        }
        else {
          // block_4:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function transfer(address arg0, uint256 arg1) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 67);
        // block_2:
        private__0x1893_0x1893(uint256(uint160(arg0)), 6342);
        return 1;
    }

    function private__0x1393_0x1393(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return private__0x1b00_0x1b00(arg1, arg0, 335951);
    }

    function private__0x139e_0x139e(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return private__0x1aed_0x1aed(arg1, arg0, 335989);
    }

    function private__0x1461_0x1461(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        if (slot_4[arg1 & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // block_2:
          return slot_4[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & arg0] == 0 ? 1 : 0;
        }
        else {
          // block_1:
          return 0;
        }
    }

    function private__0x1893_0x1893(uint256 arg0, uint256 arg1) internal {
        // block_0:
        require(arg0 & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_1:
    }

    function private__0x4e8_0x4e8(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        private__0xb83_0xb83(arg0, arg1, uint256(uint160(msg.sender)), 245910);
        return 1;
    }

    function private__0x1974_0x1974(uint256 arg0, uint256 arg1) internal {
        // block_0:
        require(arg0 < 2);
        // block_1:
    }

    function private__0x1a55_0x1a55(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(96);
    }

    function private__0x1abb_0x1abb(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 - arg1;
    }

    function private__0x1aed_0x1aed(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg1 + arg0;
    }

    function private__0x1b00_0x1b00(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg1 * arg0;
    }

    function private__0xb83_0xb83(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
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

    function private__0xca6_0xca6(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return private__0x1abb_0x1abb(arg1, arg0, 335687);
    }

    function private__0xcb1_0xcb1(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg1 / arg0;
    }
}
