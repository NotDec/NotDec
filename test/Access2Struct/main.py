class Type:
    pass

class StructType(Type):
    def __init__(self):
        self.members = {}  # offset: (type, size)

    def __str__(self):
        return 'StructType(%s)' % ', '.join('%d-%s: %s' % (offset, offset+size, type) for offset, (type, size) in self.members.items())

class ArrayType(Type):
    def __init__(self, element_type, stride):
        self.element_type = element_type
        self.stride = stride
        
    def __str__(self):
        return 'ArrayType(%s, %d)' % (self.element_type, self.stride)

class BasicType(Type):
    def __init__(self, size):
        self.size = size

    def __str__(self):
        return 'BasicType(%d)' % self.size

def infer_type(patterns):
    if not patterns:
        return StructType()  # 默认为空结构体

    struct = StructType()

    if len(pattern) == 1 and pattern[0]['base'] == 0 and len(pattern[0]['strides']) == 0:
        # 简单结构体类型
        return BasicType(size=patterns[0]['access_size'])

    # 提取所有乘数项
    all_strides = set()
    for pattern in patterns:
        all_strides.update(pattern['strides'])

    remaining_patterns = patterns

    while len(all_strides) > 0:
        # 处理乘数项的最大值所代表的数组。
        max_stride = max(all_strides)
        all_strides.remove(max_stride)

        # 提取包含max_stride的模式
        has_stride_patterns = []
        for pattern in remaining_patterns:
            if max_stride in pattern['strides']:
                has_stride_patterns.append(pattern)
        for pattern in has_stride_patterns:
            remaining_patterns.remove(pattern)

        # 按基址排序
        has_stride_patterns = list(sorted(has_stride_patterns, key=lambda x: x['base']))
        
        while len(has_stride_patterns) > 0:
            range_start = has_stride_patterns[0]['base']
            range_end = has_stride_patterns[0]['base'] + max_stride

            # 提取当前组中Base在当前范围内的模式
            in_range_patterns = []
            for pattern in has_stride_patterns:
                if pattern['base'] >= range_start and pattern['base'] < range_end:
                    in_range_patterns.append(pattern)
            for pattern in in_range_patterns:
                has_stride_patterns.remove(pattern)

            sub_array_patterns = []
            # 转换为子问题的访问模式
            for pattern in in_range_patterns:
                new_strides = pattern['strides'].copy()
                new_strides.remove(max_stride)
                new_base = pattern['base'] - range_start

                # 递归处理剩余乘数项
                sub_pattern = {
                    'base': new_base,
                    'strides': new_strides,
                    'access_size': pattern['access_size']
                }
                sub_array_patterns.append(sub_pattern)
            member_type = ArrayType(infer_type(sub_array_patterns), max_stride)
            # 添加成员到结构体
            struct.members[range_start] = (member_type, max_stride)

    if len(remaining_patterns) > 0:
        # 处理剩余的基本类型
        for pattern in remaining_patterns:
            assert len(pattern['strides']) == 0
            struct.members[pattern['base']] = (BasicType(pattern['access_size']), pattern['access_size'])

    if (len(struct.members) == 1) and (0 in struct.members):
        return struct.members[0][0]

    # 返回外层数组类型
    return struct

import unittest

class TestAccess(unittest.TestCase):
    def test_1_Basic(self):
        patterns = [
            {'base': 0, 'strides': set(), 'access_size': 4},
        ]
        type = infer_type(patterns)
        # self.assertEqual(str(type), 'ArrayType(StructType(0: BasicType(4)<4>, 4: BasicType(4)<4>, 8: BasicType(4)<4>, 12: BasicType(4)<4>), 8)')
        print(type)
    
    def test_2_Basic(self):
        patterns = [
            {'base': 0, 'strides': {4}, 'access_size': 4},
        ]
        type = infer_type(patterns)
        # self.assertEqual(str(type), 'ArrayType(StructType(0: BasicType(4)<4>, 4: BasicType(4)<4>, 8: BasicType(4)<4>, 12: BasicType(4)<4>), 8)')
        print(type)
    
    def test_3(self):
        patterns = [
            {'base': 0, 'strides': {4}, 'access_size': 4},
            {'base': 4, 'strides': {4}, 'access_size': 4},
            {'base': 8, 'strides': {4}, 'access_size': 4},
        ]
        type = infer_type(patterns)
        # self.assertEqual(str(type), 'ArrayType(StructType(0: BasicType(4)<4>, 4: BasicType(4)<4>, 8: BasicType(4)<4>, 12: BasicType(4)<4>), 8)')
        print(type)
    
    def test_4(self):
        patterns = [
            {'base': 0, 'strides': {84}, 'access_size': 4},
            {'base': 4, 'strides': {8,84}, 'access_size': 4},
            {'base': 8, 'strides': {8,84}, 'access_size': 4},
        ]
        type = infer_type(patterns)
        # self.assertEqual(str(type), 'ArrayType(StructType(0: BasicType(4)<4>, 4: BasicType(4)<4>, 8: BasicType(4)<4>, 12: BasicType(4)<4>), 8)')
        print(type)
    
    def test_5(self):
        patterns = [
            {'base': 0, 'strides': set(), 'access_size': 4},
            {'base': 4, 'strides': set(), 'access_size': 4},
            {'base': 8, 'strides': set(), 'access_size': 4},
        ]
        type = infer_type(patterns)
        # self.assertEqual(str(type), 'ArrayType(StructType(0: BasicType(4)<4>, 4: BasicType(4)<4>, 8: BasicType(4)<4>, 12: BasicType(4)<4>), 8)')
        print(type)

if __name__ == '__main__':
    unittest.main()
