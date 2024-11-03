import ast

# Define a mapping from Python types to C++ types
type_mapping = {
    'int': 'int',
    'float': 'double',
    'str': 'std::string',
    'list': 'std::vector<double>',  # Assuming list of doubles for simplicity
}

# Define custom operators
operator_mapping = {
    ast.Add: '+',
    ast.Sub: '-',
    ast.Mult: '*',
    ast.Div: '/',
}

# Function to map Python types to C++ types
def map_type(python_type):
    return type_mapping.get(python_type, python_type)

def transpile(node):
    if isinstance(node, ast.FunctionDef):
        return f"{map_type(node.returns.id)} {node.name}({', '.join([f'{map_type(arg.annotation.id)} {arg.arg}' for arg in node.args.args])}) {{\n" + \
               ''.join(transpile(child) for child in node.body) + "}\n"

    elif isinstance(node, ast.Return):
        return f"    return {transpile(node.value)};\n"

    elif isinstance(node, ast.BinOp):
        return f"{transpile(node.left)} {operator_mapping[type(node.op)]} {transpile(node.right)}"

    elif isinstance(node, ast.Constant):
        return str(node.n)

    elif isinstance(node, ast.Name):
        return node.id

    return ""

def transpile_code(user_code):
    parsed_code = ast.parse(user_code)
    cpp_code = ""

    # Generate C++ code
    for node in parsed_code.body:
        cpp_code += transpile(node)

    return cpp_code

def compile_and_execute(cpp_code):
    import cppyy
    cpp_code_with_main = f"""
#include <iostream>
#include <vector>
using namespace std;

{cpp_code}

int main() {{
    cout << "Result: " << add(5, 3) << endl;  // Calling the function for testing
    return 0;
}}
"""
    cppyy.cppdef(cpp_code_with_main)
    cppyy.gbl.main()

# User input (Python code)
user_code = """
def add(a: int, b: int) -> int:
    return a + b
"""

# Transpile the Python code to C++
cpp_code = transpile_code(user_code)

# Compile and execute the C++ code
compile_and_execute(cpp_code)
