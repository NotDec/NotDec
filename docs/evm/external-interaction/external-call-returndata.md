# External call 和 returndata

## 简介

Solidity 合约可以调用其他合约：

```solidity
interface IToken {
    function transfer(address to, uint256 amount) external returns (bool);
}

contract C {
    function pay(address token, address to, uint256 amount) external {
        IToken(token).transfer(to, amount);
    }
}
```

也可以写低级调用：

```solidity
(bool ok, bytes memory ret) = target.call(data);
```

EVM 里对应的是 `CALL`、`STATICCALL`、`DELEGATECALL`、`CALLCODE` 等指令。

## 编译过程与低层模式

外部调用通常分成几步：

1. 在 memory 里准备 call data。
2. 执行 `CALL` 或同类指令。
3. 检查 success 标志。
4. 处理 returndata。

普通接口调用需要 ABI 编码 call data：

```text
ptr = mload(0x40)
mstore(ptr, selector("transfer(address,uint256)") << 224)
mstore(ptr + 4, to)
mstore(ptr + 36, amount)
```

然后执行 call：

```text
success = CALL(gas, token, value, ptr, 68, out_ptr, out_size)
```

`CALL` 的参数大概是：

```text
gas
target address
value
input memory offset
input size
output memory offset
output size
```

`STATICCALL` 不允许修改状态，也不能发送 ETH：

```text
success = STATICCALL(gas, target, in_ptr, in_size, out_ptr, out_size)
```

`DELEGATECALL` 使用当前合约的 storage、`msg.sender` 和 `msg.value`，常见于代理合约：

```text
success = DELEGATECALL(gas, impl, in_ptr, in_size, out_ptr, out_size)
```

如果高级接口调用失败，Solidity 常常会把 returndata 原样冒泡：

```text
if success == 0:
    size = RETURNDATASIZE
    RETURNDATACOPY(0, 0, size)
    REVERT(0, size)
```

如果调用成功，编译器可能会解码返回值：

```text
RETURNDATACOPY(out_ptr, 0, RETURNDATASIZE)
ok = abi_decode_bool(out_ptr)
```

## 例子详解

Solidity 源码：

```solidity
interface IToken {
    function transfer(address to, uint256 amount) external returns (bool);
}

contract C {
    function pay(address token, address to, uint256 amount) external {
        bool ok = IToken(token).transfer(to, amount);
        require(ok, "transfer failed");
    }
}
```

编译器要保证的语义是：

- 构造 `transfer(address,uint256)` 的 ABI call data。
- 调用 `token` 合约。
- 如果外部调用本身失败，把失败原因冒泡，或者按编译器策略 revert。
- 如果调用成功，解码返回的 bool。
- 如果 bool 是 false，再执行 `require` 的错误路径。

简化 EVM 伪指令：

```text
in_ptr = mload(0x40)
mstore(in_ptr, selector("transfer(address,uint256)") << 224)
mstore(in_ptr + 4, to)
mstore(in_ptr + 36, amount)

success = CALL(gas, token, 0, in_ptr, 68, 0, 32)

if success == 0:
    size = RETURNDATASIZE
    RETURNDATACOPY(0, 0, size)
    REVERT(0, size)

RETURNDATACOPY(out_ptr, 0, RETURNDATASIZE)
ok = mload(out_ptr)

if ok == 0:
    revert Error("transfer failed")
```

低级调用例子：

```solidity
function raw(address target, bytes calldata data) external returns (bytes memory) {
    (bool ok, bytes memory ret) = target.call(data);
    require(ok);
    return ret;
}
```

低级调用不会自动知道返回值类型，所以 returndata 通常作为 bytes 处理：

```text
copy calldata bytes into memory input buffer
success = CALL(gas, target, 0, in_ptr, in_size, 0, 0)
ret_size = RETURNDATASIZE
allocate bytes(ret_size)
RETURNDATACOPY(ret_ptr + 32, 0, ret_size)
```

需要记住的是：external call 不是一个单独 opcode 就结束了。它通常带着 input buffer、
success check、returndata copy、失败冒泡和返回值解码。
