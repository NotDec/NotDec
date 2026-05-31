# Event log

## 简介

Solidity event 会写入交易日志：

```solidity
contract C {
    event Transfer(address indexed from, address indexed to, uint256 amount);

    function emitTransfer(address from, address to, uint256 amount) external {
        emit Transfer(from, to, amount);
    }
}
```

event 不会写 storage，也不会影响合约后续执行。它会生成 log，供链下程序读取。

EVM 用 `LOG0` 到 `LOG4` 指令写日志。数字表示 topic 数量。

## 编译过程与低层模式

event 编码分两部分：

- topic：最多 4 个，每个 32 字节。
- data：一段 memory buffer。

非 anonymous event 的 `topic0` 是事件签名 hash：

```text
keccak256("Transfer(address,address,uint256)")
```

`indexed` 参数放进 topic。非 indexed 参数放进 data buffer。

上面的例子里：

```solidity
event Transfer(address indexed from, address indexed to, uint256 amount);
```

topic 是：

```text
topic0 = event signature hash
topic1 = from
topic2 = to
```

data 是：

```text
amount
```

简化 EVM 伪指令：

```text
ptr = mload(0x40)
mstore(ptr, amount)
LOG3(ptr, 32, topic0, from, to)
```

如果 event 是 `anonymous`，就没有 topic0，indexed 参数从 topic0 开始放。

动态 indexed 参数比较特殊。比如：

```solidity
event E(string indexed name);
```

`name` 不会直接塞进 topic，而是先对它的 ABI 编码取 hash：

```text
topic = keccak256(encoded_name)
```

非 indexed 动态参数则放在 data buffer 里，按 ABI head/tail 编码。

## 例子详解

Solidity 源码：

```solidity
contract C {
    event Set(address indexed user, uint256 oldValue, uint256 newValue);

    function set(uint256 oldValue, uint256 newValue) external {
        emit Set(msg.sender, oldValue, newValue);
    }
}
```

编译器要保证的语义是：

- 日志里能看出这是 `Set(address,uint256,uint256)` 事件。
- `user` 是 indexed 参数，所以放 topic。
- `oldValue` 和 `newValue` 不是 indexed，所以放 data buffer。

简化 EVM 伪指令：

```text
topic0 = keccak256("Set(address,uint256,uint256)")
topic1 = CALLER

ptr = mload(0x40)
mstore(ptr + 0, oldValue)
mstore(ptr + 32, newValue)

LOG2(ptr, 64, topic0, topic1)
```

再看 anonymous event：

```solidity
event Seen(address indexed user) anonymous;
```

它没有事件签名 topic：

```text
topic0 = user
LOG1(0, 0, topic0)
```

动态非 indexed 参数例子：

```solidity
event Message(address indexed user, string text);
```

简化 data buffer：

```text
topic0 = keccak256("Message(address,string)")
topic1 = user

ptr = mload(0x40)
mstore(ptr + 0, 32)       ; text tail offset
mstore(ptr + 32, len)
mstore(ptr + 64, bytes)

LOG2(ptr, total_size, topic0, topic1)
```

需要记住的是：event 的 topic 和 data 分工很固定。indexed 参数进 topic，非 indexed 参数进
memory data buffer；非 anonymous event 还会多一个事件签名 topic。
