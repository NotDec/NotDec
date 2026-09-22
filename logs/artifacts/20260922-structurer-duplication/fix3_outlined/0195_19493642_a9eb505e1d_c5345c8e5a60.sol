contract Decompiled {
    event Event_0x9A5A8A32AFD899E7F95003C6E21C9FAB2D50E11992439D14472229180C60C7AA();
    event Event_0xC6C8102F3AC634C5FB327BA1A5D5C18030294D9F5CC309AFA9E8A9020A77175();
    event Event_0x49926BBEBE8474393F434DFA4F78694C0923EFA07D19F2284518BFABD06EB737(address indexed arg0);
    event Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF(uint256 indexed arg0, uint256 indexed arg1);

    mapping(uint256 => uint256) public slot_0;
    uint256 public slot_1;
    mapping(uint256 => uint256) public slot_10;
    uint256 public slot_2;
    uint256 public slot_3;
    uint256 public slot_4;
    uint256 public slot_5;
    uint256 public slot_6;
    uint256 public slot_7;
    uint256 public slot_8;
    uint256 public slot_9;

    function public_0x04c0c476() public returns (uint256 ret0) {
        // block_0:
        return slot_3;
    }

    function public_0x12a54b62() public returns (bool ret0) {
        // block_0:
        return slot_7 / (uint256(1) << uint256(176)) & uint256(255) != 0;
    }

    function public_0x27de2e85() public {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_7) {
          // block_1:
          if (false /* TODO: evm.lt */) {
            // block_2:
          }
          else {
            // block_3:
            revert(); // error_string, selector=0x8c379a0, string_length=26
          }
        }
        else {
          // block_4:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function public_0x9a540abf() public {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_6) {
          // block_1:
          if (slot_7 / (uint256(1) << uint256(168)) & uint256(255) == 0) {
            // block_2:
            slot_7 = uint256(1) << uint256(168) | slot_7 & ~(uint256(255) << uint256(168));
            require(!false /* TODO: evm.branch.cond10.not */, "No ETH sent");
            // block_3:
            require(false /* TODO: evm.lt */, "Block number too low");
            // block_4:
            slot_7 = uint256(1) << uint256(160) | uint256(uint160(msg.sender)) | slot_7 & uint256(0) - (uint256(1) << uint256(168));
            emit Event_0xC6C8102F3AC634C5FB327BA1A5D5C18030294D9F5CC309AFA9E8A9020A77175(); // TODO: recover event signature
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

    function maxWallet() public returns (uint256 ret0) {
        // block_0:
        return slot_2;
    }

    function balanceOf(address arg0) public returns (uint256 ret0) {
        // block_0:
        return slot_0[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0))];
    }

    function buy(uint256 arg0) public {
        // block_0:
        require(slot_7 / (uint256(1) << uint256(160)) & uint256(255) != 0, "Trading not enable");
        // block_1:
        if (slot_7 / (uint256(1) << uint256(176)) & uint256(255) == 0) {
          // block_3:
          require(!false /* TODO: evm.lt.not */, "Slippage too high");
          // block_4:
          emit Event_0x49926BBEBE8474393F434DFA4F78694C0923EFA07D19F2284518BFABD06EB737(msg.sender); // TODO: recover event signature
        }
        // block_2:
        if (!false /* TODO: evm.gt.not */) {
          // goto block_3
        }
        // goto block_6
        // block_6:
        require(false, "Max wallet exceeded");
    }

    function decimals() public returns (uint256 ret0) {
        // block_0:
        return 18;
    }

    function enableMaxWallet(bool arg0) public {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_6) {
          // block_1:
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function enableTrading(bool arg0) public {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_6) {
          // block_1:
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function fallback() public {
        // block_0:
        revert(); // empty
    }

    function getAmountOut(uint256 arg0, bool arg1) public returns (uint256 ret0) {
        // block_0:
        return 0 /* TODO: unresolved value: private__0x574_0x574 */;
    }

    function getReserves() public returns (uint256 ret0, uint256 ret1) {
        // block_0:
        return (address(this).balance, slot_0[uint256(uint160(address(this)))]);
    }

    function liquidityAdded() public returns (bool ret0) {
        // block_0:
        return slot_7 / (uint256(1) << uint256(168)) & uint256(255) != 0;
    }

    function liquidityProvider() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_7 & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function name() public {
        // block_0:
    }

    function owner() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_6 & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function presaleEnable() public returns (bool ret0) {
        // block_0:
        return slot_9 != 0;
    }

    function presale(address arg0) public {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_6) {
          // goto block_1
        }
        // goto block_7
        // block_1:
        if (slot_9 == 0) {
          while (false /* TODO: evm.lt.not */) {
            // block_5:
          }
          // block_4:
        }
        // block_6:
        revert(); // error_string, selector=0x8c379a0, string_length=23
        // block_7:
        revert(); // error_string, selector=0x8c379a0
    }

    function removeLiquidity() public {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_7) {
          // block_1:
          require(block.number > slot_3, "Liquidity locked");
          // block_2:
          slot_7 = slot_7 & ~(uint256(255) << uint256(160));
          if (false /* TODO: evm.iszero13.not */) {
            // block_4:
            revert(); // returndata_forward
          }
          else {
            // block_3:
            emit Event_0x9A5A8A32AFD899E7F95003C6E21C9FAB2D50E11992439D14472229180C60C7AA(); // TODO: recover event signature
          }
        }
        else {
          // block_6:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function renounceOwnership() public {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_6) {
          // block_1:
          slot_6 = slot_6 & uint256(0) - (uint256(1) << uint256(160));
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function sell(uint256 arg0, uint256 arg1) public {
        // block_0:
        uint256 temp_0;
        require(msg.data.length > 67);
        // block_1:
        require(slot_7 / (uint256(1) << uint256(160)) & uint256(255) != 0, "Trading not enable");
        // block_2:
        temp_0 = private__0x574_0x574(0, arg0, 2959);
        require(temp_0 != 0, "Sell amount too low");
        // block_3:
        if (address(this).balance < temp_0) {
          // block_9:
          revert(); // error_string, selector=0x8c379a0, string_length=28
        }
        else {
          // block_4:
          require(temp_0 >= arg1, "Slippage too high");
          // block_5:
          if (false /* TODO: evm.iszero18.not */) {
            // block_7:
            revert(); // returndata_forward
          }
          else {
            // block_6:
            emit Event_0x49926BBEBE8474393F434DFA4F78694C0923EFA07D19F2284518BFABD06EB737(msg.sender); // TODO: recover event signature
          }
        }
    }

    function setMaxWallet(uint256 arg0) public {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_6) {
          // block_1:
          slot_2 = arg0;
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function symbol() public {
        // block_0:
    }

    function totalSupply() public returns (uint256 ret0) {
        // block_0:
        return slot_1;
    }

    function tradingEnable() public returns (bool ret0) {
        // block_0:
        return slot_7 / (uint256(1) << uint256(160)) & uint256(255) != 0;
    }

    function transfer(address arg0, uint256 arg1) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 67);
        // block_1:
        if (uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == uint256(uint160(address(this)))) {
          // block_3:
          revert(); // error_string, selector=0x8c379a0, string_length=32
        }
        else {
          // block_2:
          return 1;
        }
    }

    function private__0x1310_0x1310(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg1 * arg0;
    }

    function private__0x12fd_0x12fd(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg1 + arg0;
    }

    function private__0x1327_0x1327(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 / arg1;
    }

    function private__0x1349_0x1349(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(128);
    }

    function private__0x138b_0x138b(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(96);
    }

    function private__0x574_0x574(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        if (arg0 == 0) {
          // block_1:
          return private__0x1327_0x1327(private__0x1310_0x1310(arg1, address(this).balance, 123214), private__0x12fd_0x12fd(slot_0[uint256(uint160(address(this)))], arg1, 1483), 125005);
        }
        else {
          // block_2:
          return private__0x1327_0x1327(private__0x1310_0x1310(arg1, slot_0[uint256(uint160(address(this)))], 123096), private__0x12fd_0x12fd(address(this).balance, arg1, 1444), 124928);
        }
    }
}
