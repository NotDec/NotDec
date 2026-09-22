contract Decompiled {
    event Event_0xCF34EF537AC33EE1AC626CA1587A0A7E8E51561E5514F8CB36AFA1C5102B3BAB();

    uint256 public slot_0;
    uint256 public slot_1;
    uint256 public slot_3;

    function public_0xeeeeeeee() public {
        // block_0:
    }

    function start() public {
        // block_0:
    }

    function tokenName() public {
        while (true) {
          // block_0:
          // block_1:
          if (!false /* TODO: evm.lt.not */) {
            break;
          }
          continue;
        }
        // goto block_4
        // block_2:
        if (!false /* TODO: evm.iszero11 */) {
          // block_3:
        }
        // block_4:
        // TODO: recover remaining body
    }

    function tokenSymbol() public {
        while (true) {
          // block_0:
          // block_1:
          if (!false /* TODO: evm.lt.not */) {
            break;
          }
          continue;
        }
        // goto block_4
        // block_2:
        if (!false /* TODO: evm.iszero11 */) {
          // block_3:
        }
        // block_4:
        // TODO: recover remaining body
    }

    function withdrawal() public {
        // block_0:
    }

    function zz2bf(uint256 arg0) public {
        // block_0:
        emit Event_0xCF34EF537AC33EE1AC626CA1587A0A7E8E51561E5514F8CB36AFA1C5102B3BAB(); // TODO: recover event signature
        if (private__0x4b7_0x4b7(786) == 0) {
          // block_1:
          if (false /* TODO: evm.iszero11.not */) {
            // block_3:
            revert(); // returndata_forward
          }
          else {
            // block_6:
          }
        }
        else {
          // block_4:
          if (false /* TODO: evm.iszero27.not */) {
            // block_5:
            revert(); // returndata_forward
          }
          else {
            // block_2:
          }
        }
        // TODO: recover remaining body
    }

    function private__0x4b7_0x4b7(uint256 arg0) internal returns (uint256 ret0) {
        // block_0:
        return address(this).balance > slot_3 ? 1 : 0;
    }

    function private__0xcd1_0xcd1(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        if (arg0 & uint256(254) > 9) {
          // block_1:
          require(arg0 & uint256(240) == 0);
          // block_3:
          return (uint256(43874346312576839672212443538448152585028080127215369968075725190498334277632) >> uint256(248)) + arg0 + uint256((type(uint256).max - 9)) << uint256(248);
        }
        else {
          // block_4:
          return (uint256(21711016731996786641919559689128982722488122124807605757398297001483711807488) >> uint256(248)) + arg0 << uint256(248);
        }
    }
}
