contract Decompiled {
    event Event_0x8CA1D928F1D72493A6B78C4F74AABDE976BC37FFE2570F2A1CE5A8ABD3DDE0AA(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0x3303FACD24627943A92E9DC87CFBB34B15C49B726EEC3AD3487C16BE9AB8EFE8(uint256 indexed arg0, uint256 indexed arg1, uint256 indexed arg2);
    event Event_0xAC1777479F07F3E7C34DA8402139D54027A6A260CAAAE168BDEE825CA5580DC5(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0xC052130BC4EF84580DB505783484B067EA8B71B3BCA78A7E12DB7AEA8658F004(uint256 indexed arg0, uint256 indexed arg1, uint256 indexed arg2);
    event Event_0x4925EAFC82D0C4D67889898EEED64B18488AB19811E61620F387026DEC126A28(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0x1A1CD5483E52E60B9FF7F3B9D1DB3BBD9E9D21C6324AD3A8C79DBA9B75E62F4D(uint256 indexed arg0, uint256 indexed arg1);

    mapping(uint256 => uint256) public slot_1;
    mapping(uint256 => uint256) public slot_2;
    uint256[] public slot_3;
    uint256[] public slot_4;
    uint256 public slot_5;
    mapping(uint256 => uint256) public slot_57;
    mapping(uint256 => uint256) public slot_58;
    mapping(uint256 => uint256) public slot_59;
    uint256 public slot_6;

    function EMISSION_MANAGER() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(1461501637330902918203684832716283019655932542975) & uint256(1443110500457411796578838204131955442359735615893);
    }

    function REVISION() public returns (uint256 ret0) {
        // block_0:
        return 1;
    }

    function public_0x955c2ad7() public {
        // block_0:
        require(msg.data.length > 35);
        // block_1:
        require(!false /* TODO: evm.gt.i.not */);
        // block_2:
        require(false /* TODO: evm.slt8.i */);
        // block_3:
        // block_13:
        revert(); // empty
        if (false /* TODO: evm.gt32.i.not */) {
          // goto block_13
        }
        else {
          // goto block_4
        }
        while (false /* TODO: evm.lt38.i.not */) {
          // block_6:
          if (!false /* TODO: evm.slt44.i */) {
            break;
          }
          // block_7:
          if (!false /* TODO: evm.eq.i.i */) {
            break;
          }
          // block_9:
          if (!false /* TODO: evm.eq.i3.i */) {
            break;
          }
          continue;
        }
        // block_8:
        revert(); // empty
        // block_10:
        revert(); // empty
        // block_12:
        revert(); // empty
    }

    function claimAllRewardsOnBehalf(address arg0, address arg1, address arg2) public {
        // block_0:
        if (msg.data.length > 99) {
          // goto block_1
        }
        // goto block_9
        // block_1:
        if (!(uint256(uint160(arg0)) > (uint256(1) << uint256(64)) + uint256(type(uint256).max))) {
          // goto block_2
        }
        // goto block_8
        // block_2:
        if (uint256(uint160(arg0)) + uint256(35) < msg.data.length) {
          // goto block_3
        }
        // goto block_7
        // block_3:
        if (!false /* TODO: evm.gt.i.i.not */) {
          // goto block_4
        }
        // goto block_6
        // block_4:
        if (false /* TODO: evm.gt8.i.i.not */) {
          // goto block_5
        }
        // goto block_10
        // block_5:
        revert(); // empty
        // block_6:
        revert(); // empty
        // block_7:
        revert(); // empty
        // block_8:
        revert(); // empty
        // block_9:
        revert(); // empty
        // block_10:
        private__0x2edf_0x2edf(uint256(uint160(arg1)), 12975);
        private__0x2edf_0x2edf(uint256(uint160(arg2)), 955656);
        if (uint256(uint160(msg.sender)) == slot_57[uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] & (uint256(1) << uint256(160)) + uint256(type(uint256).max)) {
          // goto block_11
        }
        // goto block_44
        // block_11:
        if (!(uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
          // goto block_12
        }
        // goto block_43
        // block_12:
        if (uint256(uint160(arg2)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // goto block_42
        }
        // block_13:
        if (!(slot_3.length == 0)) {
          // block_14:
        }
        // block_15:
        if (slot_3.length == 0) {
          // goto block_17
        }
        // block_16:
        // block_17:
        if (!false /* TODO: evm.iszero3.i.i.i */) {
          while (!false /* TODO: evm.branch.cond15.i.i.i.not */) {
            // block_18:
          }
        }
        // block_19:
        while (false /* TODO: evm.lt.i.i.i.not */) {
          // block_21:
          if (false /* TODO: evm.iszero54.i.i.i.not */) {
            // goto block_25
          }
          // block_22:
          if (!false /* TODO: evm.slt.i.i.i */) {
            // goto block_24
          }
          // block_23:
          continue;
        }
        // block_24:
        revert(); // empty
        // block_25:
        revert(); // returndata_forward
        while (true) {
          // block_27:
          if (false /* TODO: evm.lt.i.i.not */) {
            // goto block_34
          }
          break;
          // block_34:
          while (false /* TODO: evm.lt139.i.i.not */) {
            // block_37:
            if (false /* TODO: evm.eq159.i.i.not */) {
              // block_38:
            }
            // block_39:
            if (false /* TODO: evm.iszero212.i.i */) {
              // goto block_41
            }
            // block_40:
            // block_41:
            continue;
          }
          // block_36:
          continue;
        }
        while (false /* TODO: evm.lt31.i.i.not */) {
          // block_29:
          if (false /* TODO: evm.iszero66.i.i.not */) {
            break;
          }
          // block_30:
          if (false /* TODO: evm.iszero79.i.i.not */) {
            break;
          }
          // block_31:
          emit Event_0xC052130BC4EF84580DB505783484B067EA8B71B3BCA78A7E12DB7AEA8658F004((uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg1)), (uint256(1) << uint256(160)) + uint256(type(uint256).max) & 0 /* TODO: unresolved value: evm.mload93.i.i */, (uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg2))); // TODO: recover event signature
          continue;
        }
        // block_32:
        require(false, "TRANSFER_ERROR");
        // block_33:
        revert(); // returndata_forward
        // block_42:
        revert(); // error_string, selector=0x8c379a0
        // block_43:
        require(false, "INVALID_USER_ADDRESS");
        // block_44:
        require(false, "CLAIMER_UNAUTHORIZED");
    }

    function claimAllRewardsToSelf(address arg0) public {
        // block_0:
        if (msg.data.length > 35) {
          // goto block_1
        }
        // goto block_9
        // block_1:
        if (!(uint256(uint160(arg0)) > (uint256(1) << uint256(64)) + uint256(type(uint256).max))) {
          // goto block_2
        }
        // goto block_8
        // block_2:
        if (uint256(uint160(arg0)) + uint256(35) < msg.data.length) {
          // goto block_3
        }
        // goto block_7
        // block_3:
        if (!false /* TODO: evm.gt.i.i.not */) {
          // goto block_4
        }
        // goto block_6
        // block_4:
        if (false /* TODO: evm.gt8.i.i.not */) {
          // goto block_5
        }
        // goto block_10
        // block_5:
        revert(); // empty
        // block_6:
        revert(); // empty
        // block_7:
        revert(); // empty
        // block_8:
        revert(); // empty
        // block_9:
        revert(); // empty
        // block_10:
        if (!(slot_3.length == 0)) {
          // block_11:
        }
        // block_12:
        if (slot_3.length == 0) {
          // goto block_14
        }
        // block_13:
        // block_14:
        if (!false /* TODO: evm.iszero3.i.i.i */) {
          while (!false /* TODO: evm.branch.cond15.i.i.i.not */) {
            // block_15:
          }
        }
        // block_16:
        while (false /* TODO: evm.lt.i.i.i.not */) {
          // block_18:
          if (false /* TODO: evm.iszero54.i.i.i.not */) {
            // goto block_22
          }
          // block_19:
          if (!false /* TODO: evm.slt.i.i.i */) {
            // goto block_21
          }
          // block_20:
          continue;
        }
        // block_21:
        revert(); // empty
        // block_22:
        revert(); // returndata_forward
        while (true) {
          // block_24:
          if (false /* TODO: evm.lt.i.i.not */) {
            // goto block_31
          }
          break;
          // block_31:
          while (false /* TODO: evm.lt139.i.i.not */) {
            // block_34:
            if (false /* TODO: evm.eq159.i.i.not */) {
              // block_35:
            }
            // block_36:
            if (false /* TODO: evm.iszero212.i.i */) {
              // goto block_38
            }
            // block_37:
            // block_38:
            continue;
          }
          // block_33:
          continue;
        }
        while (false /* TODO: evm.lt31.i.i.not */) {
          // block_26:
          if (false /* TODO: evm.iszero66.i.i.not */) {
            break;
          }
          // block_27:
          if (false /* TODO: evm.iszero79.i.i.not */) {
            break;
          }
          // block_28:
          emit Event_0xC052130BC4EF84580DB505783484B067EA8B71B3BCA78A7E12DB7AEA8658F004((uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(msg.sender)), (uint256(1) << uint256(160)) + uint256(type(uint256).max) & 0 /* TODO: unresolved value: evm.mload93.i.i */, (uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(msg.sender))); // TODO: recover event signature
          continue;
        }
        // block_29:
        require(false, "TRANSFER_ERROR");
        // block_30:
        revert(); // returndata_forward
    }

    function claimAllRewards(address arg0, address arg1) public {
        // block_0:
        if (msg.data.length > 67) {
          // goto block_1
        }
        // goto block_9
        // block_1:
        if (!(uint256(uint160(arg0)) > (uint256(1) << uint256(64)) + uint256(type(uint256).max))) {
          // goto block_2
        }
        // goto block_8
        // block_2:
        if (uint256(uint160(arg0)) + uint256(35) < msg.data.length) {
          // goto block_3
        }
        // goto block_7
        // block_3:
        if (!false /* TODO: evm.gt.i.i.not */) {
          // goto block_4
        }
        // goto block_6
        // block_4:
        if (false /* TODO: evm.gt8.i.i.not */) {
          // goto block_5
        }
        // goto block_10
        // block_5:
        revert(); // empty
        // block_6:
        revert(); // empty
        // block_7:
        revert(); // empty
        // block_8:
        revert(); // empty
        // block_9:
        revert(); // empty
        // block_10:
        private__0x2edf_0x2edf(uint256(uint160(arg1)), 955492);
        if (uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // goto block_40
        }
        // block_11:
        if (!(slot_3.length == 0)) {
          // block_12:
        }
        // block_13:
        if (slot_3.length == 0) {
          // goto block_15
        }
        // block_14:
        // block_15:
        if (!false /* TODO: evm.iszero3.i.i.i */) {
          while (!false /* TODO: evm.branch.cond15.i.i.i.not */) {
            // block_16:
          }
        }
        // block_17:
        while (false /* TODO: evm.lt.i.i.i.not */) {
          // block_19:
          if (false /* TODO: evm.iszero54.i.i.i.not */) {
            // goto block_23
          }
          // block_20:
          if (!false /* TODO: evm.slt.i.i.i */) {
            // goto block_22
          }
          // block_21:
          continue;
        }
        // block_22:
        revert(); // empty
        // block_23:
        revert(); // returndata_forward
        while (true) {
          // block_25:
          if (false /* TODO: evm.lt.i.i.not */) {
            // goto block_32
          }
          break;
          // block_32:
          while (false /* TODO: evm.lt139.i.i.not */) {
            // block_35:
            if (false /* TODO: evm.eq159.i.i.not */) {
              // block_36:
            }
            // block_37:
            if (false /* TODO: evm.iszero212.i.i */) {
              // goto block_39
            }
            // block_38:
            // block_39:
            continue;
          }
          // block_34:
          continue;
        }
        while (false /* TODO: evm.lt31.i.i.not */) {
          // block_27:
          if (false /* TODO: evm.iszero66.i.i.not */) {
            break;
          }
          // block_28:
          if (false /* TODO: evm.iszero79.i.i.not */) {
            break;
          }
          // block_29:
          emit Event_0xC052130BC4EF84580DB505783484B067EA8B71B3BCA78A7E12DB7AEA8658F004((uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(msg.sender)), (uint256(1) << uint256(160)) + uint256(type(uint256).max) & 0 /* TODO: unresolved value: evm.mload93.i.i */, (uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg1))); // TODO: recover event signature
          continue;
        }
        // block_30:
        require(false, "TRANSFER_ERROR");
        // block_31:
        revert(); // returndata_forward
        // block_40:
        revert(); // error_string, selector=0x8c379a0
    }

    function claimRewardsOnBehalf(address arg0, uint256 arg1, address arg2, address arg3, address arg4) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 163);
        // block_1:
        require(uint256(uint160(arg0)) <= (uint256(1) << uint256(64)) + uint256(type(uint256).max));
        // block_2:
        require(uint256(uint160(arg0)) + uint256(35) < msg.data.length);
        // block_3:
        require(!false /* TODO: evm.gt.i.i.not */);
        // block_4:
        require(!false /* TODO: evm.gt8.i.i.not */);
        // block_10:
        private__0x2edf_0x2edf(uint256(uint160(arg2)), 12435);
        private__0x2edf_0x2edf(uint256(uint160(arg3)), 12451);
        private__0x2edf_0x2edf(uint256(uint160(arg4)), 12467);
        require(uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_57[uint256(uint160(arg2)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)], "CLAIMER_UNAUTHORIZED");
        // block_11:
        require(uint256(uint160(arg2)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) != 0, "INVALID_USER_ADDRESS");
        // block_12:
        if (uint256(uint160(arg3)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // block_30:
          revert(); // error_string, selector=0x8c379a0
        }
        // block_13:
        if (arg1 == 0) {
          // goto block_33
        }
        // goto block_14
        // block_14:
        while (!false /* TODO: evm.branch.cond15.i.i.i.not */) {
          // block_15:
        }
        while (!false /* TODO: evm.iszero54.i.i.i.not */) {
          // block_19:
          if (!false /* TODO: evm.slt.i.i.i */) {
            break;
          }
          // block_16:
          // block_17:
        }
        // block_21:
        revert(); // empty
        while (!false /* TODO: evm.gt.i.i12 */) {
          // block_23:
          // block_24:
          if (!false /* TODO: evm.lt.i.i.not */) {
            // goto block_27
          }
          // goto block_25
        }
        // block_26:
        // block_27:
        // block_26:
        // block_27:
        if (false /* TODO: evm.branch.cond42.i.i.not */) {
          // goto block_33
        }
        // block_28:
        emit Event_0xC052130BC4EF84580DB505783484B067EA8B71B3BCA78A7E12DB7AEA8658F004(uint256(uint160(arg2)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max), (uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg4)), uint256(uint160(arg3)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)); // TODO: recover event signature
        // block_33:
        return 0 /* TODO: unresolved value: private.call1.i.reload */;
    }

    function claimRewardsToSelf(address arg0, uint256 arg1, address arg2) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 99);
        // block_1:
        require(uint256(uint160(arg0)) <= (uint256(1) << uint256(64)) + uint256(type(uint256).max));
        // block_2:
        require(uint256(uint160(arg0)) + uint256(35) < msg.data.length);
        // block_3:
        require(!false /* TODO: evm.gt.i.i.not */);
        // block_4:
        require(!false /* TODO: evm.gt8.i.i.not */);
        // block_10:
        private__0x2edf_0x2edf(uint256(uint160(arg2)), 955576);
        if (arg1 == 0) {
          // goto block_27
        }
        // goto block_11
        // block_11:
        while (!false /* TODO: evm.branch.cond15.i.i.i.not */) {
          // block_12:
        }
        while (!false /* TODO: evm.iszero54.i.i.i.not */) {
          // block_16:
          if (!false /* TODO: evm.slt.i.i.i */) {
            break;
          }
          // block_13:
          // block_14:
        }
        // block_18:
        revert(); // empty
        while (!false /* TODO: evm.gt.i.i8 */) {
          // block_20:
          // block_21:
          if (!false /* TODO: evm.lt.i.i.not */) {
            // goto block_24
          }
          // goto block_22
        }
        // block_23:
        // block_24:
        // block_23:
        // block_24:
        if (false /* TODO: evm.branch.cond42.i.i.not */) {
          // goto block_27
        }
        // block_25:
        emit Event_0xC052130BC4EF84580DB505783484B067EA8B71B3BCA78A7E12DB7AEA8658F004(uint256(uint160(msg.sender)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max), (uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg2)), uint256(uint160(msg.sender)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)); // TODO: recover event signature
        // block_27:
        return 0 /* TODO: unresolved value: private.call1.i.reload */;
    }

    function claimRewards(address arg0, uint256 arg1, address arg2, address arg3) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 131);
        // block_1:
        require(uint256(uint160(arg0)) <= (uint256(1) << uint256(64)) + uint256(type(uint256).max));
        // block_2:
        require(uint256(uint160(arg0)) + uint256(35) < msg.data.length);
        // block_3:
        require(!false /* TODO: evm.gt.i.i.not */);
        // block_4:
        require(!false /* TODO: evm.gt8.i.i.not */);
        // block_10:
        private__0x2edf_0x2edf(uint256(uint160(arg2)), 12230);
        private__0x2edf_0x2edf(uint256(uint160(arg3)), 12246);
        if (uint256(uint160(arg2)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // block_28:
          revert(); // error_string, selector=0x8c379a0
        }
        // block_11:
        if (arg1 == 0) {
          // goto block_29
        }
        // goto block_12
        // block_12:
        while (!false /* TODO: evm.branch.cond15.i.i.i.not */) {
          // block_13:
        }
        while (!false /* TODO: evm.iszero54.i.i.i.not */) {
          // block_17:
          if (!false /* TODO: evm.slt.i.i.i */) {
            break;
          }
          // block_14:
          // block_15:
        }
        // block_19:
        revert(); // empty
        while (!false /* TODO: evm.gt.i.i11 */) {
          // block_21:
          // block_22:
          if (!false /* TODO: evm.lt.i.i.not */) {
            // goto block_25
          }
          // goto block_23
        }
        // block_24:
        // block_25:
        // block_24:
        // block_25:
        if (false /* TODO: evm.branch.cond42.i.i.not */) {
          // goto block_29
        }
        // block_26:
        emit Event_0xC052130BC4EF84580DB505783484B067EA8B71B3BCA78A7E12DB7AEA8658F004(uint256(uint160(msg.sender)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max), (uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg3)), uint256(uint160(arg2)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)); // TODO: recover event signature
        // block_29:
        return 0 /* TODO: unresolved value: private.call1.i.reload */;
    }

    function fallback() public {
        // block_0:
        revert(); // empty
    }

    function getAllUserRewards(address arg0, address arg1) public {
        // block_0:
        require(msg.data.length > 67);
        if (!(msg.data.length > 67)) {
          // goto block_9
        }
        // block_1:
        require(uint256(uint160(arg0)) <= (uint256(1) << uint256(64)) + uint256(type(uint256).max));
        // block_2:
        require(uint256(uint160(arg0)) + uint256(35) < msg.data.length);
        // block_3:
        require(!false /* TODO: evm.gt.i.i.not */);
        // block_4:
        require(!false /* TODO: evm.gt8.i.i.not */);
        // block_10:
        private__0x2edf_0x2edf(uint256(uint160(arg1)), 955492);
        if (!false /* TODO: evm.iszero3.i.i */) {
          while (!false /* TODO: evm.branch.cond15.i.i.not */) {
            // block_11:
          }
        }
        // block_12:
        while (false /* TODO: evm.lt.i.i.not */) {
          // block_14:
          if (false /* TODO: evm.iszero54.i.i.not */) {
            // goto block_18
          }
          // block_15:
          if (!false /* TODO: evm.slt.i.i9 */) {
            // goto block_17
          }
          // block_16:
          continue;
        }
        // goto block_21
        // block_17:
        revert(); // empty
        // block_18:
        revert(); // returndata_forward
        // block_19:
        if (!(slot_3.length == 0)) {
          // block_20:
        }
        // block_21:
        if (false /* TODO: evm.iszero21.i */) {
          // goto block_23
        }
        // block_22:
        // block_23:
        while (true) {
          // block_24:
          if (!false /* TODO: evm.lt.i.not */) {
            break;
          }
          while (false /* TODO: evm.lt35.i.not */) {
            // block_27:
            if (!false /* TODO: evm.eq.i.not */) {
              // block_28:
            }
            // block_29:
          }
          // block_26:
          continue;
        }
        // block_30:
        // TODO: recover remaining body
    }

    function getAssetDecimals(address arg0) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        private__0x2edf_0x2edf(uint256(uint160(arg0)), 955454);
        return slot_1[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0))] / (uint256(1) << uint256(128)) & uint256(255);
    }

    function getAssetIndex(address arg0, address arg1) public returns (uint256 ret0, uint256 ret1) {
        // block_0:
        require(msg.data.length > 67);
        // block_2:
        private__0x2edf_0x2edf(uint256(uint160(arg0)), 12050);
        private__0x2edf_0x2edf(uint256(uint160(arg1)), 12066);
        return (0 /* TODO: unresolved value: private.ret4 */, 0 /* TODO: unresolved value: private.ret3 */);
    }

    function getClaimer(address arg0) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        private__0x2edf_0x2edf(uint256(uint160(arg0)), 955454);
        return slot_57[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0))] & (uint256(1) << uint256(160)) + uint256(type(uint256).max) & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function getDistributionEnd(address arg0, address arg1) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 67);
        // block_2:
        private__0x2edf_0x2edf(uint256(uint160(arg0)), 12050);
        private__0x2edf_0x2edf(uint256(uint160(arg1)), 12066);
        return private__0x5bf_0x5bf(uint256(uint160(arg1)), uint256(uint160(arg0)), 412500);
    }

    function getEmissionManager() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(1461501637330902918203684832716283019655932542975) & uint256(1443110500457411796578838204131955442359735615893);
    }

    function getRewardOracle(address arg0) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        private__0x2edf_0x2edf(uint256(uint160(arg0)), 955454);
        return slot_59[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0))] & (uint256(1) << uint256(160)) + uint256(type(uint256).max) & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function getRewardsByAsset(address arg0) public {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        private__0x2edf_0x2edf(uint256(uint160(arg0)), 955454);
        if (!(slot_1[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] & (uint256(1) << uint256(128)) + uint256(type(uint256).max) == 0)) {
          // block_3:
        }
        // block_4:
        while (false /* TODO: evm.lt.i.not */) {
          // block_6:
        }
        // block_7:
    }

    function getRewardsData(address arg0, address arg1) public returns (uint256 ret0, uint256 ret1, uint256 ret2, uint256 ret3) {
        // block_0:
        require(msg.data.length > 67);
        // block_2:
        private__0x2edf_0x2edf(uint256(uint160(arg0)), 12050);
        private__0x2edf_0x2edf(uint256(uint160(arg1)), 12066);
        return (slot_1[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0))] & (uint256(1) << uint256(104)) + uint256(type(uint256).max), slot_1[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0))] / (uint256(1) << uint256(104)) & (uint256(1) << uint256(88)) + uint256(type(uint256).max), slot_1[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0))] / (uint256(1) << uint256(192)) & uint256(4294967295), slot_1[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0))] / (uint256(1) << uint256(224)) & uint256(4294967295));
    }

    function getRewardsList() public {
        // block_0:
    }

    function getTransferStrategy(address arg0) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        private__0x2edf_0x2edf(uint256(uint160(arg0)), 955454);
        return slot_58[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0))] & (uint256(1) << uint256(160)) + uint256(type(uint256).max) & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function getUserAccruedRewards(address arg0, address arg1) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 67);
        // block_2:
        private__0x2edf_0x2edf(uint256(uint160(arg0)), 12050);
        private__0x2edf_0x2edf(uint256(uint160(arg1)), 12066);
        return 0 /* TODO: unresolved value: private__0xf06_0xf06 */;
    }

    function getUserAssetIndex(address arg0, address arg1, address arg2) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 99);
        // block_2:
        private__0x2edf_0x2edf(uint256(uint160(arg0)), 12754);
        private__0x2edf_0x2edf(uint256(uint160(arg1)), 12770);
        private__0x2edf_0x2edf(uint256(uint160(arg2)), 955534);
        return (uint256(1) << uint256(104)) + uint256(type(uint256).max) & slot_1[uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)];
    }

    function getUserRewards(address arg0, address arg1, address arg2) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 99);
        if (!(msg.data.length > 99)) {
          // goto block_9
        }
        // block_1:
        require(uint256(uint160(arg0)) <= (uint256(1) << uint256(64)) + uint256(type(uint256).max));
        // block_2:
        require(uint256(uint160(arg0)) + uint256(35) < msg.data.length);
        // block_3:
        require(!false /* TODO: evm.gt.i.i.not */);
        // block_4:
        require(!false /* TODO: evm.gt8.i.i.not */);
        // block_10:
        private__0x2edf_0x2edf(uint256(uint160(arg1)), 12975);
        private__0x2edf_0x2edf(uint256(uint160(arg2)), 955656);
        if (!false /* TODO: evm.iszero3.i.i */) {
          while (!false /* TODO: evm.branch.cond15.i.i.not */) {
            // block_11:
          }
        }
        // block_12:
        while (false /* TODO: evm.lt.i.i.not */) {
          // block_14:
          if (false /* TODO: evm.iszero54.i.i.not */) {
            break;
          }
          // block_15:
          if (!false /* TODO: evm.slt.i.i9 */) {
            break;
          }
          // block_16:
          continue;
        }
        // block_17:
        revert(); // empty
        // block_18:
        revert(); // returndata_forward
    }

    function handleAction(address arg0, uint256 arg1, uint256 arg2) public {
        // block_0:
        require(msg.data.length > 99);
        // block_1:
        private__0x2edf_0x2edf(uint256(uint160(arg0)), 12328);
        private__0x643_0x643(arg2, arg1, uint256(uint160(arg0)), 547651);
    }

    function initialize(address arg0) public {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        private__0x2edf_0x2edf(uint256(uint160(arg0)), 955454);
    }

    function setClaimer(address arg0, address arg1) public {
        // block_0:
        require(msg.data.length > 67);
        // block_2:
        private__0x2edf_0x2edf(uint256(uint160(arg0)), 12050);
        private__0x2edf_0x2edf(uint256(uint160(arg1)), 12066);
        if ((uint256(1) << uint256(160)) + uint256(1461501637330902918203684832716283019655932542975) & uint256(1443110500457411796578838204131955442359735615893) == uint256(uint160(msg.sender))) {
          // block_3:
          emit Event_0x4925EAFC82D0C4D67889898EEED64B18488AB19811E61620F387026DEC126A28((uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0)), uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)); // TODO: recover event signature
        }
        else {
          // block_4:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function setDistributionEnd(address arg0, address arg1, uint32 arg2) public {
        // block_0:
        require(msg.data.length > 99);
        // block_1:
        private__0x2edf_0x2edf(uint256(uint160(arg0)), 13547);
        private__0x2edf_0x2edf(uint256(uint160(arg1)), 13563);
        require(arg2 < 4294967296);
        // block_4:
        if ((uint256(1) << uint256(160)) + uint256(1461501637330902918203684832716283019655932542975) & uint256(1443110500457411796578838204131955442359735615893) == uint256(uint160(msg.sender))) {
          // block_5:
          emit Event_0xAC1777479F07F3E7C34DA8402139D54027A6A260CAAAE168BDEE825CA5580DC5((uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0)), uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)); // TODO: recover event signature
        }
        else {
          // block_6:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function setEmissionPerSecond(address arg0, address arg1, uint88 arg2) public {
        // block_0:
        require(msg.data.length > 99);
        // block_1:
        private__0x2edf_0x2edf(uint256(uint160(arg0)), 13621);
        require(uint256(uint160(arg1)) <= (uint256(1) << uint256(64)) + uint256(type(uint256).max));
        // block_2:
        require(uint256(uint160(arg1)) + uint256(35) < msg.data.length);
        // block_3:
        require(!false /* TODO: evm.gt.i.i.not */);
        // block_4:
        require(!false /* TODO: evm.gt8.i.i.not */);
        // block_8:
        require(arg2 <= (uint256(1) << uint256(64)) + uint256(type(uint256).max));
        // block_9:
        require(arg2 + uint256(35) < msg.data.length);
        // block_10:
        require(!false /* TODO: evm.gt.i9.i.not */);
        // block_11:
        require(!false /* TODO: evm.gt8.i19.i.not */);
        // block_18:
        if (!((uint256(1) << uint256(160)) + uint256(1461501637330902918203684832716283019655932542975) & uint256(1443110500457411796578838204131955442359735615893) == uint256(uint160(msg.sender)))) {
          // block_32:
          revert(); // error_string, selector=0x8c379a0
        }
        // block_19:
        // block_31:
        require(false, "INVALID_INPUT");
        if (!false /* TODO: evm.eq1.i */) {
          // goto block_31
        }
        while (false /* TODO: evm.lt.i.not */) {
          // block_21:
          if (slot_1[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] / (uint256(1) << uint256(128)) & uint256(255) == 0) {
            break;
          }
          // block_22:
          if (slot_1[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] / (uint256(1) << uint256(192)) & uint256(4294967295) == 0) {
            break;
          }
          // block_23:
          if (false /* TODO: evm.iszero42.i.not */) {
            break;
          }
          // block_24:
          if (!false /* TODO: evm.eq.i.i.i */) {
            break;
          }
          // block_26:
          if (!false /* TODO: evm.eq.i.i6.i */) {
            break;
          }
          // block_28:
          emit Event_0xAC1777479F07F3E7C34DA8402139D54027A6A260CAAAE168BDEE825CA5580DC5((uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0)), (uint256(1) << uint256(160)) + uint256(type(uint256).max) & 0 /* TODO: unresolved value: evm.calldataload.i1.i.load */); // TODO: recover event signature
          continue;
        }
        // block_25:
        revert(); // empty
        // block_27:
        revert(); // empty
        // block_29:
        revert(); // returndata_forward
        // block_30:
        revert(); // error_string, selector=0x8c379a0, string_length=27
    }

    function setRewardOracle(address arg0, address arg1) public {
        // block_0:
        require(msg.data.length > 67);
        // block_2:
        private__0x2edf_0x2edf(uint256(uint160(arg0)), 12050);
        private__0x2edf_0x2edf(uint256(uint160(arg1)), 12066);
    }

    function setTransferStrategy(address arg0, address arg1) public {
        // block_0:
        require(msg.data.length > 67);
        // block_2:
        private__0x2edf_0x2edf(uint256(uint160(arg0)), 12050);
        private__0x2edf_0x2edf(uint256(uint160(arg1)), 12066);
    }

    function private__0x2c17_0x2c17(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        if (arg0 > (uint256(1) << uint256(128)) + uint256(type(uint256).max)) {
          // block_2:
          revert(); // error_string, selector=0x8c379a0, string_length=39
        }
        else {
          // block_1:
          return arg0;
        }
    }

    function private__0x2e56_0x2e56(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3, uint256 arg4) internal returns (uint256 ret0) {
        // block_0:
        return private__0x37e3_0x37e3(arg3, private__0x37a8_0x37a8(arg2, arg1, 11875), 11885) / arg0;
    }

    function private__0x2edf_0x2edf(uint256 arg0, uint256 arg1) internal {
        // block_0:
        require(arg0 & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_1:
    }

    function private__0x3594_0x3594(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(96);
    }

    function private__0x35ec_0x35ec(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + arg1;
    }

    function private__0x3604_0x3604(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(1);
    }

    function private__0x361f_0x361f(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(96);
    }

    function private__0x364e_0x364e(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return (arg0 & (uint256(1) << uint256(128)) + uint256(type(uint256).max)) + uint256(1);
    }

    function private__0x37a8_0x37a8(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 - arg1;
    }

    function private__0x37e3_0x37e3(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 * arg1;
    }

    function private__0x5bf_0x5bf(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return slot_1[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & arg1] / (uint256(1) << uint256(224)) & uint256(4294967295);
    }

    function private__0x643_0x643(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
        // block_0:
    }
}
