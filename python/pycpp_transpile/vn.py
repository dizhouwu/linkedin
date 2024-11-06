import ast
import astor
from collections import defaultdict, deque

# Original code to be optimized
source_code = """
x = 4
y = 5
z = x * y
q = x * y
w = x
u = w * y
"""

# Step 1: Parse the source code into an AST
tree = ast.parse(source_code)

# Step 2: Define the Value Numbering Transformer
class ValueNumberingTransformer(ast.NodeTransformer):
    def __init__(self):
        self.subexprs = {}
        self.var_subs = {}  # Track direct variable assignments (e.g., w = x)
        self.new_assignments = []
        self.value_counter = 0

    def get_value_name(self):
        # Generate a new unique variable name
        var_name = f"value_{self.value_counter}"
        self.value_counter += 1
        return var_name

    def visit_BinOp(self, node):
        # Visit the left and right to get the optimized nodes
        self.generic_visit(node)

        # Substitute variables if there's a direct assignment
        if isinstance(node.left, ast.Name) and node.left.id in self.var_subs:
            node.left = ast.Name(id=self.var_subs[node.left.id], ctx=ast.Load())
        if isinstance(node.right, ast.Name) and node.right.id in self.var_subs:
            node.right = ast.Name(id=self.var_subs[node.right.id], ctx=ast.Load())

        # Create a unique key for the subexpression
        key = (type(node.op), ast.dump(node.left), ast.dump(node.right))

        if key in self.subexprs:
            # If the subexpression has already been seen, replace it with the variable
            return ast.Name(id=self.subexprs[key], ctx=ast.Load())
        else:
            # If it's a new subexpression, create a new variable name
            var_name = self.get_value_name()
            self.subexprs[key] = var_name

            # Create a new assignment for the subexpression
            new_assign = ast.Assign(targets=[ast.Name(id=var_name, ctx=ast.Store())],
                                    value=node)
            self.new_assignments.append(new_assign)
            return ast.Name(id=var_name, ctx=ast.Load())

    def visit_Assign(self, node):
        # If it's a direct assignment (like w = x), store it in var_subs
        if isinstance(node.value, ast.Name) and len(node.targets) == 1:
            target = node.targets[0]
            if isinstance(target, ast.Name):
                self.var_subs[target.id] = node.value.id

        # Transform the right-hand side of the assignment
        self.generic_visit(node)
        return node

# Step 3: Optimize the AST with Value Numbering
transformer = ValueNumberingTransformer()
optimized_tree = transformer.visit(tree)

# Collect original assignments and newly created subexpression assignments
original_assignments = []
for node in optimized_tree.body:
    if isinstance(node, ast.Assign):
        # Check if the assignment is to a constant
        if isinstance(node.targets[0], ast.Name) and isinstance(node.value, (ast.Num, ast.Constant)):
            original_assignments.append(node)

# Collect all new assignments for common subexpressions
subexpr_assignments = [assign for assign in transformer.new_assignments]

# Step 4: Dependency graph analysis for assignment ordering
dependencies = defaultdict(set)
assignment_map = {}

# Collect dependencies for each assignment
for node in optimized_tree.body + subexpr_assignments:
    if isinstance(node, ast.Assign):
        target = node.targets[0].id
        # Find dependencies in the right-hand side (RHS) of the assignment
        used_vars = {n.id for n in ast.walk(node.value) if isinstance(n, ast.Name)}
        dependencies[target].update(used_vars)
        assignment_map[target] = node

# Calculate in-degrees and construct graph
in_degree = defaultdict(int)
graph = defaultdict(list)

for var, deps in dependencies.items():
    for dep in deps:
        graph[dep].append(var)
        in_degree[var] += 1

# Topological sort using Kahn's algorithm
sorted_assignments = []
queue = deque([node for node in assignment_map if in_degree[node] == 0])

while queue:
    var = queue.popleft()
    sorted_assignments.append(assignment_map[var])
    for neighbor in graph[var]:
        in_degree[neighbor] -= 1
        if in_degree[neighbor] == 0:
            queue.append(neighbor)

# Step 5: Compile the optimized AST back to code in sorted order
optimized_tree.body = sorted_assignments

# Step 6: Generate and print the optimized code
optimized_code = astor.to_source(optimized_tree)
print("Optimized Code:\n", optimized_code)

# Step 7: Execute the optimized code
exec_locals = {}
exec(optimized_code, {}, exec_locals)

# Print results of the executed code
for key, value in exec_locals.items():
    print(f"{key} = {value}")
