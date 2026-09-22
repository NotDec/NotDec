contract Decompiled {
    function public_0xc32e370e() public returns (bool ret0) {
        // block_0:
        require(msg.data.length > 291);
        // block_1:
        require(msg.data.length > 35);
        // block_2:
        // block_5:
        revert(); // empty
        if (!(msg.data.length > 67)) {
          // goto block_5
        }
        while (!false /* TODO: evm.lt.i.i */) {
          // block_4:
        }
        // goto block_32
        while (true) {
          // block_9:
          if (false /* TODO: evm.lt.i */) {
            // goto block_10
          }
          // block_25:
          if (false /* TODO: evm.slt.i36.i */) {
            // goto block_26
          }
          // goto block_30
          // block_26:
          if (false /* TODO: evm.gt.i42.i.not */) {
            // goto block_29
          }
          while (false /* TODO: evm.lt.i51.i.not */) {
            // block_28:
          }
          // block_31:
          continue;
        }
        // goto block_14
        while (!false /* TODO: evm.lt.i17.i */) {
          // block_13:
        }
        // block_10:
        require(msg.data.length > 227);
        // block_11:
        // block_14:
        revert(); // empty
        if (msg.data.length > 259) {
          // goto block_12
        }
        else {
          // goto block_14
        }
        // block_16:
        require(false /* TODO: evm.gt18.i */);
        // block_17:
        require(false /* TODO: evm.slt.i28.i */);
        // block_18:
        // block_22:
        revert(); // empty
        if (false /* TODO: evm.gt15.i.i.not */) {
          // goto block_22
        }
        while (true) {
          // block_19:
          // block_20:
          if (!false /* TODO: evm.lt21.i.i.not */) {
            break;
          }
          continue;
        }
        // block_29:
        revert(); // empty
        // block_30:
        revert(); // empty
        // block_7:
        require(msg.data.length > 99);
        // block_8:
        // block_32:
        revert(); // empty
        if (msg.data.length > 195) {
          // goto block_9
        }
        else {
          // goto block_32
        }
        // block_35:
        while (!false /* TODO: evm.branch.cond.i.i.not */) {
          // block_36:
        }
        // block_37:
        if (!false /* TODO: evm.eq.i.i */) {
          // goto block_72
        }
        while (true) {
          // block_38:
          // block_39:
          if (!false /* TODO: evm.lt163.i.i.not */) {
            // goto block_40
          }
          // block_62:
          if (false /* TODO: evm.lt313.i.i */) {
            // goto block_63
          }
          // goto block_71
          // block_63:
          if (!false /* TODO: evm.eq324.i.i */) {
            // goto block_64
          }
          continue;
          // block_64:
          if (false /* TODO: evm.lt349.i.i */) {
            // goto block_65
          }
          // goto block_70
          // block_65:
          if (false /* TODO: evm.branch.cond360.i.i.not */) {
            // goto block_69
          }
          // block_66:
          if (!false /* TODO: evm.branch.cond379.i.i.not */) {
            continue;
          }
          // goto block_68
        }
        // block_40:
        if (false /* TODO: evm.branch.cond.i.i.i.not */) {
          revert(); // custom_error_candidate, selector=0x9bde339, args=0
        }
        // block_42:
        while (!false /* TODO: evm.branch.cond184.i.i.not */) {
          // block_43:
        }
        // block_44:
        while (!false /* TODO: evm.branch.cond191.i.i.not */) {
          // block_45:
        }
        // block_46:
        if (false /* TODO: evm.iszero197.i.i */) {
          // block_47:
        }
        if (false /* TODO: evm.iszero204.i.i */) {
          // goto block_50
        }
        // block_48:
        if (false /* TODO: evm.gt.i.i14 */) {
          // block_51:
          revert(); // custom_error_candidate, selector=0x9bde339, args=0
        }
        // block_49:
        if (false /* TODO: evm.lt216.i.i */) {
          // goto block_51
        }
        // goto block_50
        // block_50:
        // block_53:
        require(false /* TODO: evm.eq.i.i.i */, "pairing-lengths-failed");
        // block_54:
        // block_55:
        while (true) {
          // block_56:
          // block_57:
          if (!false /* TODO: evm.lt.i.i.i.not */) {
            break;
          }
          continue;
        }
        // goto block_73
        // block_68:
        revert(); // custom_error_candidate, selector=0x9bde339, args=0
        // block_69:
        revert(); // custom_error_candidate, selector=0x9bde339, args=0
        // block_70:
        revert(); // custom_error_candidate, selector=0x9bde339, args=0
        // block_71:
        revert(); // error_string, selector=0x8c379a0, string_length=31
        // block_72:
        require(false, "verifier-bad-input");
        // block_58:
        if (!false /* TODO: evm.iszero23.i.i.i */) {
          // block_59:
        }
        // block_73:
        return 0 /* TODO: unresolved value: private.call29648.i.i.reload */ != 0;
    }

    function fallback() public {
        // block_0:
        revert(); // empty
    }

    function private__0x14a0_0x14a0(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(1);
    }

    function private__0x1488_0x1488(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + arg1;
    }

    function private__0x14d0_0x14d0(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 * arg1;
    }
}
