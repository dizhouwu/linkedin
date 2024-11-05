import ast
import astor

code = """
a = 2
b = 3
c = a + b
d = a + b
e = a * b
"""

tree = ast.parse(code)

cse_map = {}

def process_expression(node):
    if isinstance(node, ast.BinOp):
        left_key = process_expression(node.left)
        right_key = process_expression(node.right)
        key = (type(node.op), left_key, right_key)
        if key not in cse_map:
            cse_map[key] = len(cse_map)
        return cse_map[key]
    elif isinstance(node, ast.Name):
        return node.id
    elif isinstance(node, ast.Constant):
        return node.value
    else:
        raise ValueError(f"Unsupported node type: {type(node)}")

for node in ast.walk(tree):
    if isinstance(node, ast.Assign):
        for target in node.targets:
            if isinstance(target, ast.Name):
                target_value = node.value
                target_key = process_expression(target_value)
                if isinstance(target_value, ast.BinOp):
                    target_value = ast.Name(id=f'cse_{target_key}', ctx=ast.Load())
                node.value = target_value

print(cse_map)

optimized_code = astor.to_source(tree)
print(optimized_code)