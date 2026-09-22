contract Decompiled {
    event Event_0xB5A3E9571E367979A4A14DE42B248D0837C26FD8E879846062ABCF7CEE171273(uint256 indexed arg0, uint256 indexed arg1);

    uint256 public slot_0;
    uint256[] public slot_2;
    uint256[] public slot_3;

    function public_0x012b8729() public {
        // block_0:
        require(msg.data.length >= 36);
        // block_2:
        require(!false /* TODO: evm.gt */);
        // block_4:
        while (false /* TODO: evm.lt */) {
          // block_6:
        }
        // block_7:
    }

    function public_0x212f6912() public {
        // block_0:
        require(msg.data.length >= 36);
        // block_2:
        require(false /* TODO: evm.branch.cond2.not */);
        // block_4:
        if (false /* TODO: evm.iszero */) {
          // block_5:
          revert(); // error_string, selector=0x8c379a0, string_length=47
        }
        else {
          // block_6:
        }
    }

    function public_0x4a00cc48() public {
        // block_0:
        require(msg.data.length + uint256((type(uint256).max - 3)) > type(uint256).max);
        while (true) {
          // block_2:
          // block_3:
          if (!false /* TODO: evm.lt */) {
            break;
          }
          continue;
        }
        while (true) {
          // block_5:
          // block_6:
          if (!false /* TODO: evm.lt14 */) {
            break;
          }
          continue;
        }
        while (true) {
          // block_8:
          if (!false /* TODO: evm.lt21 */) {
            break;
          }
          while (true) {
            // block_9:
            // block_10:
            if (!false /* TODO: evm.lt35 */) {
              break;
            }
            continue;
          }
          // block_12:
          continue;
        }
        // block_13:
        while (false /* TODO: evm.lt63 */) {
          // block_15:
        }
        // block_16:
    }

    function public_0x7c3b1137() public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length >= 36);
        // block_2:
        require(!false /* TODO: evm.gt */);
        // block_4:
        return 0 /* TODO: unresolved value: evm.mload10 */ & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function public_0xc22707ee() public {
        // block_0:
        require(msg.data.length >= 36);
        // block_2:
        require(!false /* TODO: evm.gt */);
        // block_4:
    }

    function public_0xfdc9f257() public {
        // block_0:
        require(msg.data.length >= 36);
        // block_2:
        require(!false /* TODO: evm.gt */);
        // block_4:
        require(!false /* TODO: evm.slt7 */);
        // block_6:
        require(!false /* TODO: evm.gt12 */);
        // block_8:
        require(!false /* TODO: evm.slt19 */);
        // block_10:
        require(!false /* TODO: evm.gt25 */);
        // block_12:
        require(!false /* TODO: evm.gt33 */);
        // block_14:
        require(false /* TODO: evm.branch.cond46.not */);
        // block_16:
        require(!false /* TODO: evm.gt50 */);
        // block_18:
        require(false /* TODO: evm.slt56.not */);
        // block_20:
        require(!false /* TODO: evm.gt69 */);
        // block_22:
        while (false /* TODO: evm.lt */) {
          // block_24:
          if (false /* TODO: evm.gt76 */) {
            // goto block_25
          }
          // block_26:
          if (false /* TODO: evm.slt83 */) {
            // goto block_27
          }
          // block_28:
          if (!false /* TODO: evm.branch.cond93.not */) {
            // goto block_29
          }
          // block_30:
          if (false /* TODO: evm.gt96 */) {
            // goto block_31
          }
          // block_32:
          continue;
        }
        // block_25:
        revert(); // empty
        // block_27:
        revert(); // empty
        // block_29:
        revert(); // empty
        // block_31:
        revert(); // empty
        while (false /* TODO: evm.lt200 */) {
          // block_41:
          if (!false /* TODO: evm.branch.cond230.not */) {
            break;
          }
          // block_43:
          if (!false /* TODO: evm.branch.cond252.not */) {
            break;
          }
          // block_45:
          emit Event_0xB5A3E9571E367979A4A14DE42B248D0837C26FD8E879846062ABCF7CEE171273(0 /* TODO: unresolved value: evm.mload321 */ & (uint256(1) << uint256(160)) + uint256(type(uint256).max), 0 /* TODO: unresolved value: evm.mload328 */ & uint256(4294967295) << uint256(224)); // TODO: recover event signature
          continue;
        }
        // block_42:
        revert(); // error_string, selector=0x8c379a0, string_length=51
        // block_44:
        revert(); // error_string, selector=0x8c379a0, string_length=54
    }

    function private__0xaf9_0xaf9(uint256 arg0, uint256 arg1) internal {
        // block_0:
    }

    function private__0xb14_0xb14(uint256 arg0, uint256 arg1) internal {
        // block_0:
    }

    function private__0xb2f_0xb2f(uint256 arg0, uint256 arg1, uint256 arg2) internal {
        // block_0:
    }

    function private__0xcc3_0xcc3(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return (arg0 << uint256(5)) + uint256(32);
    }

    function private__0xd1b_0xd1b(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return (arg1 << uint256(5)) + arg0 + uint256(32);
    }
}
