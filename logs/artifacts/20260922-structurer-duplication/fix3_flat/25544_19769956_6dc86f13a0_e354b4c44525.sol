contract Decompiled {
    event Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0x8BE0079C531659141344CD1FD0A4F28419497F9722A3DAAFE3B4186F6B6457E0(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0x8C5BE1E5EBEC7D5BD14F71427D1E84F3DD0314C0F7B2291E5B200AC8C7C3B925(uint256 indexed arg0, uint256 indexed arg1);

    uint256 public slot_0;
    mapping(uint256 => uint256) public slot_2;
    mapping(uint256 => uint256) public slot_3;
    mapping(uint256 => uint256) public slot_4;
    uint256 public slot_5;
    uint256 public slot_6;
    uint256 public slot_7;
    uint256 public slot_8;
    uint256 public slot_9;

    function public_0x47d35f20() public {
        // block_0:
        require(msg.data.length > 67);
        // block_1:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0) {
          // block_2:
          if (false /* TODO: evm.lt */) {
            // block_3:
            if (false /* TODO: evm.lt11 */) {
              // block_4:
            }
            else {
              // block_5:
              revert(); // error_string, selector=0x8c379a0, string_length=35
            }
          }
          else {
            // block_6:
            revert(); // error_string, selector=0x8c379a0, string_length=34
          }
        }
        else {
          // block_7:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function public_0x4b7c6044() public returns (uint256 ret0) {
        // block_0:
        return slot_7;
    }

    function public_0x539d904d() public {
        // block_0:
    }

    function public_0x6e782199() public {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0) {
          // block_1:
          slot_8 = slot_8 & uint256(65535) - (uint256(1) << uint256(176)) | uint256(45699770836687027267851682659792382898017796180738048);
          if (false /* TODO: evm.iszero14.not */) {
            // block_7:
            revert(); // returndata_forward
          }
          else {
            // block_2:
            if (false /* TODO: evm.iszero42.not */) {
              // block_6:
              revert(); // returndata_forward
            }
            else {
              // block_3:
              if (false /* TODO: evm.iszero70.not */) {
                // block_5:
                revert(); // returndata_forward
              }
              else {
                // block_4:
              }
            }
          }
        }
        else {
          // block_8:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function public_0x9c467b7e() public returns (uint256 ret0) {
        // block_0:
        return slot_6;
    }

    function public_0xeeeeeeee() public {
        // block_0:
    }

    function addLiquidityETH() public {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0) {
          // block_1:
          private__0xa73_0xa73(type(uint256).max, (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_8 / uint256(65536), uint256(uint160(address(this))), 2400);
          if (false /* TODO: evm.iszero34.not */) {
            // block_5:
            revert(); // returndata_forward
          }
          else {
            // block_2:
            require(false /* TODO: evm.slt */);
            // block_3:
          }
        }
        else {
          // block_6:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function allowance(address arg0, address arg1) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 67);
        // block_2:
        private__0x132f_0x132f(uint256(uint160(arg0)), 5141);
        private__0x132f_0x132f(uint256(uint160(arg1)), 5157);
        return slot_3[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0))];
    }

    function approve(address arg0, uint256 arg1) public returns (bool ret0) {
        // block_0:
        require(msg.data.length > 67);
        // block_2:
        private__0x132f_0x132f(uint256(uint160(arg0)), 4962);
        return private__0x47d_0x47d(arg1, uint256(uint160(arg0)), 159892) != 0;
    }

    function balanceOf(address arg0) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        private__0x132f_0x132f(uint256(uint160(arg0)), 242160);
        return slot_2[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0))];
    }

    function decimals() public returns (uint256 ret0) {
        // block_0:
        return 18;
    }

    function excludeFromFees(address arg0) public {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        private__0x132f_0x132f(uint256(uint160(arg0)), 242160);
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0) {
          // block_3:
        }
        else {
          // block_4:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function maxTxAmount() public returns (uint256 ret0) {
        // block_0:
        return slot_5;
    }

    function name() public {
        // block_0:
        while (false /* TODO: evm.lt.not */) {
          // block_3:
        }
        // block_2:
    }

    function owner() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0 & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function removeLimits() public {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0) {
          // block_1:
          slot_5 = type(uint256).max;
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

    function symbol() public {
        // block_0:
        while (false /* TODO: evm.lt.not */) {
          // block_3:
        }
        // block_2:
    }

    function totalSupply() public returns (uint256 ret0) {
        // block_0:
        return 1000000000000000000000000000;
    }

    function transferFrom(address arg0, address arg1, uint256 arg2) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 99);
        // block_1:
        private__0x132f_0x132f(uint256(uint160(arg0)), 5008);
        private__0x132f_0x132f(uint256(uint160(arg1)), 5024);
        if (uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // block_30:
          revert(); // error_string, selector=0x8c379a0, string_length=37
        }
        else {
          // block_2:
          require(arg2 > 1000000000, "Min transfer amt");
          // block_3:
          if (slot_4[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
            // block_4:
            if (slot_4[uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
              // block_5:
              if (slot_8 == 0) {
                // block_28:
                revert(); // error_string, selector=0x8c379a0, string_length=34
              }
              // block_6:
              if (arg2 > slot_5) {
                // goto block_28
              }
              // goto block_7
            }
          }
          // block_7:
          if (slot_8 / uint256(256) & uint256(255) == 1) {
            // goto block_27
          }
          // block_8:
          // block_9:
          // block_10:
          // block_11:
          // block_12:
          // block_13:
          // block_14:
          slot_8 = slot_8 & uint256((type(uint256).max - 65280)) | uint256(256);
          // block_26:
          revert(); // returndata_forward
        }
        while (true) {
          // block_15:
          // block_16:
          if (!false /* TODO: evm.lt157.i.not */) {
            break;
          }
          continue;
        }
        // goto block_19
        // block_17:
        require(!false /* TODO: evm.iszero169.i.not */);
        // block_18:
        if (false /* TODO: evm.iszero175.i.not */) {
          // block_23:
          revert(); // returndata_forward
        }
        // block_19:
        slot_8 = slot_8 & uint256((type(uint256).max - 65280));
        // block_20:
        if (!false /* TODO: evm.iszero185.i */) {
          // block_21:
          emit Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF((uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0)), (uint256(1) << uint256(160)) + uint256(type(uint256).max) & 0 /* TODO: unresolved value: private.ret184.i */); // TODO: recover event signature
        }
        // block_22:
        emit Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF((uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0)), (uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg1))); // TODO: recover event signature
        // goto block_31
        // block_27:
        emit Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF((uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0)), (uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg1))); // TODO: recover event signature
        // block_31:
        private__0xa73_0xa73(private__0x1446_0x1446(slot_3[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)], arg2, 1239), uint256(uint160(msg.sender)), uint256(uint160(arg0)), 1244);
        return 1;
    }

    function transferOwnership(address arg0) public {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        private__0x132f_0x132f(uint256(uint160(arg0)), 242160);
        private__0xa40_0xa40(uint256(uint160(arg0)), 160654);
    }

    function transfer(address arg0, uint256 arg1) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 67);
        // block_2:
        private__0x132f_0x132f(uint256(uint160(arg0)), 4962);
        if (uint256(uint160(msg.sender)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // block_31:
          revert(); // error_string, selector=0x8c379a0, string_length=37
        }
        else {
          // block_3:
          require(arg1 > 1000000000, "Min transfer amt");
          // block_4:
          if (slot_4[uint256(uint160(msg.sender)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
            // block_5:
            if (slot_4[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
              // block_6:
              if (slot_8 == 0) {
                // block_29:
                revert(); // error_string, selector=0x8c379a0, string_length=34
              }
              // block_7:
              if (arg1 > slot_5) {
                // goto block_29
              }
              // goto block_8
            }
          }
          // block_8:
          if (slot_8 / uint256(256) & uint256(255) == 1) {
            // goto block_28
          }
          // block_9:
          // block_10:
          // block_11:
          // block_12:
          // block_13:
          // block_14:
          // block_15:
          slot_8 = slot_8 & uint256((type(uint256).max - 65280)) | uint256(256);
          // block_27:
          revert(); // returndata_forward
        }
        while (true) {
          // block_16:
          // block_17:
          if (!false /* TODO: evm.lt157.i.i.not */) {
            break;
          }
          continue;
        }
        // goto block_20
        // block_18:
        require(!false /* TODO: evm.iszero169.i.i.not */);
        // block_19:
        if (false /* TODO: evm.iszero175.i.i.not */) {
          // block_24:
          revert(); // returndata_forward
        }
        // block_20:
        slot_8 = slot_8 & uint256((type(uint256).max - 65280));
        // block_21:
        if (!false /* TODO: evm.iszero185.i.i */) {
          // block_22:
          emit Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF((uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(msg.sender)), (uint256(1) << uint256(160)) + uint256(type(uint256).max) & 0 /* TODO: unresolved value: private.ret184.i.i */); // TODO: recover event signature
        }
        // block_23:
        emit Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF((uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(msg.sender)), (uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0))); // TODO: recover event signature
        // goto block_32
        // block_28:
        emit Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF((uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(msg.sender)), (uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0))); // TODO: recover event signature
        // block_32:
        return 1;
    }

    function uniswapV2Pair() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_9 & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function uniswapV2Router() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_8 / uint256(65536) & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function private__0x1459_0x1459(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(96);
    }

    function private__0x132f_0x132f(uint256 arg0, uint256 arg1) internal {
        // block_0:
        require(arg0 & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_1:
    }

    function private__0x1446_0x1446(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 - arg1;
    }

    function private__0x14d9_0x14d9(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg1 + arg0;
    }

    function private__0x47d_0x47d(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        private__0xa73_0xa73(arg0, arg1, uint256(uint160(msg.sender)), 160687);
        return 1;
    }

    function private__0xa40_0xa40(uint256 arg0, uint256 arg1) internal {
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

    function private__0xa73_0xa73(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
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
