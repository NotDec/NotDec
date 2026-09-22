contract Decompiled {
    function execute(bytes calldata arg0, uint8 arg1) public {
        // block_0:
        require(msg.data.length > 67);
        // block_1:
        require(!false /* TODO: evm.gt.i.not */);
        // block_2:
        require(false /* TODO: evm.slt8.i */);
        // block_3:
        require(!false /* TODO: evm.gt12.i.not */);
        // block_4:
        require(!false /* TODO: evm.gt19.i.not */);
        // block_5:
        require(arg1 <= (uint256(1) << uint256(64)) + uint256(type(uint256).max));
        // block_6:
        require(arg1 + uint256(35) < msg.data.length);
        // block_7:
        // block_13:
        revert(); // empty
        if (false /* TODO: evm.gt48.i.not */) {
          // goto block_13
        }
        else {
          // goto block_8
        }
        while (false /* TODO: evm.eq.i */) {
          // block_8:
          // block_9:
          if (!false /* TODO: evm.lt.i.not */) {
            break;
          }
        }
        // block_12:
        revert(); // empty
    }

    function fallback() public {
        // block_0:
        revert(); // empty
    }

    function parseInputs(bytes calldata arg0) public returns (uint256 ret0, uint256 ret1, uint256 ret2, uint256 ret3) {
        // block_0:
        require(msg.data.length > 35);
        // block_1:
        require(!false /* TODO: evm.gt.not */);
        // block_2:
        require(false /* TODO: evm.slt11 */);
        // block_3:
        require(!false /* TODO: evm.gt24.not */);
        // block_4:
        return (0 /* TODO: unresolved value: evm.mload34 */, (uint256(1) << uint256(160)) + uint256(type(uint256).max) & 0 /* TODO: unresolved value: evm.mload36 */, 0 /* TODO: unresolved value: evm.mload42 */ & (uint256(1) << uint256(160)) + uint256(type(uint256).max), 0 /* TODO: unresolved value: evm.mload46 */);
    }

    function private__0xa12_0xa12(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 - arg1;
    }

    function private__0x61b_0x61b(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        require(arg0 > type(uint256).max, "int-overflow");
        // block_1:
        return arg0;
    }

    function private__0x666_0x666(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        if (arg1 == 0) {
          // block_2:
          return 0;
        }
        else {
          // block_1:
          temp_0 = private__0xb2e_0xb2e(arg1, arg0, 1668);
          if (temp_0 / arg1 == arg0) {
            // block_3:
            return temp_0;
          }
          else {
            // block_4:
            revert(); // error_string, selector=0x8c379a0, string_length=33
          }
        }
    }

    function private__0x8e9_0x8e9(uint256 arg0, uint256 arg1) internal {
        // block_0:
        require(arg0 & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_1:
    }

    function private__0xb2e_0xb2e(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg1 * arg0;
    }
}
