import ast
import astor

# Original code to be optimized
source_code = """
x = 4
y = 5
z = x * y 
w = x
u = w * y 
"""

# Step 1: Parse the source code into an AST
tree = ast.parse(source_code)

# Step 2: Perform CSE on the AST
class CSETransformer(ast.NodeTransformer):
    def __init__(self):
        self.subexprs = {}
        self.new_assignments = []

    def visit_BinOp(self, node):
        # Visit the left and right to get the optimized nodes
        self.generic_visit(node)

        # Create a unique key for the subexpression
        key = (type(node.op), ast.dump(node.left), ast.dump(node.right))

        if key in self.subexprs:
            # If the subexpression has already been seen, replace it with the variable
            return ast.Name(id=self.subexprs[key], ctx=ast.Load())
        else:
            # If it’s a new subexpression, create a new variable name
            var_name = f"subexpr_{len(self.subexprs)}"
            self.subexprs[key] = var_name

            # Create a new assignment for the subexpression
            new_assign = ast.Assign(targets=[ast.Name(id=var_name, ctx=ast.Store())],
                                    value=node)
            self.new_assignments.append(new_assign)
            return ast.Name(id=var_name, ctx=ast.Load())

    def visit_Assign(self, node):
        # Transform the right-hand side of the assignment
        self.generic_visit(node)
        return node

# Create an instance of the transformer and optimize the AST
transformer = CSETransformer()
optimized_tree = transformer.visit(tree)

# Step 3: Separate assignments based on the required priority
constant_assignments = []
direct_var_assignments = []
subexpr_assignments = transformer.new_assignments
final_assignments = []

for node in optimized_tree.body:
    if isinstance(node, ast.Assign):
        target = node.targets[0]
        # Assignments to constants
        if isinstance(node.value, (ast.Num, ast.Constant)):
            constant_assignments.append(node)
        # Direct variable-to-variable assignments (e.g., w = x)
        elif isinstance(node.value, ast.Name) and not node.value.id.startswith("subexpr_"):
            direct_var_assignments.append(node)
        # Other assignments that depend on prior calculations
        else:
            final_assignments.append(node)

# Step 4: Build the new body with constants, direct assignments, subexpressions, then final assignments
optimized_tree.body = (
    constant_assignments +
    direct_var_assignments +
    subexpr_assignments +
    final_assignments
)

# Step 5: Compile the optimized AST back to code
optimized_code = astor.to_source(optimized_tree)

print("Optimized Code:\n", optimized_code)

# Step 6: Execute the optimized code
exec_locals = {}
exec(optimized_code, {}, exec_locals)

# Print results of the executed code
for key, value in exec_locals.items():
    print(f"{key} = {value}")
